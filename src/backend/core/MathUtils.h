#pragma once
#include <opencv2/opencv.hpp>
#include <cmath>

class MathUtils {
public:
    static double euclideanDistance(cv::Point2f p1, cv::Point2f p2) {
        float dx = p2.x - p1.x, dy = p2.y - p1.y;
        return std::sqrt(dx*dx + dy*dy);
    }

    static double euclideanDistance(double x1, double y1, double x2, double y2) {
        return std::sqrt((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1));
    }

    // SSD between two float vectors of the same length
    static double ssd(const float* a, const float* b, int len) {
        double s = 0.0;
        for (int i = 0; i < len; ++i) {
            double d = a[i] - b[i];
            s += d * d;
        }
        return s;
    }

    // NCC between two float vectors (zero-mean normalized)
    static double ncc(const float* a, const float* b, int len) {
        double ma = 0.0, mb = 0.0;
        for (int i = 0; i < len; ++i) { ma += a[i]; mb += b[i]; }
        ma /= len; mb /= len;

        double num = 0.0, da = 0.0, db = 0.0;
        for (int i = 0; i < len; ++i) {
            double ai = a[i] - ma, bi = b[i] - mb;
            num += ai * bi;
            da  += ai * ai;
            db  += bi * bi;
        }
        double denom = std::sqrt(da * db);
        return (denom < 1e-10) ? 0.0 : num / denom;
    }

    static double degreesToRadians(double d) { return d * CV_PI / 180.0; }
    static double radiansToDegrees(double r) { return r * 180.0 / CV_PI; }

    static int clamp(int v, int lo, int hi) {
        return v < lo ? lo : v > hi ? hi : v;
    }
    static double clampd(double v, double lo, double hi) {
        return v < lo ? lo : v > hi ? hi : v;
    }
};