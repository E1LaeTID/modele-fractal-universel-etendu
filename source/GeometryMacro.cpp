#include "GeometryMacro.hpp"

#include "Transform.hpp"

#include <cmath>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

GeometryMacro::GeometryMacro(
    Pattern sourceOpenPattern,
    Pattern sourceTargetPath,
    std::size_t sourceRecursionIndex
)
    : openPattern(
          std::move(sourceOpenPattern)
      ),
      targetPath(
          std::move(sourceTargetPath)
      ),
      recursionIndex(sourceRecursionIndex)
{
    validateInputs(
        openPattern,
        targetPath
    );
}

GeometryMacroResult GeometryMacro::execute() const
{
    Pattern projectedGeometry =
        substitute(
            openPattern,
            targetPath
        );

    return GeometryMacroResult{
        recursionIndex,
        openPattern.getName(),
        targetPath.getName(),
        std::move(projectedGeometry)
    };
}

const Pattern& GeometryMacro::getOpenPattern() const
{
    return openPattern;
}

const Pattern& GeometryMacro::getTargetPath() const
{
    return targetPath;
}

const Pattern& GeometryMacro::getClosedPath() const
{
    return targetPath;
}

std::size_t GeometryMacro::getRecursionIndex() const
{
    return recursionIndex;
}

Pattern GeometryMacro::substitute(
    const Pattern& sourcePattern,
    const Pattern& targetPath,
    const std::string& requestedResultName
)
{
    validateInputs(
        sourcePattern,
        targetPath
    );

    const std::vector<std::size_t>
        orderedSourcePointIndices =
            buildOrderedPointIndices(
                sourcePattern
            );

    const Point& sourceOrigin =
        sourcePattern.getPointById(
            sourcePattern.getOriginId()
        );

    const std::size_t sourceSegmentCount =
        sourcePattern.getSegments().size();

    const std::size_t targetSegmentCount =
        targetPath.getSegments().size();

    const std::size_t resultSegmentCount =
        sourceSegmentCount
        * targetSegmentCount;

    std::vector<Point> projectedPoints;
    std::vector<Segment> projectedSegments;

    /*
     * Une géométrie fermée possède autant de
     * points que de segments.
     *
     * Une géométrie ouverte possède un point
     * de plus que son nombre de segments.
     */
    const std::size_t resultPointCount =
        targetPath.isClosed()
            ? resultSegmentCount
            : resultSegmentCount + 1;

    projectedPoints.reserve(
        resultPointCount
    );

    projectedSegments.reserve(
        resultSegmentCount
    );

    std::size_t previousProjectedIndex = 0;

    const std::vector<Segment>& targetSegments =
        targetPath.getSegments();

    for (
        std::size_t targetSegmentIndex = 0;
        targetSegmentIndex < targetSegments.size();
        ++targetSegmentIndex
    )
    {
        const Segment& targetSegment =
            targetSegments[targetSegmentIndex];

        const Point& targetStart =
            targetPath.getPoint(
                targetSegment.startIndex
            );

        const Point& targetEnd =
            targetPath.getPoint(
                targetSegment.endIndex
            );

        /*
         * Le premier point n'est créé qu'une fois.
         * Les projections suivantes réutilisent
         * l'extrémité de la projection précédente.
         */
        if (targetSegmentIndex == 0)
        {
            const Point& sourceStart =
                sourcePattern.getPoint(
                    orderedSourcePointIndices.front()
                );

            Point mappedStart =
                Transform::mapPointToSegment(
                    sourceStart,
                    sourceOrigin,
                    targetStart,
                    targetEnd
                );

            mappedStart.id = "M0";

            projectedPoints.push_back(
                std::move(mappedStart)
            );

            previousProjectedIndex = 0;
        }

        for (
            std::size_t sourcePointPosition = 1;
            sourcePointPosition
                < orderedSourcePointIndices.size();
            ++sourcePointPosition
        )
        {
            const bool isLastTargetSegment =
                targetSegmentIndex + 1
                == targetSegments.size();

            const bool isLastSourcePoint =
                sourcePointPosition + 1
                == orderedSourcePointIndices.size();

            /*
             * La fermeture n'a lieu que si
             * le chemin cible est fermé.
             */
            const bool closesFinalGeometry =
                targetPath.isClosed()
                && isLastTargetSegment
                && isLastSourcePoint;

            std::size_t currentProjectedIndex = 0;

            if (closesFinalGeometry)
            {
                currentProjectedIndex = 0;
            }
            else
            {
                const Point& sourcePoint =
                    sourcePattern.getPoint(
                        orderedSourcePointIndices[
                            sourcePointPosition
                        ]
                    );

                Point mappedPoint =
                    Transform::mapPointToSegment(
                        sourcePoint,
                        sourceOrigin,
                        targetStart,
                        targetEnd
                    );

                currentProjectedIndex =
                    projectedPoints.size();

                mappedPoint.id =
                    "M"
                    + std::to_string(
                        currentProjectedIndex
                    );

                projectedPoints.push_back(
                    std::move(mappedPoint)
                );
            }

            projectedSegments.push_back(
                Segment{
                    previousProjectedIndex,
                    currentProjectedIndex
                }
            );

            previousProjectedIndex =
                currentProjectedIndex;
        }
    }

    const std::string resultName =
        requestedResultName.empty()
            ? sourcePattern.getName()
                + "-on-"
                + targetPath.getName()
            : requestedResultName;

    const std::string resultCategory =
        sourcePattern.getCategory()
        + "-on-"
        + targetPath.getCategory();

    const std::string resultDescription =
        "Substitution du motif ouvert "
        + sourcePattern.getName()
        + " sur les segments du chemin "
        + targetPath.getName()
        + ".";

    const PatternTopology resultTopology =
        targetPath.getTopology();

    const std::string resultEndPointId =
        targetPath.isClosed()
            ? "M0"
            : projectedPoints.back().id;

    return Pattern(
        resultName,
        resultCategory,
        resultDescription,
        resultTopology,
        "M0",
        resultEndPointId,
        std::move(projectedPoints),
        std::move(projectedSegments)
    );
}

void GeometryMacro::validateInputs(
    const Pattern& sourcePattern,
    const Pattern& targetPath
)
{
    sourcePattern.validate();
    targetPath.validate();

    if (!sourcePattern.isOpen())
    {
        throw std::invalid_argument(
            "Le motif source "
            + sourcePattern.getName()
            + " doit etre ouvert."
        );
    }

    if (sourcePattern.getSegments().empty())
    {
        throw std::invalid_argument(
            "Le motif source "
            + sourcePattern.getName()
            + " ne contient aucun segment."
        );
    }

    if (targetPath.getSegments().empty())
    {
        throw std::invalid_argument(
            "Le chemin cible "
            + targetPath.getName()
            + " ne contient aucun segment."
        );
    }

    validateNormalizedOpenPattern(
        sourcePattern
    );
}

std::vector<std::size_t>
GeometryMacro::buildOrderedPointIndices(
    const Pattern& pattern
)
{
    const std::vector<Segment>& segments =
        pattern.getSegments();

    if (segments.empty())
    {
        throw std::runtime_error(
            "Impossible de parcourir le motif "
            + pattern.getName()
            + " : aucun segment."
        );
    }

    std::vector<std::size_t> orderedIndices;

    orderedIndices.reserve(
        segments.size() + 1
    );

    orderedIndices.push_back(
        segments.front().startIndex
    );

    for (const Segment& segment : segments)
    {
        if (
            orderedIndices.back()
            != segment.startIndex
        )
        {
            throw std::runtime_error(
                "Les segments du motif "
                + pattern.getName()
                + " ne forment pas une chaine "
                  "ordonnee continue."
            );
        }

        orderedIndices.push_back(
            segment.endIndex
        );
    }

    return orderedIndices;
}

void GeometryMacro::validateNormalizedOpenPattern(
    const Pattern& pattern,
    double tolerance
)
{
    const Point& origin =
        pattern.getPointById(
            pattern.getOriginId()
        );

    const Point& endPoint =
        pattern.getPointById(
            pattern.getEndPointId()
        );

    const double localEndX =
        endPoint.normalizedX
        - origin.normalizedX;

    const double localEndY =
        endPoint.normalizedY
        - origin.normalizedY;

    if (
        std::abs(localEndX - 1.0)
            > tolerance
        || std::abs(localEndY)
            > tolerance
    )
    {
        throw std::runtime_error(
            "Le motif ouvert "
            + pattern.getName()
            + " n'est pas normalise : son point "
              "final relatif doit etre (1,0)."
        );
    }
}