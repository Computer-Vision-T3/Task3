#pragma once
#include <opencv2/opencv.hpp>
#include <vector>
#include <tuple>

class SIFTDescriptor {
public:
    /**
     * Detect keypoints and compute SIFT descriptors.
     *
     * @param src        Input image (grayscale or color BGR).
     * @param nFeat      Max number of features to retain (0 = unlimited).
     * @param nOctave    Number of octave layers in each octave.
     * @param contrast   Contrast threshold for filtering weak keypoints.
     * @param edge       Edge threshold for filtering edge-like keypoints.
     * @param sigma      Gaussian sigma for the first octave.
     * @param colorMode  If true, process each BGR channel separately and
     *                   concatenate descriptors (OpponentSIFT-style).
     * @param timingMs   Output: wall-clock time taken in milliseconds.
     *
     * @return {annotated image, keypoints, descriptors (rows = keypoints)}
     */
    static std::tuple<cv::Mat, std::vector<cv::KeyPoint>, cv::Mat>
    describe(const cv::Mat& src,
             int    nFeat,
             int    nOctave,
             double contrast,
             double edge,
             double sigma,
             bool   colorMode,
             double& timingMs);

private:
    // Draw keypoints with scale circles onto a copy of the image
    static cv::Mat drawKeypoints(const cv::Mat& img,
                                 const std::vector<cv::KeyPoint>& kps);
};
