#include "ProjectionEngine.hpp"

#include "ContourReducer.hpp"
#include "RecursionReducer.hpp"

#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

ProjectionEngine::ProjectionEngine(
    std::vector<Pattern> sourceOpenPatterns,
    std::vector<Pattern> sourceClosedPaths,
    std::size_t sourceMaximumSegmentCount
)
    : openPatterns(
          std::move(sourceOpenPatterns)
      ),
      closedPaths(
          std::move(sourceClosedPaths)
      ),
      maximumSegmentCount(
          sourceMaximumSegmentCount
      )
{
    if (maximumSegmentCount == 0)
    {
        throw std::invalid_argument(
            "La limite de segments doit etre "
            "strictement positive."
        );
    }

    validatePatternLists();

    currentMotif =
        openPatterns.front();

    /*
     * Le premier niveau de provenance correspond
     * au premier motif ouvert.
     */
    substitutionSegmentCounts.clear();

    substitutionSegmentCounts.push_back(
        openPatterns.front()
            .getSegments()
            .size()
    );

    history.reserve(
        closedPaths.size()
    );

    cycles.reserve(
        closedPaths.size()
    );
}

bool ProjectionEngine::hasNext() const
{
    return !safetyLimitReached
        && currentIndex < closedPaths.size();
}

bool ProjectionEngine::canProcessNext() const
{
    if (
        safetyLimitReached
        || currentIndex >= closedPaths.size()
    )
    {
        return false;
    }

    return estimateNextClosedSegmentCount()
        <= maximumSegmentCount;
}

GeometryMacroResult ProjectionEngine::step()
{
    if (!hasNext())
    {
        throw std::out_of_range(
            "Aucune iteration supplementaire "
            "n'est disponible."
        );
    }

    const std::size_t estimatedClosedSegments =
        estimateNextClosedSegmentCount();

    if (
        estimatedClosedSegments
        > maximumSegmentCount
    )
    {
        safetyLimitReached = true;

        throw std::runtime_error(
            "Iteration interrompue : la prochaine "
            "geometrie contiendrait "
            + std::to_string(
                estimatedClosedSegments
            )
            + " segments, au-dessus de la limite de "
            + std::to_string(
                maximumSegmentCount
            )
            + "."
        );
    }

    const std::size_t cycleIndex =
        currentIndex;

    /*
     * Le motif entrant doit être sauvegardé avant
     * la création du motif récursif suivant.
     */
    Pattern inputMotif =
        currentMotif;

    const Pattern& currentClosedPath =
        closedPaths[cycleIndex];

    /*
     * ÉTAPE 1
     *
     * Le motif récursif courant remplace tous
     * les segments du contour fermé courant.
     */
    Pattern closedGeometry =
        GeometryMacro::substitute(
            inputMotif,
            currentClosedPath,
            "closed-geometry-"
                + std::to_string(
                    cycleIndex
                )
        );

    /*
     * ÉTAPE 2
     *
     * La géométrie fermée est coupée en deux.
     * La moitié est directement normalisée.
     */
    Pattern halfContour =
        ContourReducer::extractNormalizedHalf(
            closedGeometry
        );

    /*
     * ÉTAPE 3
     *
     * Le nombre de niveaux à retirer dépend
     * de l'indice du contour fermé :
     *
     * triangle   : 0
     * carré      : 1
     * pentagone  : 2
     * hexagone   : 3
     * etc.
     */
    const std::size_t
        requestedRollbackLevelCount =
            cycleIndex;

    RecursionReductionResult reductionResult =
        RecursionReducer::rollback(
            halfContour,
            substitutionSegmentCounts,
            requestedRollbackLevelCount
        );

    /*
     * Une rétrogradation peut exclure un groupe
     * incomplet situé à la frontière.
     *
     * Son nouveau point final doit donc être
     * replacé en (1,0).
     */
    Pattern reducedHalf =
        ContourReducer::normalizeOpenPath(
            reductionResult.pattern
        );

    /*
     * La pile perd les niveaux qui viennent
     * réellement d'être retirés.
     */
    substitutionSegmentCounts =
        reductionResult
            .remainingSubstitutionSegmentCounts;

    std::optional<Pattern> nextMotif;

    const std::size_t nextOpenPatternIndex =
        getOpenPatternIndex(
            cycleIndex + 1
        );

    /*
     * ÉTAPE 4
     *
     * Le prochain motif ouvert remplace les
     * segments de la moitié rétrogradée.
     */
    if (cycleIndex + 1 < closedPaths.size())
    {
        const Pattern& nextOpenPattern =
            openPatterns[
                nextOpenPatternIndex
            ];

        const std::size_t nextOpenSegmentCount =
            nextOpenPattern
                .getSegments()
                .size();

        const std::size_t
            estimatedNextMotifSegments =
                checkedSegmentProduct(
                    nextOpenSegmentCount,

                    reducedHalf
                        .getSegments()
                        .size()
                );

        if (
            estimatedNextMotifSegments
            <= maximumSegmentCount
        )
        {
            nextMotif =
                GeometryMacro::substitute(
                    nextOpenPattern,
                    reducedHalf,
                    "recursive-open-pattern-"
                        + std::to_string(
                            cycleIndex + 1
                        )
                );

            currentMotif =
                *nextMotif;

            /*
             * Le motif injecté devient le niveau
             * le plus récent de la pile.
             */
            substitutionSegmentCounts.push_back(
                nextOpenSegmentCount
            );
        }
        else
        {
            /*
             * Le cycle courant est valide, mais le
             * prochain motif serait trop volumineux.
             */
            safetyLimitReached = true;
        }
    }

    /*
     * Historique complet utilisé par le nouveau
     * Renderer.
     */
    ProjectionCycleResult cycleResult{
        cycleIndex,
        nextOpenPatternIndex,
        inputMotif,
        currentClosedPath,
        closedGeometry,
        halfContour,
        reducedHalf,
        requestedRollbackLevelCount,
        reductionResult.appliedLevelCount,
        nextMotif
    };

    cycles.push_back(
        std::move(cycleResult)
    );

    /*
     * Historique simplifié conservé pour
     * compatibilité avec les appels existants.
     */
    GeometryMacroResult displayResult{
        cycleIndex,
        inputMotif.getName(),
        currentClosedPath.getName(),
        closedGeometry
    };

    history.push_back(
        displayResult
    );

    ++currentIndex;

    return displayResult;
}

const std::vector<GeometryMacroResult>&
ProjectionEngine::run()
{
    while (
        currentIndex < closedPaths.size()
        && !safetyLimitReached
    )
    {
        if (!canProcessNext())
        {
            safetyLimitReached = true;
            break;
        }

        step();
    }

    return history;
}

void ProjectionEngine::reset()
{
    currentIndex = 0;

    safetyLimitReached = false;

    history.clear();
    cycles.clear();

    currentMotif =
        openPatterns.front();

    substitutionSegmentCounts.clear();

    substitutionSegmentCounts.push_back(
        openPatterns.front()
            .getSegments()
            .size()
    );
}

std::size_t
ProjectionEngine::getCurrentIndex() const
{
    return currentIndex;
}

std::size_t
ProjectionEngine::getTotalStepCount() const
{
    return closedPaths.size();
}

std::size_t
ProjectionEngine::estimateNextClosedSegmentCount()
    const
{
    if (currentIndex >= closedPaths.size())
    {
        return 0;
    }

    return checkedSegmentProduct(
        currentMotif
            .getSegments()
            .size(),

        closedPaths[currentIndex]
            .getSegments()
            .size()
    );
}

bool ProjectionEngine::hasReachedSafetyLimit() const
{
    return safetyLimitReached;
}

std::size_t
ProjectionEngine::getMaximumSegmentCount() const
{
    return maximumSegmentCount;
}

const Pattern&
ProjectionEngine::getCurrentMotif() const
{
    return currentMotif;
}

const Pattern&
ProjectionEngine::getCurrentClosedPath() const
{
    if (currentIndex >= closedPaths.size())
    {
        throw std::out_of_range(
            "Aucun contour ferme courant : "
            "le parcours est termine."
        );
    }

    return closedPaths[currentIndex];
}

const std::vector<Pattern>&
ProjectionEngine::getOpenPatterns() const
{
    return openPatterns;
}

const std::vector<Pattern>&
ProjectionEngine::getClosedPaths() const
{
    return closedPaths;
}

const std::vector<GeometryMacroResult>&
ProjectionEngine::getHistory() const
{
    return history;
}

const std::vector<ProjectionCycleResult>&
ProjectionEngine::getCycles() const
{
    return cycles;
}

void ProjectionEngine::validatePatternLists() const
{
    if (openPatterns.empty())
    {
        throw std::invalid_argument(
            "La liste des motifs ouverts "
            "ne peut pas etre vide."
        );
    }

    if (closedPaths.empty())
    {
        throw std::invalid_argument(
            "La liste des contours fermes "
            "ne peut pas etre vide."
        );
    }

    for (
        std::size_t index = 0;
        index < openPatterns.size();
        ++index
    )
    {
        const Pattern& pattern =
            openPatterns[index];

        pattern.validate();

        if (!pattern.isOpen())
        {
            throw std::invalid_argument(
                "Le motif "
                + pattern.getName()
                + " de la liste ouverte est ferme."
            );
        }
    }

    std::size_t previousSideCount = 0;

    for (
        std::size_t index = 0;
        index < closedPaths.size();
        ++index
    )
    {
        const Pattern& pattern =
            closedPaths[index];

        pattern.validate();

        if (!pattern.isClosed())
        {
            throw std::invalid_argument(
                "Le motif "
                + pattern.getName()
                + " de la liste fermee est ouvert."
            );
        }

        const std::size_t sideCount =
            pattern
                .getSegments()
                .size();

        if (
            index > 0
            && sideCount <= previousSideCount
        )
        {
            throw std::invalid_argument(
                "Les contours fermes doivent etre "
                "classes par nombre de cotes "
                "strictement croissant."
            );
        }

        previousSideCount =
            sideCount;
    }
}

std::size_t
ProjectionEngine::getOpenPatternIndex(
    std::size_t recursionIndex
) const
{
    return recursionIndex
        % openPatterns.size();
}

std::size_t
ProjectionEngine::checkedSegmentProduct(
    std::size_t left,
    std::size_t right
)
{
    if (
        left != 0
        && right
            > std::numeric_limits<
                  std::size_t
              >::max()
                / left
    )
    {
        throw std::overflow_error(
            "Depassement de capacite pendant "
            "le calcul du nombre de segments."
        );
    }

    return left * right;
}