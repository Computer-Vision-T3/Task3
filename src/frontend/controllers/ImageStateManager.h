#ifndef IMAGESTATEMANAGER_H
#define IMAGESTATEMANAGER_H

#include <opencv2/opencv.hpp>

// Holds the two input images and the last output.
// Matching modules (4, 5) need both A and B.
class ImageStateManager {
public:
    ImageStateManager() = default;

    void setImageA(const cv::Mat& img);
    void setImageB(const cv::Mat& img);
    void setOutput(const cv::Mat& img);

    cv::Mat getImageA() const;
    cv::Mat getImageB() const;
    cv::Mat getOutput()  const;

    bool hasImageA() const;
    bool hasImageB() const;

    void clearOutput();
    void clearAll();

private:
    cv::Mat m_imgA;
    cv::Mat m_imgB;
    cv::Mat m_output;
};

#endif // IMAGESTATEMANAGER_H