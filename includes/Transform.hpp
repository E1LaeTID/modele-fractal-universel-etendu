#pragma once

#include "Pattern.hpp"
#include "Point.hpp"

class Transform
{
public:
    static Point translatePoint(
        const Point& point,
        double offsetX,
        double offsetY
    );

    static Point rotatePoint(
        const Point& point,
        double angleRadians,
        const Point& pivot
    );

    static Point scalePoint(
        const Point& point,
        double scaleX,
        double scaleY,
        const Point& pivot
    );

    static Pattern translatePattern(
        const Pattern& pattern,
        double offsetX,
        double offsetY
    );

    static Pattern rotatePattern(
        const Pattern& pattern,
        double angleRadians,
        const Point& pivot
    );

    static Pattern rotatePatternAroundOrigin(
        const Pattern& pattern,
        double angleRadians
    );

    static Pattern scalePattern(
        const Pattern& pattern,
        double scaleX,
        double scaleY,
        const Point& pivot
    );

    static Pattern scalePatternFromOrigin(
        const Pattern& pattern,
        double scaleX,
        double scaleY
    );

    static Point mapPointToSegment(
        const Point& localPoint,
        const Point& sourceOrigin,
        const Point& targetStart,
        const Point& targetEnd
    );

    static Pattern mapPatternToSegment(
        const Pattern& sourcePattern,
        const Point& targetStart,
        const Point& targetEnd
    );

    static double distance(
        const Point& start,
        const Point& end
    );

    static double segmentAngle(
        const Point& start,
        const Point& end
    );
};