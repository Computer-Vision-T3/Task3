#include "NCCMatcher.h"

#include <chrono>
#include <algorithm>
#include <stdexcept>
#include <iostream>

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────
NCCMatcher::NCCMatcher(float threshold, bool crossCheck)
    : threshold_(threshold), crossCheck_(crossCheck)
{
    if (threshold_ < -1.0f || threshold_ > 1.0f)
        throw std::invalid_argument("NCCMatcher: threshold must be in [-1, 1].");
}

// ─────────────────────────────────────────────────────────────────────────────
// Private helpers
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Compute NCC between descriptor row idx1 (from desc1) and
 * descriptor row idx2 (from desc2).
 *
 * OpenCV's SIFT already L2-normalises each descriptor, so
 *   NCC = dot(d1, d2) / (||d1|| * ||d2||)  =  dot(d1_unit, d2_unit)
 *
 * For raw / un-normalised descriptors we normalise on the fly.
 */
float NCCMatcher::computeNCC(const cv::Mat& desc1, int idx1,
                              const cv::Mat& desc2, int idx2)
{
    const float* d1 = desc1.ptr<float>(idx1);
    const float* d2 = desc2.ptr<float>(idx2);
    int len = desc1.cols;

    double dot  = 0.0, norm1 = 0.0, norm2 = 0.0;
    for (int k = 0; k < len; ++k) {
        dot   += static_cast<double>(d1[k]) * d2[k];
        norm1 += static_cast<double>(d1[k]) * d1[k];
        norm2 += static_cast<double>(d2[k]) * d2[k];
    }

    double denom = std::sqrt(norm1) * std::sqrt(norm2);
    if (denom < 1e-10) return 0.0f;          // degenerate descriptor

    return static_cast<float>(dot / denom);
}

/**
 * For every row i in src find the index j in dst that maximises NCC(src[i], dst[j]).
 * Stores the best NCC scores in bestScores.
 */
std::vector<int> NCCMatcher::buildBestMatchIndex(const cv::Mat& src,
                                                  const cv::Mat& dst,
                                                  std::vector<float>& bestScores)
{
    int N = src.rows;
    int M = dst.rows;
    std::vector<int>   bestIdx(N, -1);
    bestScores.assign(N, -2.0f);          // below any valid NCC

    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < M; ++j) {
            float score = computeNCC(src, i, dst, j);
            if (score > bestScores[i]) {
                bestScores[i] = score;
                bestIdx[i]    = j;
            }
        }
    }
    return bestIdx;
}

// ─────────────────────────────────────────────────────────────────────────────
// Public: match (single pair)
// ─────────────────────────────────────────────────────────────────────────────
double NCCMatcher::match(const cv::Mat& queryDescriptors,
                         const cv::Mat& trainDescriptors,
                         std::vector<NCCMatch>& matches)
{
    matches.clear();

    if (queryDescriptors.empty() || trainDescriptors.empty()) {
        std::cerr << "[NCCMatcher] Warning: empty descriptor matrix received.\n";
        return 0.0;
    }
    if (queryDescriptors.type() != CV_32F || trainDescriptors.type() != CV_32F) {
        throw std::invalid_argument("[NCCMatcher] Descriptors must be CV_32F.");
    }
    if (queryDescriptors.cols != trainDescriptors.cols) {
        throw std::invalid_argument("[NCCMatcher] Descriptor dimension mismatch.");
    }

    auto t0 = std::chrono::high_resolution_clock::now();

    // Forward pass: for each query descriptor find best train descriptor
    std::vector<float> fwdScores;
    std::vector<int>   fwdIdx = buildBestMatchIndex(queryDescriptors,
                                                    trainDescriptors,
                                                    fwdScores);

    if (crossCheck_) {
        // Backward pass: for each train descriptor find best query descriptor
        std::vector<float> bwdScores;
        std::vector<int>   bwdIdx = buildBestMatchIndex(trainDescriptors,
                                                        queryDescriptors,
                                                        bwdScores);

        for (int i = 0; i < queryDescriptors.rows; ++i) {
            int j = fwdIdx[i];
            if (j < 0) continue;
            // Cross-check: the best match of train[j] must be query[i]
            if (bwdIdx[j] == i && fwdScores[i] >= threshold_) {
                NCCMatch m;
                m.queryIdx = i;
                m.trainIdx = j;
                m.nccScore = fwdScores[i];
                m.distance = 1.0f - fwdScores[i];
                matches.push_back(m);
            }
        }
    } else {
        for (int i = 0; i < queryDescriptors.rows; ++i) {
            if (fwdIdx[i] < 0) continue;
            if (fwdScores[i] >= threshold_) {
                NCCMatch m;
                m.queryIdx = i;
                m.trainIdx = fwdIdx[i];
                m.nccScore = fwdScores[i];
                m.distance = 1.0f - fwdScores[i];
                matches.push_back(m);
            }
        }
    }

    // Sort by NCC score descending (best first)
    std::sort(matches.begin(), matches.end(),
              [](const NCCMatch& a, const NCCMatch& b) {
                  return a.nccScore > b.nccScore;
              });

    auto t1 = std::chrono::high_resolution_clock::now();
    double elapsed = std::chrono::duration<double, std::milli>(t1 - t0).count();

    std::cout << "[NCCMatcher] Matches found: " << matches.size()
              << "  |  Time: " << elapsed << " ms\n";

    return elapsed;
}

// ─────────────────────────────────────────────────────────────────────────────
// Public: matchMultiple (one query vs. many train images)
// ─────────────────────────────────────────────────────────────────────────────
double NCCMatcher::matchMultiple(const cv::Mat& queryDescriptors,
                                  const std::vector<cv::Mat>& trainDescriptorSet,
                                  std::vector<std::vector<NCCMatch>>& allMatches)
{
    allMatches.clear();
    allMatches.resize(trainDescriptorSet.size());

    double totalTime = 0.0;
    for (size_t i = 0; i < trainDescriptorSet.size(); ++i) {
        std::cout << "[NCCMatcher] Matching against train image " << i << " ...\n";
        totalTime += match(queryDescriptors, trainDescriptorSet[i], allMatches[i]);
    }

    std::cout << "[NCCMatcher] Total matching time: " << totalTime << " ms\n";
    return totalTime;
}

// ─────────────────────────────────────────────────────────────────────────────
// Public static: drawMatches
// ─────────────────────────────────────────────────────────────────────────────
cv::Mat NCCMatcher::drawMatches(const cv::Mat& img1,
                                 const std::vector<cv::KeyPoint>& kp1,
                                 const cv::Mat& img2,
                                 const std::vector<cv::KeyPoint>& kp2,
                                 const std::vector<NCCMatch>& matches,
                                 int maxDraw)
{
    // Convert NCCMatch → cv::DMatch for OpenCV drawing
    std::vector<cv::DMatch> cvMatches;
    int limit = (maxDraw > 0 && maxDraw < static_cast<int>(matches.size()))
                ? maxDraw
                : static_cast<int>(matches.size());

    cvMatches.reserve(limit);
    for (int i = 0; i < limit; ++i) {
        cvMatches.emplace_back(matches[i].queryIdx,
                               matches[i].trainIdx,
                               matches[i].distance);
    }

    cv::Mat result;
    cv::drawMatches(img1, kp1, img2, kp2, cvMatches, result,
                    cv::Scalar::all(-1), cv::Scalar::all(-1),
                    std::vector<char>(),
                    cv::DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);
    return result;
}