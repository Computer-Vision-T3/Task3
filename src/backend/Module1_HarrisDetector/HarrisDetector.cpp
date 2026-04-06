#include "HarrisDetector.h"
#include "../core/ImageUtils.h"
#include <chrono>
#include <iostream>
#include <cmath>
#include <future>
#include <thread>

// ── OPTIMIZATION: Custom Multithreading Engine ────────────────────────────────
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
            // Spawn async threads for each chunk
            futures.push_back(std::async(std::launch::async, [startRow, endRow, &func]() {
                func(startRow, endRow);
            }));
        }
        for (auto& f : futures) f.get(); // Wait for all threads to finish
    }
}

// ── Main entry point ──────────────────────────────────────────────────────────
std::tuple<cv::Mat, std::vector<cv::KeyPoint>>
HarrisDetector::detect(const cv::Mat& src, double k, int /*blockSize*/, 
                       int apertureSize, int threshold, bool colorMode, double& timingMs)
{
    cv::Mat gray = ImageUtils::toGray(src);

    cv::Mat responseRaw;
    auto t0 = std::chrono::high_resolution_clock::now();
    computeResponse(gray, responseRaw, k, apertureSize);
    auto t1 = std::chrono::high_resolution_clock::now();
    timingMs = std::chrono::duration<double, std::milli>(t1 - t0).count();

    std::cout << "[Harris]  Computation time : " << timingMs << " ms\n";

    // ── OPTIMIZATION: Threaded Manual Normalization ──
    float minVal = std::numeric_limits<float>::max();
    float maxVal = std::numeric_limits<float>::lowest();
    for (int y = 0; y < responseRaw.rows; ++y) {
        const float* row = responseRaw.ptr<float>(y);
        for (int x = 0; x < responseRaw.cols; ++x) {
            if (row[x] < minVal) minVal = row[x];
            if (row[x] > maxVal) maxVal = row[x];
        }
    }

    cv::Mat responseNorm = cv::Mat::zeros(responseRaw.size(), CV_32FC1);
    float range = maxVal - minVal;
    if (range > 0) {
        parallel_for_rows(responseRaw.rows, [&](int startRow, int endRow) {
            for (int y = startRow; y < endRow; ++y) {
                const float* srcRow = responseRaw.ptr<float>(y);
                float* dstRow = responseNorm.ptr<float>(y);
                for (int x = 0; x < responseRaw.cols; ++x) {
                    dstRow[x] = 255.0f * (srcRow[x] - minVal) / range;
                }
            }
        });
    }

    cv::Mat thresholded = applyThreshold(responseNorm, threshold);
    cv::Mat nmsMap = nonMaximalSuppression(thresholded, 3);
    std::vector<cv::KeyPoint> keypoints = collectKeyPoints(nmsMap, responseNorm);
    
    cv::Mat canvas = colorMode ? src.clone() : ImageUtils::toBGR8(gray);
    drawCorners(canvas, keypoints);

    return { canvas, keypoints };
}

// ── 100% From-Scratch Harris Response ─────────────────────────────────────────
void HarrisDetector::computeResponse(const cv::Mat& gray, cv::Mat& dst, double k, int apertureSize)
{
    cv::Mat grayFloat;
    gray.convertTo(grayFloat, CV_32FC1);

    // 1. OPTIMIZATION: Separable Sobel Kernels (1D instead of 2D)
    std::vector<float> sobelX_H = {-1.0f, 0.0f, 1.0f};
    std::vector<float> sobelX_V = { 1.0f, 2.0f, 1.0f};

    std::vector<float> sobelY_H = { 1.0f, 2.0f, 1.0f};
    std::vector<float> sobelY_V = {-1.0f, 0.0f, 1.0f};

    // 2. Compute Gradients in passes (Horizontal then Vertical)
    cv::Mat Ix_tmp = apply1DConvolutionX(grayFloat, sobelX_H);
    cv::Mat Ix     = apply1DConvolutionY(Ix_tmp,    sobelX_V);

    cv::Mat Iy_tmp = apply1DConvolutionX(grayFloat, sobelY_H);
    cv::Mat Iy     = apply1DConvolutionY(Iy_tmp,    sobelY_V);

    // 3. OPTIMIZATION: Threaded matrix arithmetic
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

    // 4. OPTIMIZATION: Separable 1D Gaussian Integration
    auto gauss1D = create1DGaussianKernel(7, 2.0f);
    cv::Mat a = apply1DConvolutionY(apply1DConvolutionX(Ix2, gauss1D), gauss1D);
    cv::Mat c = apply1DConvolutionY(apply1DConvolutionX(Iy2, gauss1D), gauss1D);
    cv::Mat b = apply1DConvolutionY(apply1DConvolutionX(IxIy, gauss1D), gauss1D);

    // 5. Threaded Harris equation
    dst = cv::Mat::zeros(gray.size(), CV_32FC1);
    parallel_for_rows(gray.rows, [&](int startRow, int endRow) {
        for (int y = startRow; y < endRow; ++y) {
            const float* pa = a.ptr<float>(y);
            const float* pc = c.ptr<float>(y);
            const float* pb = b.ptr<float>(y);
            float* pdst = dst.ptr<float>(y);

            for (int x = 0; x < gray.cols; ++x) {
                float det = (pa[x] * pc[x]) - (pb[x] * pb[x]);
                float trace = pa[x] + pc[x];
                pdst[x] = det - static_cast<float>(k) * (trace * trace);
            }
        }
    });
}

// ── Manual Math Primitives ────────────────────────────────────────────────────
cv::Mat HarrisDetector::apply1DConvolutionX(const cv::Mat& src, const std::vector<float>& kernel) {
    cv::Mat dst = cv::Mat::zeros(src.size(), CV_32FC1);
    int pad = kernel.size() / 2;
    
    parallel_for_rows(src.rows, [&](int startRow, int endRow) {
        for (int y = startRow; y < endRow; ++y) {
            const float* srcRow = src.ptr<float>(y);
            float* dstRow = dst.ptr<float>(y);
            for (int x = pad; x < src.cols - pad; ++x) {
                float sum = 0.0f;
                // SIMD-friendly tight inner loop
                for (int k = -pad; k <= pad; ++k) {
                    sum += srcRow[x + k] * kernel[k + pad];
                }
                dstRow[x] = sum;
            }
        }
    });
    return dst;
}

cv::Mat HarrisDetector::apply1DConvolutionY(const cv::Mat& src, const std::vector<float>& kernel) {
    cv::Mat dst = cv::Mat::zeros(src.size(), CV_32FC1);
    int pad = kernel.size() / 2;
    
    parallel_for_rows(src.rows, [&](int startRow, int endRow) {
        if (startRow < pad) startRow = pad;
        if (endRow > src.rows - pad) endRow = src.rows - pad;
        
        std::vector<const float*> rowPtrs(kernel.size());
        for (int y = startRow; y < endRow; ++y) {
            // OPTIMIZATION: Pointer caching to maintain CPU L1 cache hits
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

std::vector<float> HarrisDetector::create1DGaussianKernel(int size, float sigma) {
    std::vector<float> kernel(size);
    float sum = 0.0f;
    int pad = size / 2;
    for (int i = -pad; i <= pad; ++i) {
        kernel[i + pad] = std::exp(-(i * i) / (2 * sigma * sigma));
        sum += kernel[i + pad];
    }
    for (int i = 0; i < size; ++i) kernel[i] /= sum;
    return kernel;
}

cv::Mat HarrisDetector::applyThreshold(const cv::Mat& responseNorm, int thresh) {
    cv::Mat out = cv::Mat::zeros(responseNorm.size(), CV_32FC1);
    parallel_for_rows(responseNorm.rows, [&](int startRow, int endRow) {
        for (int i = startRow; i < endRow; ++i) {
            const float* srcRow = responseNorm.ptr<float>(i);
            float* dstRow = out.ptr<float>(i);
            for (int j = 0; j < responseNorm.cols; ++j) {
                if (srcRow[j] > thresh) dstRow[j] = srcRow[j];
            }
        }
    });
    return out;
}

cv::Mat HarrisDetector::nonMaximalSuppression(const cv::Mat& response, int winSize) {
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

std::vector<cv::KeyPoint> HarrisDetector::collectKeyPoints(const cv::Mat& nmsMap, const cv::Mat& responseNorm) {
    std::vector<cv::KeyPoint> kps;
    for (int i = 0; i < nmsMap.rows; ++i) {
        const float* row = nmsMap.ptr<float>(i);
        const float* normRow = responseNorm.ptr<float>(i);
        for (int j = 0; j < nmsMap.cols; ++j) {
            if (row[j] > 0.0f) {
                float strength = normRow[j];
                float size = strength * 30.0f / 255.0f;
                kps.emplace_back(cv::Point2f((float)j, (float)i), size, -1.0f, strength);
            }
        }
    }
    return kps;
}

void HarrisDetector::drawCorners(cv::Mat& canvas, const std::vector<cv::KeyPoint>& kps) {
    for (const auto& kp : kps) {
        float s = kp.response;
        cv::Scalar color =
            (s < 85.0f)  ? cv::Scalar( 45, 155, 111) :  
            (s < 170.0f) ? cv::Scalar(207,  79,  91) :  
                           cv::Scalar( 48,  90, 216);    
        cv::circle(canvas, cv::Point(cvRound(kp.pt.x), cvRound(kp.pt.y)), 4, color, 1, cv::LINE_AA);
    }
}