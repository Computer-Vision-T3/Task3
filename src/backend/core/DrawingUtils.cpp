#include "DrawingUtils.h"
#include <cmath>

void DrawingUtils::drawKeyPoint(cv::Mat& img,
                                 const cv::KeyPoint& kp,
                                 const cv::Scalar& color,
                                 int thickness) {
    int r = std::max(2, (int)(kp.size * 0.5f));
    cv::circle(img, kp.pt, r, color, thickness, cv::LINE_AA);
    if (kp.angle >= 0.0f) {
        float rad = kp.angle * static_cast<float>(CV_PI) / 180.0f;
        cv::Point2f ep(kp.pt.x + r * std::cos(rad),
                       kp.pt.y + r * std::sin(rad));
        cv::line(img, kp.pt, ep, color, 1, cv::LINE_AA);
    }
}

cv::Mat DrawingUtils::sideBySide(const cv::Mat& a, const cv::Mat& b) {
    int h = std::max(a.rows, b.rows);
    cv::Mat la, lb;
    cv::copyMakeBorder(a, la, 0, h - a.rows, 0, 0,
                       cv::BORDER_CONSTANT, cv::Scalar(245, 243, 239));
    cv::copyMakeBorder(b, lb, 0, h - b.rows, 0, 0,
                       cv::BORDER_CONSTANT, cv::Scalar(245, 243, 239));
    cv::Mat result;
    cv::hconcat(la, lb, result);
    return result;
}

void DrawingUtils::drawMatchLines(cv::Mat& canvas,
                                   const std::vector<cv::KeyPoint>& kpsA,
                                   const std::vector<cv::KeyPoint>& kpsB,
                                   const std::vector<cv::DMatch>& matches,
                                   int offsetX,
                                   bool richStyle) {
    for (size_t i = 0; i < matches.size(); ++i) {
        const auto& m = matches[i];
        cv::Point2f pa = kpsA[m.queryIdx].pt;
        cv::Point2f pb = kpsB[m.trainIdx].pt;
        pb.x += offsetX;

        cv::Scalar col;
        switch (i % 3) {
        case 0: col = cv::Scalar(207,  79,  91); break; // purple
        case 1: col = cv::Scalar(111, 155,  45); break; // sage
        default:col = cv::Scalar( 48,  90, 216); break; // coral
        }

        if (richStyle) {
            cv::line(canvas, pa, pb, col, 1, cv::LINE_AA);
            cv::circle(canvas, pa, 4, col, -1, cv::LINE_AA);
            cv::circle(canvas, pb, 4, col, -1, cv::LINE_AA);
        } else {
            cv::line(canvas, pa, pb, col, 1, cv::LINE_AA);
        }
    }
}