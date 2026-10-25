/*
 * Surge XT - a free and open source hybrid synthesizer
 * Copyright 2026, various authors, as described in the GitHub transaction log.
 * Licensed under GPL-3.0-or-later; see LICENSE.
 */
#ifndef SURGE_SRC_SURGE_XT_GUI_WIDGETS_LFOLIVEHISTORY_H
#define SURGE_SRC_SURGE_XT_GUI_WIDGETS_LFOLIVEHISTORY_H

#include <array>
#include <cmath>
#include <cstddef>
#include "juce_graphics/juce_graphics.h"

namespace Surge::Widgets
{
// Message-thread history only. No allocation or audio-thread buffer/copy is needed.
struct LFOLiveHistory
{
    struct Sample
    {
        double time;
        float value;
    };
    static constexpr size_t capacity = 256;
    static constexpr double durationMs = 8000;
    std::array<Sample, capacity> samples{};
    size_t count{0}, next{0};

    void clear() { count = next = 0; }
    const Sample &operator[](size_t index) const
    {
        return samples[(next + capacity - count + index) % capacity];
    }
    void push(double time, float value)
    {
        if (!std::isfinite(time) || !std::isfinite(value))
        {
            clear();
            return;
        }
        if (count && (time <= (*this)[count - 1].time || time - (*this)[count - 1].time > 250))
            clear();
        samples[next] = {time, value};
        next = (next + 1) % capacity;
        if (count < capacity)
            ++count;
    }

    juce::Path path(juce::Rectangle<float> bounds, bool stepped) const
    {
        juce::Path trace;
        bool started = false;
        auto latest = count ? (*this)[count - 1].time : 0;
        for (size_t i = 0; i < count; ++i)
        {
            auto &point = (*this)[i];
            auto age = latest - point.time;
            if (age > durationMs)
                continue;
            auto x = bounds.getRight() - float(age / durationMs) * bounds.getWidth();
            auto y = bounds.getY() +
                     (0.5f - 0.4f * std::clamp(point.value, -1.f, 1.f)) * bounds.getHeight();
            if (!started)
            {
                trace.startNewSubPath(x, y);
                started = true;
            }
            else
            {
                if (stepped)
                    trace.lineTo(x, trace.getCurrentPosition().y);
                trace.lineTo(x, y);
            }
        }
        return trace;
    }
};
} // namespace Surge::Widgets
#endif
