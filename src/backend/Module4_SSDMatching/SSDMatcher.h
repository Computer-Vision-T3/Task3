#pragma once
#include <opencv2/opencv.hpp>
#include <vector>
#include <tuple>

class SSDMatcher {
public:
    static std::tuple<cv::Mat, std::vector<cv::DMatch>>
    match(const cv::Mat& imgA, const cv::Mat& imgB, int topK, double ratio, bool crossCheck, int vizMode, double& timingMs) {
        return {imgA.clone(), {}};
    }
};