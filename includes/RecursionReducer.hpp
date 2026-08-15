#pragma once

#include "Pattern.hpp"

#include <cstddef>
#include <vector>

struct RecursionReductionResult
{
    Pattern pattern;

    std::size_t requestedLevelCount = 0;
    std::size_t appliedLevelCount = 0;

    std::vector<std::size_t>
        remainingSubstitutionSegmentCounts;
};

class RecursionReducer
{
public:
    static RecursionReductionResult rollback(
        const Pattern& openGeometry,
        const std::vector<std::size_t>&
            substitutionSegmentCounts,
        std::size_t requestedLevelCount
    );

private:
    static Pattern collapseLastLevel(
        const Pattern& openGeometry,
        std::size_t sourceSegmentCount,
        std::size_t rollbackLevel
    );
};