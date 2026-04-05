#include "LambdaDetector.h"
#include "../core/ImageUtils.h"
#include "../core/MathUtils.h"
#include <chrono>
#include <iostream>
#include <algorithm>
#include <cmath>

// ── Main entry point ──────────────────────────────────────────────────────────
std::tuple<cv::Mat, std::vector<cv::KeyPoint>>
LambdaDetector::detect(const cv::Mat& src,
                        int maxKeypoints,
                        double qualityLevel,
                        int minDistance,
                        int blockSize,
                        bool colorMode,
                        double& timingMs)
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

    // Step 4: NMS
    cv::Mat nmsMap = nonMaximalSuppression(thresholded, 3);

    // Normalize for display / strength encoding
    cv::Mat lambdaNorm;
    cv::normalize(lambdaMap, lambdaNorm, 0, 255, cv::NORM_MINMAX, CV_32FC1);

    // Step 5: collect, rank, spacing-filter
    std::vector<cv::KeyPoint> keypoints =
        collectKeyPoints(nmsMap, lambdaNorm, maxKeypoints, minDistance);

    std::cout << "[Lambda]  Features detected: " << keypoints.size() << "\n";

    // Step 6: draw
    cv::Mat canvas = colorMode ? src.clone() : ImageUtils::toBGR8(gray);
    drawCorners(canvas, keypoints);

    return { canvas, keypoints };
}

// ── Steps 1-2: λ_min map ─────────────────────────────────────────────────────
//
// The Shi-Tomasi / "Lambda" score is the minimum eigenvalue of the
// second-moment matrix M:
//
//   M = [Σ(Ix²)  Σ(Ix·Iy)]   (summed over blockSize×blockSize window)
//       [Σ(Ix·Iy) Σ(Iy²) ]
//
// For a 2×2 symmetric matrix:
//   λ_min = ½ ( (a+c) - √((a-c)²+4b²) )
//   where a=Σ(Ix²), b=Σ(Ix·Iy), c=Σ(Iy²)
//
// This is more stable than Harris R because it directly measures the
// weaker of the two gradient directions rather than a polynomial proxy.
cv::Mat LambdaDetector::computeLambdaMin(const cv::Mat& gray, int blockSize)
{
    // Step 1: image gradients (same Sobel as Harris)
    cv::Mat Ix, Iy;
    cv::Sobel(gray, Ix, CV_32FC1, 1, 0, 3, 1, 0, cv::BORDER_DEFAULT);
    cv::Sobel(gray, Iy, CV_32FC1, 0, 1, 3, 1, 0, cv::BORDER_DEFAULT);

    // Step 2: second-moment matrix elements
    cv::Mat Ix2, Iy2, IxIy;
    cv::pow(Ix, 2.0, Ix2);
    cv::pow(Iy, 2.0, Iy2);
    cv::multiply(Ix, Iy, IxIy);

    // Integrate over local neighbourhood using box filter of blockSize
    cv::Mat a, c, b;
    cv::Size ksize(blockSize, blockSize);
    cv::boxFilter(Ix2,  a, CV_32FC1, ksize, cv::Point(-1,-1), false, cv::BORDER_DEFAULT);
    cv::boxFilter(Iy2,  c, CV_32FC1, ksize, cv::Point(-1,-1), false, cv::BORDER_DEFAULT);
    cv::boxFilter(IxIy, b, CV_32FC1, ksize, cv::Point(-1,-1), false, cv::BORDER_DEFAULT);

    // Compute λ_min = ½ * ( (a+c) - sqrt((a-c)² + 4b²) )
    cv::Mat lambdaMap(gray.size(), CV_32FC1);
    for (int i = 0; i < gray.rows; ++i) {
        for (int j = 0; j < gray.cols; ++j) {
            float ai = a.at<float>(i, j);
            float ci = c.at<float>(i, j);
            float bi = b.at<float>(i, j);
            float disc = std::sqrt((ai - ci) * (ai - ci) + 4.0f * bi * bi);
            lambdaMap.at<float>(i, j) = 0.5f * ((ai + ci) - disc);
        }
    }
    return lambdaMap;
}

// ── Step 3: quality threshold ─────────────────────────────────────────────────
// Keeps only pixels whose λ_min >= qualityLevel * max(λ_min)
// (same semantics as OpenCV goodFeaturesToTrack)
cv::Mat LambdaDetector::applyQualityThreshold(const cv::Mat& lambdaMap,
                                                double qualityLevel)
{
    double maxVal;
    cv::minMaxLoc(lambdaMap, nullptr, &maxVal);
    float minAccepted = static_cast<float>(qualityLevel * maxVal);

    cv::Mat out = cv::Mat::zeros(lambdaMap.size(), CV_32FC1);
    for (int i = 0; i < lambdaMap.rows; ++i)
        for (int j = 0; j < lambdaMap.cols; ++j)
            if (lambdaMap.at<float>(i, j) >= minAccepted)
                out.at<float>(i, j) = lambdaMap.at<float>(i, j);
    return out;
}

// ── Step 4: NMS — identical algorithm to Harris ───────────────────────────────
cv::Mat LambdaDetector::nonMaximalSuppression(const cv::Mat& response,
                                               int winSize)
{
    cv::Mat out = cv::Mat::zeros(response.size(), CV_32FC1);
    int half = winSize / 2;

    for (int i = half; i < response.rows - half; ++i) {
        for (int j = half; j < response.cols - half; ++j) {
            float center = response.at<float>(i, j);
            if (center <= 0.0f) continue;

            bool isMax = true;
            for (int di = -half; di <= half && isMax; ++di)
                for (int dj = -half; dj <= half && isMax; ++dj)
                    if (response.at<float>(i + di, j + dj) > center)
                        isMax = false;

            if (isMax)
                out.at<float>(i, j) = center;
        }
    }
    return out;
}

// ── Step 5: collect, rank, and enforce minDistance spacing ───────────────────
std::vector<cv::KeyPoint>
LambdaDetector::collectKeyPoints(const cv::Mat& nmsMap,
                                  const cv::Mat& lambdaNorm,
                                  int maxKeypoints,
                                  int minDistance)
{
    // Gather all candidates with their strength
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

    // Sort by descending response strength (strongest corners first)
    std::sort(candidates.begin(), candidates.end(),
              [](const cv::KeyPoint& a, const cv::KeyPoint& b) {
                  return a.response > b.response;
              });

    // Greedy minimum-distance selection (same as goodFeaturesToTrack)
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
        if (!tooClose)
            selected.push_back(kp);
    }

    return selected;
}

// ── Step 6: colour-coded drawing ─────────────────────────────────────────────
void LambdaDetector::drawCorners(cv::Mat& canvas,
                                  const std::vector<cv::KeyPoint>& kps)
{
    for (const auto& kp : kps) {
        float s = kp.response;
        cv::Scalar color =
            (s < 85.0f)  ? cv::Scalar( 45, 155, 111) :   // sage green
            (s < 170.0f) ? cv::Scalar(207,  79,  91) :   // purple
                           cv::Scalar( 48,  90, 216);     // coral
        cv::circle(canvas,
                   cv::Point(cvRound(kp.pt.x), cvRound(kp.pt.y)),
                   4, color, 1, cv::LINE_AA);
    }
}