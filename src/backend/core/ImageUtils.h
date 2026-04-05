#pragma once
#include <opencv2/opencv.hpp>

// ── Shared image pre-processing helpers ──────────────────────────────────────
class ImageUtils {
public:
    // Safe grayscale conversion: returns clone if already 1-ch
    static cv::Mat toGray(const cv::Mat& src) {
        if (src.channels() == 1) return src.clone();
        cv::Mat gray;
        cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
        return gray;
    }

    // Ensure 8-bit, 3-channel BGR for drawing
    static cv::Mat toBGR8(const cv::Mat& src) {
        cv::Mat out;
        if (src.channels() == 1)
            cv::cvtColor(src, out, cv::COLOR_GRAY2BGR);
        else
            out = src.clone();
        if (out.depth() != CV_8U)
            out.convertTo(out, CV_8U, 255.0);
        return out;
    }

    // Normalize a float map to [0, 255] uint8
    static cv::Mat normalizeToU8(const cv::Mat& floatMap) {
        cv::Mat out;
        cv::normalize(floatMap, out, 0, 255, cv::NORM_MINMAX);
        out.convertTo(out, CV_8U);
        return out;
    }

    // Draw keypoints with rich style (color + size + orientation)
    static void drawKeyPointsRich(cv::Mat& img,
                                   const std::vector<cv::KeyPoint>& kps) {
        for (const auto& kp : kps) {
            float r = std::max(2.0f, kp.size * 0.5f);
            r = std::min(r, 15.0f);

            cv::Scalar fill, ring;
            if (kp.size < 10.0f) {
                fill = cv::Scalar(111, 155, 45);    // sage green
                ring = cv::Scalar( 70, 100, 20);
            } else if (kp.size < 25.0f) {
                fill = cv::Scalar(207, 79, 91);     // purple (BGR)
                ring = cv::Scalar(160, 50, 60);
            } else {
                fill = cv::Scalar( 48, 90, 216);    // coral (BGR)
                ring = cv::Scalar( 25, 60, 150);
            }

            cv::circle(img, kp.pt, (int)r, ring, 2, cv::LINE_AA);
            cv::circle(img, kp.pt, (int)r - 1, fill, -1, cv::LINE_AA);

            if (kp.angle >= 0.0f) {
                float rad = kp.angle * static_cast<float>(CV_PI) / 180.0f;
                cv::Point2f ep(kp.pt.x + r * std::cos(rad),
                               kp.pt.y + r * std::sin(rad));
                cv::line(img, kp.pt, ep, ring, 1, cv::LINE_AA);
            }
        }
    }
};