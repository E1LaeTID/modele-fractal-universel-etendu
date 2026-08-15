#pragma once

#include "Pattern.hpp"
#include "Point.hpp"
#include "ProjectionEngine.hpp"

#include <SFML/Graphics.hpp>

#include <initializer_list>

enum class RenderStage
{
    ClosedPath,
    ClosedGeometry,
    HalfContour,
    NextMotif
};

class Renderer
{
public:
    explicit Renderer(
        float margin = 60.0f
    );

    void drawPattern(
        sf::RenderTarget& target,
        const Pattern& pattern,
        const sf::Color& color,
        bool drawPoints = false
    ) const;

    void drawCycle(
        sf::RenderTarget& target,
        const ProjectionCycleResult& cycle,
        RenderStage stage
    ) const;

private:
    struct GeometryBounds
    {
        double minimumX = 0.0;
        double maximumX = 0.0;
        double minimumY = 0.0;
        double maximumY = 0.0;
    };

    struct ScreenMapping
    {
        double geometryCenterX = 0.0;
        double geometryCenterY = 0.0;

        float screenCenterX = 0.0f;
        float screenCenterY = 0.0f;

        double scale = 1.0;
    };

    float margin;

    GeometryBounds calculateBounds(
        std::initializer_list<
            const Pattern*
        > patterns
    ) const;

    ScreenMapping createScreenMapping(
        const sf::RenderTarget& target,
        const GeometryBounds& bounds
    ) const;

    sf::Vector2f mapPointToScreen(
        const Point& point,
        const ScreenMapping& mapping
    ) const;

    void drawSegments(
        sf::RenderTarget& target,
        const Pattern& pattern,
        const ScreenMapping& mapping,
        const sf::Color& color
    ) const;

    void drawPatternPoints(
        sf::RenderTarget& target,
        const Pattern& pattern,
        const ScreenMapping& mapping,
        const sf::Color& color
    ) const;

    void drawOrigin(
        sf::RenderTarget& target,
        const Pattern& pattern,
        const ScreenMapping& mapping
    ) const;
};