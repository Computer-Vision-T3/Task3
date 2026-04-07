// #pragma once
// #include <opencv2/opencv.hpp>
// #include <vector>

// // ── Drawing helpers shared across all backend modules ────────────────────────
// class DrawingUtils {
// public:
//     // Draw a single keypoint circle + optional orientation line
//     static void drawKeyPoint(cv::Mat& img,
//                               const cv::KeyPoint& kp,
//                               const cv::Scalar& color,
//                               int thickness = 1) {
//         int r = std::max(2, (int)(kp.size * 0.5f));
//         cv::circle(img, kp.pt, r, color, thickness, cv::LINE_AA);
//         if (kp.angle >= 0.0f) {
//             float rad = kp.angle * static_cast<float>(CV_PI) / 180.0f;
//             cv::Point2f ep(kp.pt.x + r * std::cos(rad),
//                            kp.pt.y + r * std::sin(rad));
//             cv::line(img, kp.pt, ep, color, 1, cv::LINE_AA);
//         }
//     }

//     // Side-by-side image concat for match visualization
//     static cv::Mat sideBySide(const cv::Mat& a, const cv::Mat& b) {
//         int h = std::max(a.rows, b.rows);
//         cv::Mat la, lb;
//         cv::copyMakeBorder(a, la, 0, h - a.rows, 0, 0, cv::BORDER_CONSTANT, {245,243,239});
//         cv::copyMakeBorder(b, lb, 0, h - b.rows, 0, 0, cv::BORDER_CONSTANT, {245,243,239});
//         cv::Mat result;
//         cv::hconcat(la, lb, result);
//         return result;
//     }

//     // Draw match lines between two side-by-side images
//     static void drawMatchLines(cv::Mat& canvas,
//                                 const std::vector<cv::KeyPoint>& kpsA,
//                                 const std::vector<cv::KeyPoint>& kpsB,
//                                 const std::vector<cv::DMatch>& matches,
//                                 int offsetX,       // width of image A
//                                 bool richStyle = true) {
//         for (size_t i = 0; i < matches.size(); ++i) {
//             const auto& m = matches[i];
//             cv::Point2f pa = kpsA[m.queryIdx].pt;
//             cv::Point2f pb = kpsB[m.trainIdx].pt;
//             pb.x += offsetX;

//             // Color cycles through purple / sage / coral per match index
//             cv::Scalar col;
//             switch (i % 3) {
//             case 0: col = cv::Scalar(207, 79, 91);  break; // purple BGR
//             case 1: col = cv::Scalar(111,155, 45);  break; // sage
//             default: col = cv::Scalar( 48, 90,216); break; // coral
//             }

//             if (richStyle) {
//                 cv::line(canvas, pa, pb, col, 1, cv::LINE_AA);
//                 cv::circle(canvas, pa, 4, col, -1, cv::LINE_AA);
//                 cv::circle(canvas, pb, 4, col, -1, cv::LINE_AA);
//             } else {
//                 cv::line(canvas, pa, pb, col, 1, cv::LINE_AA);
//             }
//         }
//     }
// };

#pragma once
#include <opencv2/opencv.hpp>
#include <vector>

class DrawingUtils {
public:
    static void drawKeyPoint(cv::Mat& img,
                             const cv::KeyPoint& kp,
                             const cv::Scalar& color,
                             int thickness = 1);

    static cv::Mat sideBySide(const cv::Mat& a, const cv::Mat& b);

    static void drawMatchLines(cv::Mat& canvas,
                               const std::vector<cv::KeyPoint>& kpsA,
                               const std::vector<cv::KeyPoint>& kpsB,
                               const std::vector<cv::DMatch>& matches,
                               int offsetX,
                               bool richStyle = true);
};