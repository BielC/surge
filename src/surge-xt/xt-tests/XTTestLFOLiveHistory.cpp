/*
 * Surge XT - a free and open source hybrid synthesizer
 * Copyright 2026, various authors, as described in the GitHub transaction log.
 * Licensed under GPL-3.0-or-later; see LICENSE.
 */
#include "catch2/catch_amalgamated.hpp"
#include "../gui/widgets/LFOLiveHistory.h"
#include <limits>

TEST_CASE("Live LFO history stays bounded and never joins stale samples", "[lfo-history]")
{
    Surge::Widgets::LFOLiveHistory history;
    for (int i = 0; i < 1000; ++i)
        history.push(i * 33., float(i));
    REQUIRE(history.count == history.capacity);
    REQUIRE(history[0].value == 744.f);
    REQUIRE(history[history.count - 1].value == 999.f);
    REQUIRE(history[history.count - 1].time == 999 * 33.);
    history.push(40000, 1.f); // Engine/editor stopped responding.
    REQUIRE(history.count == 1);
    history.push(39900, 2.f); // Clock reset.
    REQUIRE(history.count == 1);
    REQUIRE(history[0].value == 2.f);
    history.push(39933, std::numeric_limits<float>::quiet_NaN());
    REQUIRE(history.count == 0);
    history.push(40000, 3.f);
    history.clear(); // Scene, source, shape or output selection changed.
    REQUIRE(history.count == 0);
    history.push(40033, 4.f);
    REQUIRE(history.count == 1);
    REQUIRE(history[0].value == 4.f);
    REQUIRE(sizeof(history) < 8192);
}

TEST_CASE("Live history renders joined output and held steps", "[lfo-history]")
{
    Surge::Widgets::LFOLiveHistory history;
    for (int i = 0; i <= 250; ++i)
        history.push(i * 32., i / 125.f - 1.f);
    auto smooth = history.path({0, 0, 800, 100}, false);
    REQUIRE_FALSE(smooth.isEmpty());
    REQUIRE(smooth.getBounds().getWidth() == Catch::Approx(800));
    REQUIRE(smooth.getBounds().getHeight() == Catch::Approx(80));
    REQUIRE(smooth.getLength() > 800);
    auto held = history.path({0, 0, 800, 100}, true);
    REQUIRE_FALSE(held.isEmpty());
    REQUIRE(held.getLength() == Catch::Approx(880).margin(.1));
    history.clear();
    REQUIRE(history.path({0, 0, 800, 100}, false).isEmpty());
}
