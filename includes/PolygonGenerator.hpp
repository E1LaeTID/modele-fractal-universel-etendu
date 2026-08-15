#pragma once

#include "Pattern.hpp"

#include <cstddef>
#include <filesystem>
#include <vector>

class PolygonGenerator
{
public:
    static Pattern loadFromJson(
        const std::filesystem::path& filePath
    );

    static std::vector<Pattern> loadDirectory(
        const std::filesystem::path& directoryPath,
        std::size_t minimumSideCount = 3,
        std::size_t maximumSideCount = 10
    );

    static void validateRegularPolygon(
        const Pattern& polygon,
        double tolerance = 1e-9
    );

private:
    static double segmentLength(
        const Pattern& polygon,
        const Segment& segment
    );

    static void validatePointOrder(
        const Pattern& polygon
    );

    static void validateSegmentOrder(
        const Pattern& polygon
    );
};