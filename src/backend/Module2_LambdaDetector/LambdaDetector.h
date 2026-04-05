#pragma once
#include <opencv2/opencv.hpp>
#include <vector>
#include <tuple>

class LambdaDetector {
public:
    // ── Main entry point (called by AppController) ────────────────────
    // Returns {result image, keypoints vector}; writes elapsed ms to timingMs.
    static std::tuple<cv::Mat, std::vector<cv::KeyPoint>>
    detect(const cv::Mat& src,
           int maxKeypoints,
           double qualityLevel,
           int minDistance,
           int blockSize,
           bool colorMode,
           double& timingMs);

private:
    // Step 1-2: Compute λ_min map (smaller eigenvalue of M at every pixel)
    static cv::Mat computeLambdaMin(const cv::Mat& gray, int blockSize);

    // Step 3: Threshold λ_min by qualityLevel * global_maximum
    static cv::Mat applyQualityThreshold(const cv::Mat& lambdaMap,
                                          double qualityLevel);

    // Step 4: Non-Maximal Suppression enforcing minDistance between survivors
    static cv::Mat nonMaximalSuppression(const cv::Mat& response,
                                          int winSize = 3);

    // Step 5: Collect surviving pixels as cv::KeyPoint objects,
    //         sorted by strength, capped at maxKeypoints,
    //         with minDistance spacing enforced.
    static std::vector<cv::KeyPoint> collectKeyPoints(
        const cv::Mat& nmsMap,
        const cv::Mat& lambdaNorm,
        int maxKeypoints,
        int minDistance);

    // Step 6: Draw detected corners on the canvas
    static void drawCorners(cv::Mat& canvas,
                             const std::vector<cv::KeyPoint>& kps);
};