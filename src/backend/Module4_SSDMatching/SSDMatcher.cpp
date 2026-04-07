#include "SSDMatcher.h"
#include "../Module3_SIFTDescriptor/SIFTDescriptor.h"
#include <chrono>
#include <limits>
#include <algorithm>

std::pair<cv::Mat, std::vector<cv::DMatch>> SSDMatcher::match(
    const cv::Mat& imgA, const cv::Mat& imgB,
    int topK, double ratio, bool crossCheck, int vizMode, double& timingMs) 
{
    // 1. Extract SIFT features (Assuming returns: tuple<Mat, vector<KeyPoint>, Mat>)
    double tA = 0, tB = 0;
    auto [vA, kpA, descA] = SIFTDescriptor::describe(imgA, 0, 3, 0.04, 10.0, 1.6, false, tA);
    auto [vB, kpB, descB] = SIFTDescriptor::describe(imgB, 0, 3, 0.04, 10.0, 1.6, false, tB);

    if (descA.empty() || descB.empty()) return {cv::Mat(), {}};

    auto start = std::chrono::high_resolution_clock::now();
    std::vector<cv::DMatch> matches;

    // 2. SSD Brute Force with Lowe's Ratio Test
    for (int i = 0; i < descA.rows; ++i) {
        float d1 = std::numeric_limits<float>::max();
        float d2 = std::numeric_limits<float>::max();
        int bestIdx = -1;

        for (int j = 0; j < descB.rows; ++j) {
            float dist = calculateSSD(descA.row(i), descB.row(j));
            if (dist < d1) {
                d2 = d1;
                d1 = dist;
                bestIdx = j;
            } else if (dist < d2) {
                d2 = dist;
            }
        }

        if (bestIdx != -1 && (d1 < ratio * d2)) {
            matches.push_back(cv::DMatch(i, bestIdx, d1));
        }
    }

    // 3. Cross-Check
    if (crossCheck && !matches.empty()) {
        std::vector<cv::DMatch> finalMatches;
        for (const auto& m : matches) {
            float minDist = std::numeric_limits<float>::max();
            int bestIdxBack = -1;
            for (int k = 0; k < descA.rows; ++k) {
                float d = calculateSSD(descB.row(m.trainIdx), descA.row(k));
                if (d < minDist) { minDist = d; bestIdxBack = k; }
            }
            if (bestIdxBack == m.queryIdx) finalMatches.push_back(m);
        }
        matches = finalMatches;
    }

    // 4. Sort and Top-K
    std::sort(matches.begin(), matches.end(), [](const cv::DMatch& a, const cv::DMatch& b){
        return a.distance < b.distance;
    });
    if (topK > 0 && (int)matches.size() > topK) matches.resize(topK);

    auto end = std::chrono::high_resolution_clock::now();
    timingMs = std::chrono::duration<double, std::milli>(end - start).count();

    // 5. Drawing (FIXED FLAGS TYPE)
    cv::Mat result;
    cv::DrawMatchesFlags flags = cv::DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS;
    
    cv::drawMatches(imgA, kpA, imgB, kpB, matches, result, 
                    cv::Scalar::all(-1), cv::Scalar::all(-1), 
                    std::vector<char>(), flags);

    return {result, matches};
}

float SSDMatcher::calculateSSD(const cv::Mat& desc1, const cv::Mat& desc2) {
    float sum = 0;
    // Fast pointer access for float descriptors
    const float* p1 = desc1.ptr<float>();
    const float* p2 = desc2.ptr<float>();
    for (int i = 0; i < desc1.cols; ++i) {
        float diff = p1[i] - p2[i];
        sum += diff * diff;
    }
    return sum;
}
