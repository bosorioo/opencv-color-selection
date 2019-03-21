#ifndef CLOCK_H
#define CLOCK_H

#include <chrono>

class Clock
{
public:

    using c_clock = std::chrono::high_resolution_clock;

    Clock(bool running = true, float rate = 1.f) :
        m_start(c_clock::now()), m_running(running), m_rate(rate) {}

    float getElapsedSeconds() {
        if (m_running)
            return (m_run_seconds + (float)(c_clock::now() - m_start).count() *
                c_clock::period::num / c_clock::period::den) * m_rate;
        else
            return m_run_seconds * m_rate;
    }

    float restart() {
        float runtime = getElapsedSeconds();
        m_running = true;
        m_run_seconds = 0.f;
        m_start = c_clock::now();
        return runtime;
    }

    float pause() {

        if (m_running)
        {
            m_run_seconds = getElapsedSeconds();
            m_running = false;
        }

        return m_run_seconds;
    }

    void resume() {
        if (!m_running)
        {
            m_running = true;
            m_start = c_clock::now();
        }
    }

private:

    c_clock::time_point m_start;
    bool m_running = true;
    float m_run_seconds = 0.f;
    float m_rate;

};

#endif // CLOCK_H
