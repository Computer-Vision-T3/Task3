#include "LambdaDetector.h"
#include "../core/ImageUtils.h"
#include "../core/MathUtils.h"
#include <chrono>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <future>
#include <thread>
#include <limits>

// ── OPTIMIZATION: Custom Multithreading Engine & Math Primitives ──────────────
namespace {
    template <typename Func>
    void parallel_for_rows(int totalRows, Func func) {
        int numThreads = std::thread::hardware_concurrency();
        if (numThreads == 0) numThreads = 4;
        
        int chunk = totalRows / numThreads;
        std::vector<std::future<void>> futures;
        
        for (int t = 0; t < numThreads; ++t) {
            int startRow = t * chunk;
            int endRow = (t == numThreads - 1) ? totalRows : startRow + chunk;
            futures.push_back(std::async(std::launch::async, [startRow, endRow, &func]() {
                func(startRow, endRow);
            }));
        }
        for (auto& f : futures) f.get(); 
    }

    // 1D Convolution X (Horizontal)
    cv::Mat apply1DConvolutionX(const cv::Mat& src, const std::vector<float>& kernel) {
        cv::Mat dst = cv::Mat::zeros(src.size(), CV_32FC1);
        int pad = kernel.size() / 2;
        
        parallel_for_rows(src.rows, [&](int startRow, int endRow) {
            for (int y = startRow; y < endRow; ++y) {
                const float* srcRow = src.ptr<float>(y);
                float* dstRow = dst.ptr<float>(y);
                for (int x = pad; x < src.cols - pad; ++x) {
                    float sum = 0.0f;
                    for (int k = -pad; k <= pad; ++k) {
                        sum += srcRow[x + k] * kernel[k + pad];
                    }
                    dstRow[x] = sum;
                }
            }
        });
        return dst;
    }

    // 1D Convolution Y (Vertical) with L1 Cache Pointer Optimization
    cv::Mat apply1DConvolutionY(const cv::Mat& src, const std::vector<float>& kernel) {
        cv::Mat dst = cv::Mat::zeros(src.size(), CV_32FC1);
        int pad = kernel.size() / 2;
        
        parallel_for_rows(src.rows, [&](int startRow, int endRow) {
            if (startRow < pad) startRow = pad;
            if (endRow > src.rows - pad) endRow = src.rows - pad;
            
            std::vector<const float*> rowPtrs(kernel.size());
            for (int y = startRow; y < endRow; ++y) {
                for(int k = -pad; k <= pad; ++k) {
                    rowPtrs[k + pad] = src.ptr<float>(y + k);
                }
                float* dstRow = dst.ptr<float>(y);
                
                for (int x = 0; x < src.cols; ++x) {
                    float sum = 0.0f;
                    for (int k = -pad; k <= pad; ++k) {
                        sum += rowPtrs[k + pad][x] * kernel[k + pad];
                    }
                    dstRow[x] = sum;
                }
            }
        });
        return dst;
    }
}

// ── Main entry point ──────────────────────────────────────────────────────────
std::tuple<cv::Mat, std::vector<cv::KeyPoint>>
LambdaDetector::detect(const cv::Mat& src, int maxKeypoints, double qualityLevel,
                       int minDistance, int blockSize, bool colorMode, double& timingMs)
{
    cv::Mat gray = ImageUtils::toGray(src);

    // Steps 1-2: λ_min response map (timed)
    auto t0 = std::chrono::high_resolution_clock::now();
    cv::Mat lambdaMap = computeLambdaMin(gray, blockSize);
    auto t1 = std::chrono::high_resolution_clock::now();
    timingMs = std::chrono::duration<double, std::milli>(t1 - t0).count();

    std::cout << "[Lambda]  Computation time : " << timingMs << " ms\n";

    // Step 3: quality threshold
    cv::Mat thresholded = applyQualityThreshold(lambdaMap, qualityLevel);

    // Step 4: Threaded NMS
    cv::Mat nmsMap = nonMaximalSuppression(thresholded, 3);

    // ── OPTIMIZATION: Threaded Manual Normalization (with bug fix) ──
    float minVal = std::numeric_limits<float>::max();
    float maxVal = std::numeric_limits<float>::lowest();
    for (int y = 0; y < lambdaMap.rows; ++y) {
        const float* row = lambdaMap.ptr<float>(y);
        for (int x = 0; x < lambdaMap.cols; ++x) {
            if (row[x] < minVal) minVal = row[x];
            if (row[x] > maxVal) maxVal = row[x];
        }
    }

    // ⭐ Clamp minimum to 0 so negative flat/edge spaces stay at 0
    if (minVal < 0.0f) minVal = 0.0f;

    cv::Mat lambdaNorm = cv::Mat::zeros(lambdaMap.size(), CV_32FC1);
    float range = maxVal - minVal;
    if (range > 0) {
        parallel_for_rows(lambdaMap.rows, [&](int startRow, int endRow) {
            for (int y = startRow; y < endRow; ++y) {
                const float* srcRow = lambdaMap.ptr<float>(y);
                float* dstRow = lambdaNorm.ptr<float>(y);
                for (int x = 0; x < lambdaMap.cols; ++x) {
                    dstRow[x] = 255.0f * (srcRow[x] - minVal) / range;
                }
            }
        });
    }

    // Step 5: collect, rank, spacing-filter
    std::vector<cv::KeyPoint> keypoints =
        collectKeyPoints(nmsMap, lambdaNorm, maxKeypoints, minDistance);

    std::cout << "[Lambda]  Features detected: " << keypoints.size() << "\n";

    // Step 6: draw
    cv::Mat canvas = colorMode ? src.clone() : ImageUtils::toBGR8(gray);
    drawCorners(canvas, keypoints);

    return { canvas, keypoints };
}

// ── Steps 1-2: From-Scratch λ_min map ─────────────────────────────────────────
cv::Mat LambdaDetector::computeLambdaMin(const cv::Mat& gray, int blockSize)
{
    cv::Mat grayFloat;
    gray.convertTo(grayFloat, CV_32FC1);

    // 1. OPTIMIZATION: Separable Sobel Kernels
    std::vector<float> sobelX_H = {-1.0f, 0.0f, 1.0f};
    std::vector<float> sobelX_V = { 1.0f, 2.0f, 1.0f};
    std::vector<float> sobelY_H = { 1.0f, 2.0f, 1.0f};
    std::vector<float> sobelY_V = {-1.0f, 0.0f, 1.0f};

    // Compute Gradients
    cv::Mat Ix_tmp = apply1DConvolutionX(grayFloat, sobelX_H);
    cv::Mat Ix     = apply1DConvolutionY(Ix_tmp,    sobelX_V);

    cv::Mat Iy_tmp = apply1DConvolutionX(grayFloat, sobelY_H);
    cv::Mat Iy     = apply1DConvolutionY(Iy_tmp,    sobelY_V);

    // 2. OPTIMIZATION: Threaded matrix arithmetic (Ix^2, Iy^2, IxIy)
    cv::Mat Ix2 = cv::Mat::zeros(gray.size(), CV_32FC1);
    cv::Mat Iy2 = cv::Mat::zeros(gray.size(), CV_32FC1);
    cv::Mat IxIy = cv::Mat::zeros(gray.size(), CV_32FC1);

    parallel_for_rows(gray.rows, [&](int startRow, int endRow) {
        for (int y = startRow; y < endRow; ++y) {
            const float* px = Ix.ptr<float>(y);
            const float* py = Iy.ptr<float>(y);
            float* px2 = Ix2.ptr<float>(y);
            float* py2 = Iy2.ptr<float>(y);
            float* pxy = IxIy.ptr<float>(y);

            for (int x = 0; x < gray.cols; ++x) {
                float vx = px[x];
                float vy = py[x];
                px2[x] = vx * vx;
                py2[x] = vy * vy;
                pxy[x] = vx * vy;
            }
        }
    });

    // 3. OPTIMIZATION: Separable 1D Box Filter Integration
    // A box filter is just summing 1.0s. (normalize=false equivalent)
    std::vector<float> box1D(blockSize, 1.0f);
    
    cv::Mat a = apply1DConvolutionY(apply1DConvolutionX(Ix2, box1D), box1D);
    cv::Mat c = apply1DConvolutionY(apply1DConvolutionX(Iy2, box1D), box1D);
    cv::Mat b = apply1DConvolutionY(apply1DConvolutionX(IxIy, box1D), box1D);

    // 4. Threaded λ_min equation
    cv::Mat lambdaMap = cv::Mat::zeros(gray.size(), CV_32FC1);
    parallel_for_rows(gray.rows, [&](int startRow, int endRow) {
        for (int y = startRow; y < endRow; ++y) {
            const float* pa = a.ptr<float>(y);
            const float* pc = c.ptr<float>(y);
            const float* pb = b.ptr<float>(y);
            float* pdst = lambdaMap.ptr<float>(y);

            for (int x = 0; x < gray.cols; ++x) {
                float ai = pa[x];
                float ci = pc[x];
                float bi = pb[x];
                float disc = std::sqrt((ai - ci) * (ai - ci) + 4.0f * bi * bi);
                pdst[x] = 0.5f * ((ai + ci) - disc);
            }
        }
    });
    
    return lambdaMap;
}

// ── Step 3: Manual Threaded Quality Threshold ─────────────────────────────────
cv::Mat LambdaDetector::applyQualityThreshold(const cv::Mat& lambdaMap, double qualityLevel)
{
    // Find absolute maximum sequentially
    float maxVal = std::numeric_limits<float>::lowest();
    for (int y = 0; y < lambdaMap.rows; ++y) {
        const float* row = lambdaMap.ptr<float>(y);
        for (int x = 0; x < lambdaMap.cols; ++x) {
            if (row[x] > maxVal) maxVal = row[x];
        }
    }

    float minAccepted = static_cast<float>(qualityLevel * maxVal);
    cv::Mat out = cv::Mat::zeros(lambdaMap.size(), CV_32FC1);
    
    // Threshold with threads
    parallel_for_rows(lambdaMap.rows, [&](int startRow, int endRow) {
        for (int i = startRow; i < endRow; ++i) {
            const float* srcRow = lambdaMap.ptr<float>(i);
            float* dstRow = out.ptr<float>(i);
            for (int j = 0; j < lambdaMap.cols; ++j) {
                if (srcRow[j] >= minAccepted) dstRow[j] = srcRow[j];
            }
        }
    });
    return out;
}

// ── Step 4: Optimized Threaded NMS ────────────────────────────────────────────
cv::Mat LambdaDetector::nonMaximalSuppression(const cv::Mat& response, int winSize)
{
    cv::Mat out = cv::Mat::zeros(response.size(), CV_32FC1);
    int half = winSize / 2;

    parallel_for_rows(response.rows, [&](int startRow, int endRow) {
        if (startRow < half) startRow = half;
        if (endRow > response.rows - half) endRow = response.rows - half;

        for (int i = startRow; i < endRow; ++i) {
            const float* rowC = response.ptr<float>(i);
            float* outRow = out.ptr<float>(i);
            
            for (int j = half; j < response.cols - half; ++j) {
                float center = rowC[j];
                if (center <= 0.0f) continue;

                bool isMax = true;
                for (int di = -half; di <= half && isMax; ++di) {
                    const float* rowN = response.ptr<float>(i + di);
                    for (int dj = -half; dj <= half; ++dj) {
                        if (rowN[j + dj] > center) {
                            isMax = false;
                            break;
                        }
                    }
                }
                if (isMax) outRow[j] = center;
            }
        }
    });
    return out;
}

// ── Step 5: collect, rank, and enforce minDistance spacing ───────────────────
std::vector<cv::KeyPoint>
LambdaDetector::collectKeyPoints(const cv::Mat& nmsMap, const cv::Mat& lambdaNorm,
                                 int maxKeypoints, int minDistance)
{
    std::vector<cv::KeyPoint> candidates;
    for (int i = 0; i < nmsMap.rows; ++i) {
        for (int j = 0; j < nmsMap.cols; ++j) {
            if (nmsMap.at<float>(i, j) > 0.0f) {
                float strength = lambdaNorm.at<float>(i, j);
                float size = strength * 30.0f / 255.0f;
                candidates.emplace_back(cv::Point2f((float)j, (float)i),
                                        size, -1.0f, strength);
            }
        }
    }

    std::sort(candidates.begin(), candidates.end(),
              [](const cv::KeyPoint& a, const cv::KeyPoint& b) {
                  return a.response > b.response;
              });

    std::vector<cv::KeyPoint> selected;
    float minDist2 = static_cast<float>(minDistance * minDistance);

    for (const auto& kp : candidates) {
        if (maxKeypoints > 0 && (int)selected.size() >= maxKeypoints) break;

        bool tooClose = false;
        for (const auto& sel : selected) {
            float dx = kp.pt.x - sel.pt.x;
            float dy = kp.pt.y - sel.pt.y;
            if (dx * dx + dy * dy < minDist2) { tooClose = true; break; }
        }
        if (!tooClose) selected.push_back(kp);
    }

    return selected;
}

// ── Step 6: colour-coded drawing ─────────────────────────────────────────────
void LambdaDetector::drawCorners(cv::Mat& canvas, const std::vector<cv::KeyPoint>& kps)
{
    for (const auto& kp : kps) {
        float s = kp.response;
        cv::Scalar color =
            (s < 85.0f)  ? cv::Scalar( 45, 155, 111) :   // sage green
            (s < 170.0f) ? cv::Scalar(207,  79,  91) :   // purple
                           cv::Scalar( 48,  90, 216);    // coral
        cv::circle(canvas, cv::Point(cvRound(kp.pt.x), cvRound(kp.pt.y)),
                   4, color, 1, cv::LINE_AA);
    }
}