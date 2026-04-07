#pragma once
#include <opencv2/opencv.hpp>
#include <cmath>

class MathUtils {
public:
    static double euclideanDistance(cv::Point2f p1, cv::Point2f p2);

    static double euclideanDistance(double x1, double y1, double x2, double y2);

    // SSD between two float vectors of the same length
    static double ssd(const float* a, const float* b, int len);

    // NCC between two float vectors (zero-mean normalized)
    static double ncc(const float* a, const float* b, int len);

    static double degreesToRadians(double d);
    static double radiansToDegrees(double r);

    static int clamp(int v, int lo, int hi);
    static double clampd(double v, double lo, double hi);
};