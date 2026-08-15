#include "Renderer.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>

Renderer::Renderer(
    float rendererMargin
)
    : margin(rendererMargin)
{
    if (margin < 0.0f)
    {
        throw std::invalid_argument(
            "La marge du Renderer ne peut pas "
            "etre negative."
        );
    }
}

void Renderer::drawPattern(
    sf::RenderTarget& target,
    const Pattern& pattern,
    const sf::Color& color,
    bool drawPoints
) const
{
    const GeometryBounds bounds =
        calculateBounds({&pattern});

    const ScreenMapping mapping =
        createScreenMapping(
            target,
            bounds
        );

    drawSegments(
        target,
        pattern,
        mapping,
        color
    );

    if (drawPoints)
    {
        drawPatternPoints(
            target,
            pattern,
            mapping,
            color
        );
    }

    drawOrigin(
        target,
        pattern,
        mapping
    );
}

void Renderer::drawCycle(
    sf::RenderTarget& target,
    const ProjectionCycleResult& cycle,
    RenderStage stage
) const
{
    switch (stage)
    {
        case RenderStage::ClosedPath:
        {
            drawPattern(
                target,
                cycle.closedPath,
                sf::Color(190, 200, 215),
                true
            );

            break;
        }

        case RenderStage::ClosedGeometry:
        {
            const GeometryBounds bounds =
                calculateBounds(
                    {
                        &cycle.closedPath,
                        &cycle.closedGeometry
                    }
                );

            const ScreenMapping mapping =
                createScreenMapping(
                    target,
                    bounds
                );

            /*
             * Contour polygonal de référence.
             */
            drawSegments(
                target,
                cycle.closedPath,
                mapping,
                sf::Color(75, 85, 105)
            );

            drawPatternPoints(
                target,
                cycle.closedPath,
                mapping,
                sf::Color(170, 180, 200)
            );

            /*
             * Nouvelle géométrie fermée.
             */
            drawSegments(
                target,
                cycle.closedGeometry,
                mapping,
                sf::Color(60, 225, 185)
            );

            drawOrigin(
                target,
                cycle.closedGeometry,
                mapping
            );

            break;
        }

        case RenderStage::HalfContour:
        {
            /*
             * La moitié est normalisée dans une
             * nouvelle base locale. Elle ne doit
             * donc pas être superposée directement
             * à la géométrie fermée précédente.
             */
            drawPattern(
                target,
                cycle.halfContour,
                sf::Color(255, 185, 70),
                cycle.halfContour
                        .getPoints()
                        .size()
                    <= 200
            );

            break;
        }

        case RenderStage::NextMotif:
        {
            if (cycle.nextMotif.has_value())
            {
                drawPattern(
                    target,
                    cycle.nextMotif.value(),
                    sf::Color(120, 155, 255),
                    cycle.nextMotif
                            ->getPoints()
                            .size()
                        <= 200
                );
            }
            else
            {
                /*
                 * Aucun prochain motif n'existe
                 * après le dernier cycle ou lorsque
                 * la limite de sécurité est atteinte.
                 */
                drawPattern(
                    target,
                    cycle.halfContour,
                    sf::Color(120, 125, 140),
                    false
                );
            }

            break;
        }
    }
}

Renderer::GeometryBounds
Renderer::calculateBounds(
    std::initializer_list<
        const Pattern*
    > patterns
) const
{
    GeometryBounds bounds;

    bounds.minimumX =
        std::numeric_limits<double>::max();

    bounds.maximumX =
        std::numeric_limits<double>::lowest();

    bounds.minimumY =
        std::numeric_limits<double>::max();

    bounds.maximumY =
        std::numeric_limits<double>::lowest();

    bool containsPoint = false;

    for (const Pattern* pattern : patterns)
    {
        if (pattern == nullptr)
        {
            continue;
        }

        for (
            const Point& point :
            pattern->getPoints()
        )
        {
            containsPoint = true;

            bounds.minimumX =
                std::min(
                    bounds.minimumX,
                    point.normalizedX
                );

            bounds.maximumX =
                std::max(
                    bounds.maximumX,
                    point.normalizedX
                );

            bounds.minimumY =
                std::min(
                    bounds.minimumY,
                    point.normalizedY
                );

            bounds.maximumY =
                std::max(
                    bounds.maximumY,
                    point.normalizedY
                );
        }
    }

    if (!containsPoint)
    {
        throw std::runtime_error(
            "Impossible d'afficher une "
            "geometrie vide."
        );
    }

    return bounds;
}

Renderer::ScreenMapping
Renderer::createScreenMapping(
    const sf::RenderTarget& target,
    const GeometryBounds& bounds
) const
{
    const sf::Vector2u targetSize =
        target.getSize();

    const double geometryWidth =
        bounds.maximumX - bounds.minimumX;

    const double geometryHeight =
        bounds.maximumY - bounds.minimumY;

    const double safeGeometryWidth =
        geometryWidth > 0.0
            ? geometryWidth
            : 1.0;

    const double safeGeometryHeight =
        geometryHeight > 0.0
            ? geometryHeight
            : 1.0;

    const double availableWidth =
        std::max(
            1.0,
            static_cast<double>(targetSize.x)
                - 2.0 * margin
        );

    const double availableHeight =
        std::max(
            1.0,
            static_cast<double>(targetSize.y)
                - 2.0 * margin
        );

    ScreenMapping mapping;

    mapping.geometryCenterX =
        (
            bounds.minimumX
            + bounds.maximumX
        )
        / 2.0;

    mapping.geometryCenterY =
        (
            bounds.minimumY
            + bounds.maximumY
        )
        / 2.0;

    mapping.screenCenterX =
        static_cast<float>(targetSize.x)
        / 2.0f;

    mapping.screenCenterY =
        static_cast<float>(targetSize.y)
        / 2.0f;

    mapping.scale =
        std::min(
            availableWidth
                / safeGeometryWidth,

            availableHeight
                / safeGeometryHeight
        );

    return mapping;
}

sf::Vector2f Renderer::mapPointToScreen(
    const Point& point,
    const ScreenMapping& mapping
) const
{
    return {
        mapping.screenCenterX
            + static_cast<float>(
                (
                    point.normalizedX
                    - mapping.geometryCenterX
                )
                * mapping.scale
            ),

        mapping.screenCenterY
            - static_cast<float>(
                (
                    point.normalizedY
                    - mapping.geometryCenterY
                )
                * mapping.scale
            )
    };
}

void Renderer::drawSegments(
    sf::RenderTarget& target,
    const Pattern& pattern,
    const ScreenMapping& mapping,
    const sf::Color& color
) const
{
    sf::VertexArray lines(
        sf::PrimitiveType::Lines
    );

    lines.resize(
        pattern.getSegments().size() * 2
    );

    std::size_t vertexIndex = 0;

    for (
        const Segment& segment :
        pattern.getSegments()
    )
    {
        const Point& start =
            pattern.getPoint(
                segment.startIndex
            );

        const Point& end =
            pattern.getPoint(
                segment.endIndex
            );

        lines[vertexIndex].position =
            mapPointToScreen(
                start,
                mapping
            );

        lines[vertexIndex].color =
            color;

        ++vertexIndex;

        lines[vertexIndex].position =
            mapPointToScreen(
                end,
                mapping
            );

        lines[vertexIndex].color =
            color;

        ++vertexIndex;
    }

    target.draw(lines);
}

void Renderer::drawPatternPoints(
    sf::RenderTarget& target,
    const Pattern& pattern,
    const ScreenMapping& mapping,
    const sf::Color& color
) const
{
    /*
     * Ne pas créer des milliers de CircleShape
     * pour les géométries récursives volumineuses.
     */
    if (pattern.getPoints().size() > 500)
    {
        return;
    }

    constexpr float radius = 3.5f;

    sf::CircleShape pointShape(radius);

    pointShape.setOrigin(
        {
            radius,
            radius
        }
    );

    pointShape.setFillColor(color);

    for (
        const Point& point :
        pattern.getPoints()
    )
    {
        pointShape.setPosition(
            mapPointToScreen(
                point,
                mapping
            )
        );

        target.draw(pointShape);
    }
}

void Renderer::drawOrigin(
    sf::RenderTarget& target,
    const Pattern& pattern,
    const ScreenMapping& mapping
) const
{
    const Point& origin =
        pattern.getPointById(
            pattern.getOriginId()
        );

    constexpr float radius = 6.0f;

    sf::CircleShape originShape(radius);

    originShape.setOrigin(
        {
            radius,
            radius
        }
    );

    originShape.setPosition(
        mapPointToScreen(
            origin,
            mapping
        )
    );

    originShape.setFillColor(
        sf::Color(255, 95, 80)
    );

    target.draw(originShape);
}