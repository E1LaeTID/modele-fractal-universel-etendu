#include "RecursionReducer.hpp"

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

RecursionReductionResult
RecursionReducer::rollback(
    const Pattern& openGeometry,
    const std::vector<std::size_t>&
        substitutionSegmentCounts,
    std::size_t requestedLevelCount
)
{
    openGeometry.validate();

    if (!openGeometry.isOpen())
    {
        throw std::invalid_argument(
            "RecursionReducer::rollback attend "
            "une geometrie ouverte."
        );
    }

    Pattern currentPattern =
        openGeometry;

    std::vector<std::size_t> remainingCounts =
        substitutionSegmentCounts;

    std::size_t appliedLevelCount = 0;

    while (
        appliedLevelCount < requestedLevelCount
        && !remainingCounts.empty()
    )
    {
        const std::size_t sourceSegmentCount =
            remainingCounts.back();

        if (sourceSegmentCount == 0)
        {
            throw std::runtime_error(
                "Un niveau de substitution contient "
                "un nombre de segments nul."
            );
        }

        /*
         * Si aucun groupe complet ne peut être
         * reconstruit, la rétrogradation s'arrête
         * sans supprimer ce niveau de l'historique.
         */
        if (
            currentPattern
                .getSegments()
                .size()
            < sourceSegmentCount
        )
        {
            break;
        }

        currentPattern =
            collapseLastLevel(
                currentPattern,
                sourceSegmentCount,
                appliedLevelCount
            );

        remainingCounts.pop_back();

        ++appliedLevelCount;
    }

    return RecursionReductionResult{
        std::move(currentPattern),
        requestedLevelCount,
        appliedLevelCount,
        std::move(remainingCounts)
    };
}

Pattern RecursionReducer::collapseLastLevel(
    const Pattern& openGeometry,
    std::size_t sourceSegmentCount,
    std::size_t rollbackLevel
)
{
    openGeometry.validate();

    if (!openGeometry.isOpen())
    {
        throw std::invalid_argument(
            "RecursionReducer::collapseLastLevel "
            "attend une geometrie ouverte."
        );
    }

    if (sourceSegmentCount == 0)
    {
        throw std::invalid_argument(
            "Le nombre de segments du motif source "
            "doit etre strictement positif."
        );
    }

    const std::vector<Segment>& sourceSegments =
        openGeometry.getSegments();

    /*
     * Seuls les groupes complets sont utilisés.
     *
     * Exemple :
     *
     * 22 segments avec un motif de 7 segments
     *     ↓
     * 3 groupes complets
     *     ↓
     * 21 segments utilisables
     *     ↓
     * 3 segments parents
     *
     * Le dernier segment incomplet est exclu.
     */
    const std::size_t completeGroupCount =
        sourceSegments.size()
        / sourceSegmentCount;

    if (completeGroupCount == 0)
    {
        throw std::runtime_error(
            "Impossible de retrograder "
            + openGeometry.getName()
            + " : aucun groupe complet de "
            + std::to_string(sourceSegmentCount)
            + " segments."
        );
    }

    const std::size_t usableSegmentCount =
        completeGroupCount
        * sourceSegmentCount;

    std::vector<Point> parentPoints;
    std::vector<Segment> parentSegments;

    parentPoints.reserve(
        completeGroupCount + 1
    );

    parentSegments.reserve(
        completeGroupCount
    );

    const Segment& firstChildSegment =
        sourceSegments.front();

    Point firstParentPoint =
        openGeometry.getPoint(
            firstChildSegment.startIndex
        );

    firstParentPoint.id = "M0";

    parentPoints.push_back(
        std::move(firstParentPoint)
    );

    for (
        std::size_t groupIndex = 0;
        groupIndex < completeGroupCount;
        ++groupIndex
    )
    {
        const std::size_t firstChildIndex =
            groupIndex
            * sourceSegmentCount;

        const std::size_t lastChildIndex =
            firstChildIndex
            + sourceSegmentCount
            - 1;

        /*
         * Vérification de la continuité à
         * l'intérieur du groupe.
         */
        for (
            std::size_t childIndex =
                firstChildIndex + 1;

            childIndex <= lastChildIndex;

            ++childIndex
        )
        {
            const Segment& previousChild =
                sourceSegments[
                    childIndex - 1
                ];

            const Segment& currentChild =
                sourceSegments[
                    childIndex
                ];

            if (
                previousChild.endIndex
                != currentChild.startIndex
            )
            {
                throw std::runtime_error(
                    "Le groupe de substitution "
                    + std::to_string(groupIndex)
                    + " n'est pas continu dans "
                    + openGeometry.getName()
                    + "."
                );
            }
        }

        /*
         * Vérification de la continuité entre
         * deux groupes successifs.
         */
        if (groupIndex > 0)
        {
            const Segment& previousGroupLast =
                sourceSegments[
                    firstChildIndex - 1
                ];

            const Segment& currentGroupFirst =
                sourceSegments[
                    firstChildIndex
                ];

            if (
                previousGroupLast.endIndex
                != currentGroupFirst.startIndex
            )
            {
                throw std::runtime_error(
                    "Les groupes de substitution "
                    "ne sont pas continus dans "
                    + openGeometry.getName()
                    + "."
                );
            }
        }

        const Segment& lastChildSegment =
            sourceSegments[lastChildIndex];

        Point parentEndPoint =
            openGeometry.getPoint(
                lastChildSegment.endIndex
            );

        const std::size_t parentEndIndex =
            parentPoints.size();

        parentEndPoint.id =
            "M"
            + std::to_string(
                parentEndIndex
            );

        parentPoints.push_back(
            std::move(parentEndPoint)
        );

        parentSegments.push_back(
            Segment{
                parentEndIndex - 1,
                parentEndIndex
            }
        );
    }

    const std::string endPointId =
        parentPoints.back().id;

    return Pattern(
        openGeometry.getName()
            + "-rollback-"
            + std::to_string(
                rollbackLevel + 1
            ),

        openGeometry.getCategory()
            + "-rollback",

        "Retrogradation structurelle de "
            + openGeometry.getName()
            + ". Segments source : "
            + std::to_string(
                sourceSegments.size()
            )
            + ". Segments utilisables : "
            + std::to_string(
                usableSegmentCount
            )
            + ". Segments parents : "
            + std::to_string(
                completeGroupCount
            )
            + ".",

        PatternTopology::Open,

        "M0",
        endPointId,

        std::move(parentPoints),
        std::move(parentSegments)
    );
}