#pragma once
#include <opencv2/opencv.hpp>
#include <vector>
#include <tuple>

class HarrisDetector {
public:
    static std::tuple<cv::Mat, std::vector<cv::KeyPoint>>
    detect(const cv::Mat& src,
           double k,
           int blockSize,
           int apertureSize,
           int threshold,
           bool colorMode,
           double& timingMs);

private:
    static void computeResponse(const cv::Mat& gray,
                                 cv::Mat& dst,
                                 double k,
                                 int apertureSize);

    static cv::Mat applyThreshold(const cv::Mat& responseNorm, int thresh);
    
    // Optimized NMS
    static cv::Mat nonMaximalSuppression(const cv::Mat& response, int winSize = 3);
    
    static std::vector<cv::KeyPoint> collectKeyPoints(const cv::Mat& nmsMap,
                                                       const cv::Mat& responseNorm);
    static void drawCorners(cv::Mat& canvas,
                             const std::vector<cv::KeyPoint>& kps);

    // ── OPTIMIZATION: 1D Separable Math Primitives ────────────────────
    static cv::Mat apply1DConvolutionX(const cv::Mat& src, const std::vector<float>& kernel);
    static cv::Mat apply1DConvolutionY(const cv::Mat& src, const std::vector<float>& kernel);
    static std::vector<float> create1DGaussianKernel(int size, float sigma);
};