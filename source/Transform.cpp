#include "Transform.hpp"

#include <cmath>
#include <functional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace
{
    Pattern transformPatternPoints(
        const Pattern& pattern,
        const std::string& resultName,
        const std::function<Point(const Point&)>&
            transformation
    )
    {
        std::vector<Point> transformedPoints;

        transformedPoints.reserve(
            pattern.getPoints().size()
        );

        for (
            const Point& point :
            pattern.getPoints()
        )
        {
            transformedPoints.push_back(
                transformation(point)
            );
        }

        return Pattern(
            resultName,
            pattern.getCategory(),
            pattern.getDescription(),
            pattern.getTopology(),
            pattern.getOriginId(),
            pattern.getEndPointId(),
            std::move(transformedPoints),
            pattern.getSegments()
        );
    }
}

Point Transform::translatePoint(
    const Point& point,
    double offsetX,
    double offsetY
)
{
    return Point{
        point.id,
        point.normalizedX + offsetX,
        point.normalizedY + offsetY
    };
}

Point Transform::rotatePoint(
    const Point& point,
    double angleRadians,
    const Point& pivot
)
{
    const double translatedX =
        point.normalizedX
        - pivot.normalizedX;

    const double translatedY =
        point.normalizedY
        - pivot.normalizedY;

    const double cosine =
        std::cos(angleRadians);

    const double sine =
        std::sin(angleRadians);

    const double rotatedX =
        translatedX * cosine
        - translatedY * sine;

    const double rotatedY =
        translatedX * sine
        + translatedY * cosine;

    return Point{
        point.id,

        pivot.normalizedX
            + rotatedX,

        pivot.normalizedY
            + rotatedY
    };
}

Point Transform::scalePoint(
    const Point& point,
    double scaleX,
    double scaleY,
    const Point& pivot
)
{
    return Point{
        point.id,

        pivot.normalizedX
            + (
                point.normalizedX
                - pivot.normalizedX
            )
            * scaleX,

        pivot.normalizedY
            + (
                point.normalizedY
                - pivot.normalizedY
            )
            * scaleY
    };
}

Pattern Transform::translatePattern(
    const Pattern& pattern,
    double offsetX,
    double offsetY
)
{
    return transformPatternPoints(
        pattern,
        pattern.getName() + "-translated",

        [offsetX, offsetY](
            const Point& point
        )
        {
            return translatePoint(
                point,
                offsetX,
                offsetY
            );
        }
    );
}

Pattern Transform::rotatePattern(
    const Pattern& pattern,
    double angleRadians,
    const Point& pivot
)
{
    return transformPatternPoints(
        pattern,
        pattern.getName() + "-rotated",

        [angleRadians, &pivot](
            const Point& point
        )
        {
            return rotatePoint(
                point,
                angleRadians,
                pivot
            );
        }
    );
}

Pattern Transform::rotatePatternAroundOrigin(
    const Pattern& pattern,
    double angleRadians
)
{
    const Point& origin =
        pattern.getPointById(
            pattern.getOriginId()
        );

    return rotatePattern(
        pattern,
        angleRadians,
        origin
    );
}

Pattern Transform::scalePattern(
    const Pattern& pattern,
    double scaleX,
    double scaleY,
    const Point& pivot
)
{
    if (
        scaleX == 0.0
        || scaleY == 0.0
    )
    {
        throw std::invalid_argument(
            "Le facteur d'echelle ne peut pas "
            "etre nul."
        );
    }

    return transformPatternPoints(
        pattern,
        pattern.getName() + "-scaled",

        [scaleX, scaleY, &pivot](
            const Point& point
        )
        {
            return scalePoint(
                point,
                scaleX,
                scaleY,
                pivot
            );
        }
    );
}

Pattern Transform::scalePatternFromOrigin(
    const Pattern& pattern,
    double scaleX,
    double scaleY
)
{
    const Point& origin =
        pattern.getPointById(
            pattern.getOriginId()
        );

    return scalePattern(
        pattern,
        scaleX,
        scaleY,
        origin
    );
}

Point Transform::mapPointToSegment(
    const Point& localPoint,
    const Point& sourceOrigin,
    const Point& targetStart,
    const Point& targetEnd
)
{
    const double segmentX =
        targetEnd.normalizedX
        - targetStart.normalizedX;

    const double segmentY =
        targetEnd.normalizedY
        - targetStart.normalizedY;

    const double segmentLength =
        std::hypot(
            segmentX,
            segmentY
        );

    if (segmentLength == 0.0)
    {
        throw std::invalid_argument(
            "Impossible de projeter un point "
            "sur un segment de longueur nulle."
        );
    }

    /*
     * Coordonnees locales relativement a M0.
     */
    const double localX =
        localPoint.normalizedX
        - sourceOrigin.normalizedX;

    const double localY =
        localPoint.normalizedY
        - sourceOrigin.normalizedY;

    /*
     * Le segment cible fournit directement
     * le premier vecteur de la base locale.
     *
     * U = targetEnd - targetStart
     */
    const double basisUX =
        segmentX;

    const double basisUY =
        segmentY;

    /*
     * V est la rotation antihoraire de U
     * d'un angle de 90 degres.
     *
     * V = (-Uy, Ux)
     */
    const double basisVX =
        -segmentY;

    const double basisVY =
        segmentX;

    /*
     * Projection affine :
     *
     * P =
     *     targetStart
     *     + localX * U
     *     + localY * V
     *
     * La longueur du segment est deja comprise
     * dans U et V. Il ne faut donc pas multiplier
     * une seconde fois par segmentLength.
     */
    const double mappedX =
        targetStart.normalizedX
        + localX * basisUX
        + localY * basisVX;

    const double mappedY =
        targetStart.normalizedY
        + localX * basisUY
        + localY * basisVY;

    return Point{
        localPoint.id,
        mappedX,
        mappedY
    };
}

Pattern Transform::mapPatternToSegment(
    const Pattern& sourcePattern,
    const Point& targetStart,
    const Point& targetEnd
)
{
    if (
        distance(targetStart, targetEnd)
        == 0.0
    )
    {
        throw std::invalid_argument(
            "Impossible de projeter un motif "
            "sur un segment de longueur nulle."
        );
    }

    const Point sourceOrigin =
        sourcePattern.getPointById(
            sourcePattern.getOriginId()
        );

    return transformPatternPoints(
        sourcePattern,
        sourcePattern.getName() + "-mapped",

        [
            &sourceOrigin,
            &targetStart,
            &targetEnd
        ](
            const Point& point
        )
        {
            return mapPointToSegment(
                point,
                sourceOrigin,
                targetStart,
                targetEnd
            );
        }
    );
}

double Transform::distance(
    const Point& start,
    const Point& end
)
{
    const double deltaX =
        end.normalizedX
        - start.normalizedX;

    const double deltaY =
        end.normalizedY
        - start.normalizedY;

    return std::hypot(
        deltaX,
        deltaY
    );
}

double Transform::segmentAngle(
    const Point& start,
    const Point& end
)
{
    return std::atan2(
        end.normalizedY
            - start.normalizedY,

        end.normalizedX
            - start.normalizedX
    );
}