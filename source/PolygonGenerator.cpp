#include "PolygonGenerator.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

Pattern PolygonGenerator::loadFromJson(
    const std::filesystem::path& filePath
)
{
    Pattern polygon =
        Pattern::loadFromJson(filePath);

    validateRegularPolygon(polygon);

    return polygon;
}

std::vector<Pattern> PolygonGenerator::loadDirectory(
    const std::filesystem::path& directoryPath,
    std::size_t minimumSideCount,
    std::size_t maximumSideCount
)
{
    if (minimumSideCount < 3)
    {
        throw std::invalid_argument(
            "Un polygone doit posseder au moins "
            "trois cotes."
        );
    }

    if (minimumSideCount > maximumSideCount)
    {
        throw std::invalid_argument(
            "Le nombre minimal de cotes ne peut pas "
            "etre superieur au nombre maximal."
        );
    }

    if (!std::filesystem::exists(directoryPath))
    {
        throw std::runtime_error(
            "Le dossier des contours fermes "
            "n'existe pas : "
            + directoryPath.string()
        );
    }

    if (!std::filesystem::is_directory(directoryPath))
    {
        throw std::runtime_error(
            "Le chemin des contours fermes "
            "n'est pas un dossier : "
            + directoryPath.string()
        );
    }

    std::vector<Pattern> polygons;

    for (
        const std::filesystem::directory_entry& entry :
        std::filesystem::directory_iterator(directoryPath)
    )
    {
        if (!entry.is_regular_file())
        {
            continue;
        }

        if (entry.path().extension() != ".json")
        {
            continue;
        }

        Pattern polygon =
            loadFromJson(entry.path());

        const std::size_t sideCount =
            polygon.getSegments().size();

        if (
            sideCount < minimumSideCount
            || sideCount > maximumSideCount
        )
        {
            continue;
        }

        polygons.push_back(
            std::move(polygon)
        );
    }

    std::sort(
        polygons.begin(),
        polygons.end(),
        [](const Pattern& left, const Pattern& right)
        {
            return left.getSegments().size()
                < right.getSegments().size();
        }
    );

    std::unordered_map<std::size_t, std::string>
        polygonNamesBySideCount;

    for (const Pattern& polygon : polygons)
    {
        const std::size_t sideCount =
            polygon.getSegments().size();

        const auto iterator =
            polygonNamesBySideCount.find(sideCount);

        if (iterator != polygonNamesBySideCount.end())
        {
            throw std::runtime_error(
                "Plusieurs contours fermes possedent "
                + std::to_string(sideCount)
                + " cotes : "
                + iterator->second
                + " et "
                + polygon.getName()
            );
        }

        polygonNamesBySideCount.emplace(
            sideCount,
            polygon.getName()
        );
    }

    const std::size_t expectedCount =
        maximumSideCount
        - minimumSideCount
        + 1;

    if (polygons.size() != expectedCount)
    {
        throw std::runtime_error(
            "Le dossier doit contenir exactement "
            + std::to_string(expectedCount)
            + " polygones reguliers, de "
            + std::to_string(minimumSideCount)
            + " a "
            + std::to_string(maximumSideCount)
            + " cotes. Nombre charge : "
            + std::to_string(polygons.size())
        );
    }

    for (
        std::size_t expectedSideCount =
            minimumSideCount;

        expectedSideCount <= maximumSideCount;

        ++expectedSideCount
    )
    {
        const bool found =
            std::any_of(
                polygons.begin(),
                polygons.end(),
                [expectedSideCount](
                    const Pattern& polygon
                )
                {
                    return polygon
                        .getSegments()
                        .size()
                        == expectedSideCount;
                }
            );

        if (!found)
        {
            throw std::runtime_error(
                "Le polygone regulier a "
                + std::to_string(expectedSideCount)
                + " cotes est absent."
            );
        }
    }

    return polygons;
}

void PolygonGenerator::validateRegularPolygon(
    const Pattern& polygon,
    double tolerance
)
{
    if (tolerance <= 0.0)
    {
        throw std::invalid_argument(
            "La tolerance de validation doit "
            "etre strictement positive."
        );
    }

    polygon.validate();

    if (!polygon.isClosed())
    {
        throw std::runtime_error(
            "Le motif "
            + polygon.getName()
            + " n'est pas un contour ferme."
        );
    }

    const std::size_t pointCount =
        polygon.getPoints().size();

    const std::size_t segmentCount =
        polygon.getSegments().size();

    if (pointCount < 3)
    {
        throw std::runtime_error(
            "Le contour "
            + polygon.getName()
            + " possede moins de trois points."
        );
    }

    if (pointCount != segmentCount)
    {
        throw std::runtime_error(
            "Le contour "
            + polygon.getName()
            + " doit posseder autant de points "
              "que de segments."
        );
    }

    if (polygon.getOriginId() != "M0")
    {
        throw std::runtime_error(
            "L'origine du contour "
            + polygon.getName()
            + " doit etre M0."
        );
    }

    if (polygon.getEndPointId() != "M0")
    {
        throw std::runtime_error(
            "Le point final du contour "
            + polygon.getName()
            + " doit revenir sur M0."
        );
    }

    const Point& origin =
        polygon.getPointById("M0");

    if (
        std::abs(origin.normalizedX) > tolerance
        || std::abs(origin.normalizedY) > tolerance
    )
    {
        throw std::runtime_error(
            "M0 doit posseder les coordonnees "
            "normalisees (0, 0) dans "
            + polygon.getName()
        );
    }

    validatePointOrder(polygon);
    validateSegmentOrder(polygon);

    const std::vector<Segment>& segments =
        polygon.getSegments();

    const double referenceLength =
        segmentLength(
            polygon,
            segments.front()
        );

    if (referenceLength <= tolerance)
    {
        throw std::runtime_error(
            "Le premier segment du contour "
            + polygon.getName()
            + " possede une longueur nulle."
        );
    }

    for (
        std::size_t index = 0;
        index < segments.size();
        ++index
    )
    {
        const double currentLength =
            segmentLength(
                polygon,
                segments[index]
            );

        if (
            std::abs(
                currentLength - referenceLength
            ) > tolerance
        )
        {
            throw std::runtime_error(
                "Le segment "
                + std::to_string(index)
                + " du contour "
                + polygon.getName()
                + " ne possede pas la longueur "
                  "commune L."
            );
        }
    }

    if (
        std::abs(referenceLength - 1.0)
        > tolerance
    )
    {
        throw std::runtime_error(
            "La longueur normalisee des cotes de "
            + polygon.getName()
            + " doit etre egale a 1."
        );
    }
}

double PolygonGenerator::segmentLength(
    const Pattern& polygon,
    const Segment& segment
)
{
    const Point& start =
        polygon.getPoint(segment.startIndex);

    const Point& end =
        polygon.getPoint(segment.endIndex);

    const double deltaX =
        end.normalizedX - start.normalizedX;

    const double deltaY =
        end.normalizedY - start.normalizedY;

    return std::hypot(
        deltaX,
        deltaY
    );
}

void PolygonGenerator::validatePointOrder(
    const Pattern& polygon
)
{
    const std::vector<Point>& points =
        polygon.getPoints();

    for (
        std::size_t index = 0;
        index < points.size();
        ++index
    )
    {
        const std::string expectedId =
            "M" + std::to_string(index);

        if (points[index].id != expectedId)
        {
            throw std::runtime_error(
                "Ordre de points invalide dans "
                + polygon.getName()
                + ". Le point d'indice "
                + std::to_string(index)
                + " doit etre "
                + expectedId
                + ", mais il est "
                + points[index].id
                + "."
            );
        }
    }
}

void PolygonGenerator::validateSegmentOrder(
    const Pattern& polygon
)
{
    const std::vector<Segment>& segments =
        polygon.getSegments();

    const std::size_t pointCount =
        polygon.getPoints().size();

    for (
        std::size_t index = 0;
        index < segments.size();
        ++index
    )
    {
        const std::size_t expectedStart =
            index;

        const std::size_t expectedEnd =
            (index + 1) % pointCount;

        if (
            segments[index].startIndex
                != expectedStart
            || segments[index].endIndex
                != expectedEnd
        )
        {
            throw std::runtime_error(
                "Ordre de segments invalide dans "
                + polygon.getName()
                + " au segment "
                + std::to_string(index)
                + "."
            );
        }
    }
}