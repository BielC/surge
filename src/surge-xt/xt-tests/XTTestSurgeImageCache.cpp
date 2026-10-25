/*
 * Surge XT - a free and open source hybrid synthesizer
 * Copyright 2026, various authors, as described in the GitHub transaction log.
 * Licensed under GPL-3.0-or-later; see LICENSE.
 */
#include "catch2/catch_amalgamated.hpp"
#include "../gui/SurgeImage.h"

namespace
{
struct CountingRectangle : juce::DrawableRectangle
{
    int &draws;
    explicit CountingRectangle(int &count) : draws(count)
    {
        setRectangle(juce::Rectangle<float>(4, 6, 12, 10));
        setFill(juce::Colours::red);
    }
    void paint(juce::Graphics &g) override
    {
        ++draws;
        juce::DrawableRectangle::paint(g);
    }
};
} // namespace

TEST_CASE("Shared artwork caches rasterize once per display scale", "[image-cache]")
{
    juce::ScopedJuceInitialiser_GUI juce;
    int draws = 0;
    std::unique_ptr<juce::Drawable> drawable = std::make_unique<CountingRectangle>(draws);
    SurgeImage image(drawable);
    auto render = [&](float scale, int mode = 0) {
        juce::Image pixels(juce::Image::ARGB, 128, 128, true);
        juce::Graphics g(pixels);
        g.addTransform(juce::AffineTransform::scale(scale));
        if (mode == 1)
            image.drawAt(g, 8, 8, .5f);
        else if (mode == 2)
            image.drawWithin(g, {20, 20, 12, 10}, juce::RectanglePlacement::stretchToFit, 1.f);
        else
            image.draw(g, 1.f);
        return pixels;
    };
    REQUIRE(render(1.f).getPixelAt(8, 10) == juce::Colours::red);
    REQUIRE(draws == 1);
    render(1.f);
    REQUIRE(draws == 1);
    auto translated = render(1.f, 1);
    REQUIRE(draws == 1);
    REQUIRE(translated.getPixelAt(16, 18).getAlpha() == 128);
    REQUIRE(translated.getPixelAt(8, 10).isTransparent());
    auto fitted = render(1.f, 2);
    REQUIRE(draws == 1);
    REQUIRE(fitted.getPixelAt(24, 24) == juce::Colours::red);
    REQUIRE(fitted.getPixelAt(8, 10).isTransparent());

    REQUIRE(render(2.f).getPixelAt(16, 20) == juce::Colours::red);
    REQUIRE(draws == 2);
    render(2.f);
    REQUIRE(draws == 2);
    image.setPhysicalZoomFactor(150);
    render(2.f);
    REQUIRE(draws == 3);

    // Callers receiving the original pointer may mutate it at any later point.
    auto *mutableRectangle =
        dynamic_cast<CountingRectangle *>(image.getDrawableButUseWithCaution());
    REQUIRE(mutableRectangle);
    mutableRectangle->setFill(juce::Colours::blue);
    REQUIRE(render(2.f).getPixelAt(16, 20) == juce::Colours::blue);
    REQUIRE(draws == 4);
    mutableRectangle->setFill(juce::Colours::green);
    REQUIRE(render(2.f).getPixelAt(16, 20) == juce::Colours::green);
    REQUIRE(draws == 5);
}

TEST_CASE("Large artwork falls back to vectors without a large raster allocation", "[image-cache]")
{
    juce::ScopedJuceInitialiser_GUI juce;
    int draws = 0;
    auto rectangle = std::make_unique<CountingRectangle>(draws);
    rectangle->setRectangle(juce::Rectangle<float>(0, 0, 10000, 10000));
    std::unique_ptr<juce::Drawable> drawable = std::move(rectangle);
    SurgeImage image(drawable);
    juce::Image pixels(juce::Image::ARGB, 8, 8, true);
    juce::Graphics g(pixels);
    image.draw(g, 1.f);
    REQUIRE(draws == 1);
    image.draw(g, 1.f);
    REQUIRE(draws == 2);
    REQUIRE(pixels.getPixelAt(4, 4) == juce::Colours::red);
}
