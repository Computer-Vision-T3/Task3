#pragma once
#include <opencv2/opencv.hpp>
#include <vector>
#include <tuple>

class HarrisDetector {
public:
    // ── Main entry point (called by AppController) ────────────────────
    // Returns {result image, keypoints vector}; writes elapsed ms to timingMs.
    static std::tuple<cv::Mat, std::vector<cv::KeyPoint>>
    detect(const cv::Mat& src,
           double k,
           int blockSize,
           int apertureSize,
           int threshold,
           bool colorMode,
           double& timingMs);

private:
    // Step 1-3: Compute raw Harris response map R = det(M) - k*trace(M)^2
    static void computeResponse(const cv::Mat& gray,
                                 cv::Mat& dst,
                                 double k);

    // Step 4a: Threshold the normalised response map
    static cv::Mat applyThreshold(const cv::Mat& responseNorm, int thresh);

    // Step 4b: Non-Maximal Suppression in winSize x winSize neighbourhood
    static cv::Mat nonMaximalSuppression(const cv::Mat& response, int winSize = 3);

    // Step 5: Collect surviving pixels as cv::KeyPoint objects
    static std::vector<cv::KeyPoint> collectKeyPoints(const cv::Mat& nmsMap,
                                                       const cv::Mat& responseNorm);

    // Step 6: Draw detected corners on the output canvas
    static void drawCorners(cv::Mat& canvas,
                             const std::vector<cv::KeyPoint>& kps);
};