#pragma once

#include "RecursionReducer.hpp"
#include "GeometryMacro.hpp"
#include "Pattern.hpp"

#include <cstddef>
#include <optional>
#include <vector>

struct ProjectionCycleResult
{
    std::size_t recursionIndex = 0;
    std::size_t injectedOpenPatternIndex = 0;

    Pattern inputMotif;
    Pattern closedPath;

    Pattern closedGeometry;
    Pattern halfContour;

    /*
     * Moitié obtenue après rétrogradation
     * et nouvelle normalisation.
     */
    Pattern reducedHalf;

    std::size_t requestedRollbackLevelCount = 0;
    std::size_t appliedRollbackLevelCount = 0;

    std::optional<Pattern> nextMotif;
};

class ProjectionEngine
{
public:
    ProjectionEngine(
        std::vector<Pattern> openPatterns,
        std::vector<Pattern> closedPaths,
        std::size_t maximumSegmentCount = 2'000'000
    );

    bool hasNext() const;
    bool canProcessNext() const;

    GeometryMacroResult step();

    const std::vector<GeometryMacroResult>& run();

    void reset();

    std::size_t getCurrentIndex() const;
    std::size_t getTotalStepCount() const;

    std::size_t estimateNextClosedSegmentCount() const;

    bool hasReachedSafetyLimit() const;
    std::size_t getMaximumSegmentCount() const;

    const Pattern& getCurrentMotif() const;
    const Pattern& getCurrentClosedPath() const;

    const std::vector<Pattern>&
    getOpenPatterns() const;

    const std::vector<Pattern>&
    getClosedPaths() const;

    /*
     * Historique simplifié temporairement utilisé
     * par le Renderer et le main.cpp actuels.
     */
    const std::vector<GeometryMacroResult>&
    getHistory() const;

    /*
     * Historique complet du nouveau pipeline.
     */
    const std::vector<ProjectionCycleResult>&
    getCycles() const;

private:
    std::vector<Pattern> openPatterns;
    std::vector<Pattern> closedPaths;
    
    std::vector<std::size_t> substitutionSegmentCounts;

    std::vector<GeometryMacroResult> history;
    std::vector<ProjectionCycleResult> cycles;

    Pattern currentMotif;

    std::size_t currentIndex = 0;
    std::size_t maximumSegmentCount = 0;

    bool safetyLimitReached = false;

    void validatePatternLists() const;

    std::size_t getOpenPatternIndex(
        std::size_t recursionIndex
    ) const;

    static std::size_t checkedSegmentProduct(
        std::size_t left,
        std::size_t right
    );
};