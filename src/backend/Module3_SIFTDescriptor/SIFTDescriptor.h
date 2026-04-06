#pragma once
#include <opencv2/opencv.hpp>
#include <vector>
#include <tuple>

class SIFTDescriptor {
public:
    static std::tuple<cv::Mat, std::vector<cv::KeyPoint>, cv::Mat>
    describe(const cv::Mat& src, int nFeat, int nOctave, double contrast, double edge, double sigma, bool colorMode, double& timingMs) {
        // Temporary stub so the compiler doesn't crash
        return {src.clone(), {}, cv::Mat()};
    }
};