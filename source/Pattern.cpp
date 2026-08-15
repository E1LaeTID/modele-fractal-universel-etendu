#include "Pattern.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace
{
    using json = nlohmann::json;

    std::string trim(const std::string& value)
    {
        const auto first = std::find_if_not(
            value.begin(),
            value.end(),
            [](unsigned char character)
            {
                return std::isspace(character);
            }
        );

        const auto last = std::find_if_not(
            value.rbegin(),
            value.rend(),
            [](unsigned char character)
            {
                return std::isspace(character);
            }
        ).base();

        if (first >= last)
        {
            return {};
        }

        return std::string(first, last);
    }

    double parseCoordinate(const json& value)
    {
        if (value.is_number())
        {
            return value.get<double>();
        }

        if (!value.is_string())
        {
            throw std::runtime_error(
                "Une coordonnee normalisee doit etre un nombre "
                "ou une chaine numerique."
            );
        }

        const std::string expression =
            trim(value.get<std::string>());

        if (expression.empty())
        {
            throw std::runtime_error(
                "Une coordonnee normalisee est vide."
            );
        }

        const std::size_t separator =
            expression.find('/');

        if (separator == std::string::npos)
        {
            return std::stod(expression);
        }

        if (
            expression.find('/', separator + 1)
            != std::string::npos
        )
        {
            throw std::runtime_error(
                "Fraction invalide : " + expression
            );
        }

        const std::string numeratorText =
            trim(expression.substr(0, separator));

        const std::string denominatorText =
            trim(expression.substr(separator + 1));

        const double numerator =
            std::stod(numeratorText);

        const double denominator =
            std::stod(denominatorText);

        if (denominator == 0.0)
        {
            throw std::runtime_error(
                "Division par zero dans la fraction : "
                + expression
            );
        }

        return numerator / denominator;
    }

    PatternTopology detectTopology(const json& document)
    {
        if (document.contains("closedPathType"))
        {
            return PatternTopology::Closed;
        }

        return PatternTopology::Open;
    }

    std::string readCategory(
        const json& document,
        PatternTopology topology
    )
    {
        if (topology == PatternTopology::Closed)
        {
            return document.at(
                "closedPathType"
            ).get<std::string>();
        }

        return document.at(
            "element"
        ).get<std::string>();
    }
}

Pattern::Pattern(
    std::string patternName,
    std::string patternCategory,
    std::string patternDescription,
    PatternTopology patternTopology,
    std::string patternOriginId,
    std::string patternEndPointId,
    std::vector<Point> patternPoints,
    std::vector<Segment> patternSegments
)
    : name(std::move(patternName)),
      category(std::move(patternCategory)),
      description(std::move(patternDescription)),
      topology(patternTopology),
      originId(std::move(patternOriginId)),
      endPointId(std::move(patternEndPointId)),
      points(std::move(patternPoints)),
      segments(std::move(patternSegments))
{
    validate();
}

Pattern Pattern::loadFromJson(
    const std::filesystem::path& filePath
)
{
    std::ifstream inputFile(filePath);

    if (!inputFile.is_open())
    {
        throw std::runtime_error(
            "Impossible d'ouvrir le fichier JSON : "
            + filePath.string()
        );
    }

    json document;

    try
    {
        inputFile >> document;
    }
    catch (const json::parse_error& error)
    {
        throw std::runtime_error(
            "JSON invalide dans "
            + filePath.string()
            + " : "
            + error.what()
        );
    }

    const PatternTopology topology =
        detectTopology(document);

    const std::string name =
        document.at("name").get<std::string>();

    const std::string category =
        readCategory(document, topology);

    const std::string description =
        document.value(
            "description",
            std::string{}
        );

    const std::string originId =
        document.at("origin").get<std::string>();

    const std::string endPointId =
        document.at("endPoint").get<std::string>();

    std::vector<Point> points;
    std::unordered_map<std::string, std::size_t>
        pointIndices;

    const json& pointsDocument =
        document.at("points");

    points.reserve(pointsDocument.size());

    for (const json& pointDocument : pointsDocument)
    {
        const std::string id =
            pointDocument.at("id").get<std::string>();

        if (
            pointIndices.find(id)
            != pointIndices.end()
        )
        {
            throw std::runtime_error(
                "Identifiant de point duplique : " + id
            );
        }

        const json& normalized =
            pointDocument.at("normalized");

        if (
            !normalized.is_array()
            || normalized.size() != 2
        )
        {
            throw std::runtime_error(
                "Le point "
                + id
                + " doit contenir exactement "
                  "deux coordonnees normalisees."
            );
        }

        Point point;

        point.id = id;
        point.normalizedX =
            parseCoordinate(normalized.at(0));

        point.normalizedY =
            parseCoordinate(normalized.at(1));

        pointIndices.emplace(
            point.id,
            points.size()
        );

        points.push_back(std::move(point));
    }

    std::vector<Segment> segments;

    const json& segmentsDocument =
        document.at("segments");

    segments.reserve(segmentsDocument.size());

    for (
        const json& segmentDocument :
        segmentsDocument
    )
    {
        if (
            !segmentDocument.is_array()
            || segmentDocument.size() != 2
        )
        {
            throw std::runtime_error(
                "Chaque segment doit contenir "
                "exactement deux identifiants."
            );
        }

        const std::string startId =
            segmentDocument.at(0).get<std::string>();

        const std::string endId =
            segmentDocument.at(1).get<std::string>();

        const auto startIterator =
            pointIndices.find(startId);

        const auto endIterator =
            pointIndices.find(endId);

        if (startIterator == pointIndices.end())
        {
            throw std::runtime_error(
                "Point initial introuvable : "
                + startId
            );
        }

        if (endIterator == pointIndices.end())
        {
            throw std::runtime_error(
                "Point final introuvable : "
                + endId
            );
        }

        segments.push_back(
            {
                startIterator->second,
                endIterator->second
            }
        );
    }

    return Pattern(
        name,
        category,
        description,
        topology,
        originId,
        endPointId,
        std::move(points),
        std::move(segments)
    );
}

const std::string& Pattern::getName() const
{
    return name;
}

const std::string& Pattern::getCategory() const
{
    return category;
}

const std::string& Pattern::getDescription() const
{
    return description;
}

PatternTopology Pattern::getTopology() const
{
    return topology;
}

const std::string& Pattern::getOriginId() const
{
    return originId;
}

const std::string& Pattern::getEndPointId() const
{
    return endPointId;
}

const std::vector<Point>& Pattern::getPoints() const
{
    return points;
}

const std::vector<Segment>& Pattern::getSegments() const
{
    return segments;
}

const Point& Pattern::getPoint(
    std::size_t index
) const
{
    if (index >= points.size())
    {
        throw std::out_of_range(
            "Indice de point hors limites."
        );
    }

    return points[index];
}

const Point& Pattern::getPointById(
    const std::string& id
) const
{
    return getPoint(
        getPointIndex(id)
    );
}

std::size_t Pattern::getPointIndex(
    const std::string& id
) const
{
    const auto iterator = std::find_if(
        points.begin(),
        points.end(),
        [&id](const Point& point)
        {
            return point.id == id;
        }
    );

    if (iterator == points.end())
    {
        throw std::out_of_range(
            "Point introuvable : " + id
        );
    }

    return static_cast<std::size_t>(
        std::distance(points.begin(), iterator)
    );
}

bool Pattern::isOpen() const
{
    return topology == PatternTopology::Open;
}

bool Pattern::isClosed() const
{
    return topology == PatternTopology::Closed;
}

bool Pattern::empty() const
{
    return points.empty();
}

void Pattern::validate() const
{
    if (name.empty())
    {
        throw std::runtime_error(
            "Le motif doit posseder un nom."
        );
    }

    if (category.empty())
    {
        throw std::runtime_error(
            "Le motif doit posseder une categorie."
        );
    }

    if (points.empty())
    {
        throw std::runtime_error(
            "Le motif ne contient aucun point."
        );
    }

    if (segments.empty())
    {
        throw std::runtime_error(
            "Le motif ne contient aucun segment."
        );
    }

    getPointById(originId);
    getPointById(endPointId);

    for (const Segment& segment : segments)
    {
        if (
            segment.startIndex >= points.size()
            || segment.endIndex >= points.size()
        )
        {
            throw std::runtime_error(
                "Un segment reference un point inexistant."
            );
        }

        if (segment.startIndex == segment.endIndex)
        {
            throw std::runtime_error(
                "Un segment ne peut pas relier "
                "un point a lui-meme."
            );
        }
    }

    if (isOpen())
    {
        if (segments.size() + 1 != points.size())
        {
            throw std::runtime_error(
                "Un motif ouvert doit contenir "
                "exactement points.size() - 1 segments."
            );
        }

        if (
            segments.front().startIndex
            != getPointIndex(originId)
        )
        {
            throw std::runtime_error(
                "Le premier segment du motif ouvert "
                "doit commencer a l'origine."
            );
        }

        if (
            segments.back().endIndex
            != getPointIndex(endPointId)
        )
        {
            throw std::runtime_error(
                "Le dernier segment du motif ouvert "
                "doit terminer au point final."
            );
        }
    }

    if (isClosed())
    {
        if (segments.size() != points.size())
        {
            throw std::runtime_error(
                "Un contour ferme doit contenir "
                "autant de segments que de points."
            );
        }

        const std::size_t originIndex =
            getPointIndex(originId);

        if (
            segments.front().startIndex
            != originIndex
        )
        {
            throw std::runtime_error(
                "Le premier segment du contour ferme "
                "doit commencer en M0."
            );
        }

        if (
            segments.back().endIndex
            != originIndex
        )
        {
            throw std::runtime_error(
                "Le dernier segment du contour ferme "
                "doit revenir en M0."
            );
        }
    }

    for (
        std::size_t index = 1;
        index < segments.size();
        ++index
    )
    {
        if (
            segments[index - 1].endIndex
            != segments[index].startIndex
        )
        {
            throw std::runtime_error(
                "Les segments ne forment pas "
                "une chaine continue."
            );
        }
    }
}