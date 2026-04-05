#include "HarrisDetector.h"
#include "../core/ImageUtils.h"
#include <chrono>
#include <iostream>

// ── Main entry point ──────────────────────────────────────────────────────────
std::tuple<cv::Mat, std::vector<cv::KeyPoint>>
HarrisDetector::detect(const cv::Mat& src,
                        double k,
                        int /*blockSize*/,
                        int /*apertureSize*/,
                        int threshold,
                        bool colorMode,
                        double& timingMs)
{
    cv::Mat gray = ImageUtils::toGray(src);

    // Steps 1-3: compute Harris response (timed)
    cv::Mat responseRaw;
    auto t0 = std::chrono::high_resolution_clock::now();
    computeResponse(gray, responseRaw, k);
    auto t1 = std::chrono::high_resolution_clock::now();
    timingMs = std::chrono::duration<double, std::milli>(t1 - t0).count();

    std::cout << "[Harris]  Computation time : " << timingMs << " ms\n";

    // Step 4a: normalize then threshold
    cv::Mat responseNorm;
    cv::normalize(responseRaw, responseNorm, 0, 255, cv::NORM_MINMAX, CV_32FC1);
    cv::Mat thresholded = applyThreshold(responseNorm, threshold);

    // Step 4b: NMS
    cv::Mat nmsMap = nonMaximalSuppression(thresholded, 3);

    // Step 5: keypoints
    std::vector<cv::KeyPoint> keypoints = collectKeyPoints(nmsMap, responseNorm);
    std::cout << "[Harris]  Corners after NMS: " << keypoints.size() << "\n";

    // Step 6: draw
    cv::Mat canvas = colorMode ? src.clone() : ImageUtils::toBGR8(gray);
    drawCorners(canvas, keypoints);

    return { canvas, keypoints };
}

// ── Steps 1-3: Harris response R = det(M) - k * trace²(M) ───────────────────
void HarrisDetector::computeResponse(const cv::Mat& gray,
                                      cv::Mat& dst,
                                      double k)
{
    // Step 1: gradients
    cv::Mat Ix, Iy;
    cv::Sobel(gray, Ix, CV_32FC1, 1, 0, 3, 1, 0, cv::BORDER_DEFAULT);
    cv::Sobel(gray, Iy, CV_32FC1, 0, 1, 3, 1, 0, cv::BORDER_DEFAULT);

    // Step 2: second-moment matrix elements
    cv::Mat Ix2, Iy2, IxIy;
    cv::pow(Ix, 2.0, Ix2);
    cv::pow(Iy, 2.0, Iy2);
    cv::multiply(Ix, Iy, IxIy);

    // Step 3: Gaussian-weighted neighbourhood integration
    cv::Mat a, c, b;
    cv::GaussianBlur(Ix2,  a, cv::Size(7,7), 2.0, 2.0, cv::BORDER_DEFAULT);
    cv::GaussianBlur(Iy2,  c, cv::Size(7,7), 2.0, 2.0, cv::BORDER_DEFAULT);
    cv::GaussianBlur(IxIy, b, cv::Size(7,7), 2.0, 2.0, cv::BORDER_DEFAULT);

    // R = det(M) - k * trace²(M)
    //   det   = a*c - b²
    //   trace² = (a+c)²
    cv::Mat detM, b2, trace2;
    cv::multiply(a, c, detM);
    cv::multiply(b, b, b2);
    detM = detM - b2;
    cv::pow(a + c, 2.0, trace2);
    dst = detM - static_cast<float>(k) * trace2;
}

// ── Step 4a: threshold normalised response ────────────────────────────────────
cv::Mat HarrisDetector::applyThreshold(const cv::Mat& responseNorm, int thresh) {
    cv::Mat out = cv::Mat::zeros(responseNorm.size(), CV_32FC1);
    for (int i = 0; i < responseNorm.rows; ++i)
        for (int j = 0; j < responseNorm.cols; ++j)
            if (static_cast<int>(responseNorm.at<float>(i, j)) > thresh)
                out.at<float>(i, j) = responseNorm.at<float>(i, j);
    return out;
}

// ── Step 4b: Non-Maximal Suppression ─────────────────────────────────────────
cv::Mat HarrisDetector::nonMaximalSuppression(const cv::Mat& response,
                                               int winSize) {
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

// ── Step 5: collect KeyPoints ─────────────────────────────────────────────────
std::vector<cv::KeyPoint>
HarrisDetector::collectKeyPoints(const cv::Mat& nmsMap,
                                  const cv::Mat& responseNorm) {
    std::vector<cv::KeyPoint> kps;
    for (int i = 0; i < nmsMap.rows; ++i) {
        for (int j = 0; j < nmsMap.cols; ++j) {
            if (nmsMap.at<float>(i, j) > 0.0f) {
                float strength = responseNorm.at<float>(i, j);
                float size = strength * 30.0f / 255.0f;
                kps.emplace_back(cv::Point2f((float)j, (float)i),
                                 size, -1.0f, strength);
            }
        }
    }
    return kps;
}

// ── Step 6: draw colour-coded corners ────────────────────────────────────────
void HarrisDetector::drawCorners(cv::Mat& canvas,
                                  const std::vector<cv::KeyPoint>& kps) {
    for (const auto& kp : kps) {
        float s = kp.response;
        cv::Scalar color =
            (s < 85.0f)  ? cv::Scalar( 45, 155, 111) :  // sage green
            (s < 170.0f) ? cv::Scalar(207,  79,  91) :  // purple
                           cv::Scalar( 48,  90, 216);    // coral
        cv::circle(canvas,
                   cv::Point(cvRound(kp.pt.x), cvRound(kp.pt.y)),
                   4, color, 1, cv::LINE_AA);
    }
}