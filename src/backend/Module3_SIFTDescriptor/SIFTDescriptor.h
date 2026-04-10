#pragma once
#include <opencv2/opencv.hpp>
#include <vector>
#include <tuple>

/**
 * SIFT implementation built entirely from scratch.
 * Uses only basic OpenCV primitives (GaussianBlur, resize, etc.).
 * No cv::SIFT, cv::xfeatures2d::SIFT, or any feature-detector API.
 *
 * Pipeline:
 *  1. Build Gaussian scale-space pyramid
 *  2. Build Difference-of-Gaussians (DoG) pyramid
 *  3. Detect extrema in DoG (scale-space local max/min)
 *  4. Sub-pixel keypoint localisation + filtering (contrast + edge)
 *  5. Orientation assignment (gradient histogram per keypoint)
 *  6. 4×4 spatial histogram descriptor (128 dimensions)
 *
 * Color mode: detect on luminance, describe each BGR channel separately,
 * concatenate → 384-dim descriptor (Opponent-SIFT style).
 */
class SIFTDescriptor {
public:
    // -----------------------------------------------------------------------
    // Public API  (matches the original interface)
    // -----------------------------------------------------------------------
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
    // -----------------------------------------------------------------------
    // Scale-space types
    // -----------------------------------------------------------------------
    // One octave = S+3 Gaussian images and S+2 DoG images (S = nOctave layers)
    using Pyramid = std::vector<std::vector<cv::Mat>>;   // [octave][layer]

    // -----------------------------------------------------------------------
    // Internal helpers
    // -----------------------------------------------------------------------

    /** Build Gaussian pyramid: nOctaves octaves, each with (nLayers+3) images */
    static Pyramid buildGaussianPyramid(const cv::Mat& base,
                                        int nOctaves,
                                        int nLayers,
                                        double sigma0);

    /** Build DoG pyramid from Gaussian pyramid */
    static Pyramid buildDoGPyramid(const Pyramid& gauss);

    /** Detect scale-space extrema across all DoG octaves */
    static std::vector<cv::KeyPoint>
    detectExtrema(const Pyramid& dog,
                  int    nLayers,
                  double contrastThresh,
                  double edgeThresh,
                  double sigma0,
                  int    imageRows,   // original image size (for coord scaling)
                  int    imageCols);

    /** Localise a single extremum candidate; returns false if rejected */
    static bool localiseExtrema(const Pyramid& dog,
                                int oct, int layer, int r, int c,
                                int nLayers,
                                double contrastThresh,
                                double edgeThresh,
                                double sigma0,
                                cv::KeyPoint& kp);

    /** Assign up to 2 dominant orientations; may duplicate keypoints */
    static std::vector<cv::KeyPoint>
    assignOrientations(const std::vector<cv::KeyPoint>& kps,
                       const Pyramid& gauss,
                       int nLayers);

    /** Compute a 128-dim descriptor for each keypoint on a given image */
    static cv::Mat computeDescriptors(const std::vector<cv::KeyPoint>& kps,
                                      const Pyramid& gauss,
                                      int nLayers);

    /** Drop nFeat weakest keypoints by response; 0 = keep all */
    static std::vector<cv::KeyPoint>
    filterByResponse(const std::vector<cv::KeyPoint>& kps, int nFeat);

    /** Draw keypoints onto an image copy */
    static cv::Mat drawKeypoints(const cv::Mat& img,
                                 const std::vector<cv::KeyPoint>& kps);

    // -----------------------------------------------------------------------
    // Tiny math utilities
    // -----------------------------------------------------------------------
    static float at(const cv::Mat& m, int r, int c) {
        return m.at<float>(r, c);
    }
    static float clampedAt(const cv::Mat& m, int r, int c) {
        r = std::max(0, std::min(r, m.rows - 1));
        c = std::max(0, std::min(c, m.cols - 1));
        return m.at<float>(r, c);
    }
};