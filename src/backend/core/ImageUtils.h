#pragma once
#include <opencv2/opencv.hpp>

// ── Shared image pre-processing helpers ──────────────────────────────────────
class ImageUtils {
public:
    // Safe grayscale conversion: returns clone if already 1-ch
    static cv::Mat toGray(const cv::Mat& src);

    // Ensure 8-bit, 3-channel BGR for drawing
    static cv::Mat toBGR8(const cv::Mat& src);

    // Normalize a float map to [0, 255] uint8
    static cv::Mat normalizeToU8(const cv::Mat& floatMap);

    // Draw keypoints with rich style (color + size + orientation)
    static void drawKeyPointsRich(cv::Mat& img,
                                  const std::vector<cv::KeyPoint>& kps);
};