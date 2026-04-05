#pragma once
#include <chrono>

// ── Header-only precision timer ──────────────────────────────────────────────
// Usage:
//   TimingUtils::Timer t;
//   t.start();
//   /* ... work ... */
//   double ms = t.elapsedMs();

class TimingUtils {
public:
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = Clock::time_point;

    struct Timer {
        TimePoint m_t0;

        void start() { m_t0 = Clock::now(); }

        double elapsedMs() const {
            auto t1 = Clock::now();
            return std::chrono::duration<double, std::milli>(t1 - m_t0).count();
        }

        // Convenience: start + capture in one expression
        static Timer started() {
            Timer t;
            t.start();
            return t;
        }
    };
};