#pragma once

#include "Point.hpp"
#include "Segment.hpp"

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

enum class PatternTopology
{
    Open,
    Closed
};

class Pattern
{
public:
    Pattern() = default;

    Pattern(
        std::string name,
        std::string category,
        std::string description,
        PatternTopology topology,
        std::string originId,
        std::string endPointId,
        std::vector<Point> points,
        std::vector<Segment> segments
    );

    static Pattern loadFromJson(
        const std::filesystem::path& filePath
    );

    const std::string& getName() const;
    const std::string& getCategory() const;
    const std::string& getDescription() const;

    PatternTopology getTopology() const;

    const std::string& getOriginId() const;
    const std::string& getEndPointId() const;

    const std::vector<Point>& getPoints() const;
    const std::vector<Segment>& getSegments() const;

    const Point& getPoint(std::size_t index) const;
    const Point& getPointById(const std::string& id) const;

    std::size_t getPointIndex(
        const std::string& id
    ) const;

    bool isOpen() const;
    bool isClosed() const;
    bool empty() const;

    void validate() const;

private:
    std::string name;
    std::string category;
    std::string description;

    PatternTopology topology = PatternTopology::Open;

    std::string originId;
    std::string endPointId;

    std::vector<Point> points;
    std::vector<Segment> segments;
};