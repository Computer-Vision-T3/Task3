#pragma once

#include <opencv2/opencv.hpp>
#include <vector>
#include <string>

/**
 * NCCMatcher - Module 5: Normalized Cross-Correlation Feature Matching
 *
 * Matches SIFT keypoint descriptors between images using the
 * Normalized Cross-Correlation (NCC) similarity measure.
 * NCC values range from -1 (inverse match) to +1 (perfect match).
 */

struct NCCMatch
{
    int queryIdx;   // Index in query descriptors
    int trainIdx;   // Index in train descriptors
    float nccScore; // NCC score in [-1, 1]; higher is better
    float distance; // 1 - nccScore  (for compatibility with cv::DMatch style)
};

class NCCMatcher
{
public:
    /**
     * @param threshold  Minimum NCC score to accept a match (default 0.8).
     *                   Range: (-1, 1].
     * @param crossCheck If true, only keep mutually-best matches (recommended).
     */
    explicit NCCMatcher(float threshold = 0.8f, bool crossCheck = true);

    /**
     * Match descriptors from a single query image against a single train image.
     *
     * @param queryDescriptors  Descriptor matrix from the query image  (N x 128, CV_32F)
     * @param trainDescriptors  Descriptor matrix from the train image  (M x 128, CV_32F)
     * @param matches           Output vector of NCCMatch structs
     * @return                  Elapsed time in milliseconds
     */
    double match(const cv::Mat &queryDescriptors,
                 const cv::Mat &trainDescriptors,
                 std::vector<NCCMatch> &matches);

    /**
     * Match descriptors from one query image against multiple train images.
     *
     * @param queryDescriptors   Descriptor matrix for the query image
     * @param trainDescriptorSet Vector of descriptor matrices (one per train image)
     * @param allMatches         Output: one vector of NCCMatch per train image
     * @return                   Total elapsed time in milliseconds
     */
    double matchMultiple(const cv::Mat &queryDescriptors,
                         const std::vector<cv::Mat> &trainDescriptorSet,
                         std::vector<std::vector<NCCMatch>> &allMatches);

    /**
     * Draw matches between two images and return the result image.
     *
     * @param img1     Query image
     * @param kp1      Keypoints from query image
     * @param img2     Train image
     * @param kp2      Keypoints from train image
     * @param matches  Matches produced by match()
     * @param maxDraw  Maximum number of matches to draw (0 = all)
     */
    static cv::Mat drawMatches(const cv::Mat &img1,
                               const std::vector<cv::KeyPoint> &kp1,
                               const cv::Mat &img2,
                               const std::vector<cv::KeyPoint> &kp2,
                               const std::vector<NCCMatch> &matches,
                               int maxDraw = 50);

    // Getters / Setters
    float getThreshold() const { return threshold_; }
    bool getCrossCheck() const { return crossCheck_; }
    void setThreshold(float t) { threshold_ = t; }
    void setCrossCheck(bool cc) { crossCheck_ = cc; }

private:
    float threshold_;
    bool crossCheck_;

    /**
     * Compute NCC score between two L2-normalised descriptor vectors.
     * Because SIFT descriptors are already L2-normalised by OpenCV,
     * NCC = dot product of the two unit vectors.
     */
    static float computeNCC(const cv::Mat &desc1, int idx1,
                            const cv::Mat &desc2, int idx2);

    /**
     * Build best-match lookup: for every row in src, find the best row in dst.
     * Returns a vector of size src.rows where result[i] = best index in dst.
     */
    static std::vector<int> buildBestMatchIndex(const cv::Mat &src,
                                                const cv::Mat &dst,
                                                std::vector<float> &bestScores);
};