#include "SIFTDescriptor.h"
#include <opencv2/features2d.hpp>
#include <stdexcept>
#include <chrono>

// ---------------------------------------------------------------------------
// Helper: draw rich keypoint circles (centre + scale ring + orientation tick)
// ---------------------------------------------------------------------------
cv::Mat SIFTDescriptor::drawKeypoints(const cv::Mat& img,
                                      const std::vector<cv::KeyPoint>& kps)
{
    cv::Mat out;
    if (img.channels() == 1)
        cv::cvtColor(img, out, cv::COLOR_GRAY2BGR);
    else
        out = img.clone();

    for (const auto& kp : kps) {
        int   r   = std::max(1, static_cast<int>(kp.size / 2.0f));
        float ang = kp.angle * static_cast<float>(CV_PI) / 180.0f;
        cv::Point2f centre(kp.pt);
        cv::Point2f tip(centre.x + r * std::cos(ang),
                        centre.y + r * std::sin(ang));

        cv::circle   (out, centre, r,    cv::Scalar(0, 255, 0), 1, cv::LINE_AA);
        cv::line     (out, centre, tip,  cv::Scalar(0, 0, 255), 1, cv::LINE_AA);
        cv::circle   (out, centre, 2,    cv::Scalar(255, 0, 0), -1);
    }
    return out;
}

// ---------------------------------------------------------------------------
// Core implementation
// ---------------------------------------------------------------------------
std::tuple<cv::Mat, std::vector<cv::KeyPoint>, cv::Mat>
SIFTDescriptor::describe(const cv::Mat& src,
                         int    nFeat,
                         int    nOctave,
                         double contrast,
                         double edge,
                         double sigma,
                         bool   colorMode,
                         double& timingMs)
{
    if (src.empty())
        throw std::invalid_argument("SIFTDescriptor::describe – input image is empty.");

    // -----------------------------------------------------------------------
    // Start timer
    // -----------------------------------------------------------------------
    auto t0 = std::chrono::high_resolution_clock::now();

    std::vector<cv::KeyPoint> keypoints;
    cv::Mat descriptors;

    // -----------------------------------------------------------------------
    // Build SIFT detector/descriptor
    // -----------------------------------------------------------------------
#if CV_VERSION_MAJOR >= 4 && CV_VERSION_MINOR >= 4
    // OpenCV 4.4+ exposes SIFT in the main module
    auto sift = cv::SIFT::create(nFeat, nOctave,
                                 static_cast<float>(contrast),
                                 static_cast<float>(edge),
                                 static_cast<float>(sigma));
#else
    auto sift = cv::xfeatures2d::SIFT::create(nFeat, nOctave,
                                              static_cast<float>(contrast),
                                              static_cast<float>(edge),
                                              static_cast<float>(sigma));
#endif

    if (!colorMode) {
        // ------------------------------------------------------------------
        // GRAYSCALE mode – standard SIFT pipeline
        // ------------------------------------------------------------------
        cv::Mat gray;
        if (src.channels() == 3)
            cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
        else
            gray = src.clone();

        sift->detectAndCompute(gray, cv::noArray(), keypoints, descriptors);

    } else {
        // ------------------------------------------------------------------
        // COLOR mode – Opponent-colour SIFT
        //   Split into B, G, R channels, run SIFT on each, then
        //   detect keypoints on the luminance channel and describe using
        //   the concatenated per-channel descriptors.
        //   Result descriptor length = 3 × 128 = 384.
        // ------------------------------------------------------------------
        cv::Mat gray;
        if (src.channels() == 3)
            cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
        else
            gray = src.clone();

        // Step 1: detect keypoints on grayscale
        sift->detect(gray, keypoints);

        if (keypoints.empty()) {
            timingMs = 0.0;
            return { drawKeypoints(src, keypoints), keypoints, cv::Mat() };
        }

        // Step 2: compute descriptors channel-by-channel
        std::vector<cv::Mat> channels;
        if (src.channels() == 3) {
            cv::split(src, channels);            // B, G, R
        } else {
            // Single channel – just replicate three times
            channels = { gray.clone(), gray.clone(), gray.clone() };
        }

        cv::Mat descAll;
        for (auto& ch : channels) {
            cv::Mat chDesc;
            std::vector<cv::KeyPoint> kpsCopy = keypoints; // compute() may filter
            sift->compute(ch, kpsCopy, chDesc);

            // Align rows: if compute() pruned some keypoints we keep as-is
            // (we used the same keypoint set for every channel so counts match)
            if (descAll.empty())
                descAll = chDesc;
            else
                cv::hconcat(descAll, chDesc, descAll);  // 128 → 256 → 384
        }
        descriptors = descAll;
    }

    // -----------------------------------------------------------------------
    // Stop timer
    // -----------------------------------------------------------------------
    auto t1 = std::chrono::high_resolution_clock::now();
    timingMs  = std::chrono::duration<double, std::milli>(t1 - t0).count();

    // -----------------------------------------------------------------------
    // Annotate result image
    // -----------------------------------------------------------------------
    cv::Mat annotated = drawKeypoints(src, keypoints);

    return { annotated, keypoints, descriptors };
}
