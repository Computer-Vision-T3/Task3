#ifndef SSDMATCHER_H
#define SSDMATCHER_H

#include <opencv2/opencv.hpp>
#include <vector>

class SSDMatcher {
public:
    /**
     * @return Pair of {Visualization Image, Vector of Matches}
     */
    static std::pair<cv::Mat, std::vector<cv::DMatch>> match(
        const cv::Mat& imgA, 
        const cv::Mat& imgB,
        int topK, 
        double ratio, 
        bool crossCheck, 
        int vizMode, 
        double& timingMs);

private:
    static float calculateSSD(const cv::Mat& desc1, const cv::Mat& desc2);
};

#endif
