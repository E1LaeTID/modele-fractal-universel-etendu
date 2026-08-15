#include "ContourReducer.hpp"

#include <cmath>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

Pattern ContourReducer::extractFirstHalf(
    const Pattern& closedGeometry
)
{
    closedGeometry.validate();

    if (!closedGeometry.isClosed())
    {
        throw std::invalid_argument(
            "ContourReducer::extractFirstHalf "
            "attend une geometrie fermee."
        );
    }

    const std::vector<Segment>& sourceSegments =
        closedGeometry.getSegments();

    if (sourceSegments.empty())
    {
        throw std::invalid_argument(
            "Impossible de decouper un contour "
            "sans segment."
        );
    }

    /*
     * Si le nombre de segments est impair,
     * on utilise le nombre pair immédiatement
     * inférieur.
     *
     * Exemple :
     *
     * 21 segments
     *     ↓
     * 20 segments appairables
     *     ↓
     * moitié conservée : 10 segments
     *
     * Le segment frontière impair est exclu.
     */
    const std::size_t usableSegmentCount =
        sourceSegments.size()
        - (
            sourceSegments.size() % 2
        );

    const std::size_t halfSegmentCount =
        usableSegmentCount / 2;

    if (halfSegmentCount == 0)
    {
        throw std::runtime_error(
            "Le contour "
            + closedGeometry.getName()
            + " ne contient pas suffisamment "
              "de segments pour extraire "
              "une moitie."
        );
    }

    std::vector<Point> halfPoints;
    std::vector<Segment> halfSegments;

    halfPoints.reserve(
        halfSegmentCount + 1
    );

    halfSegments.reserve(
        halfSegmentCount
    );

    /*
     * Le parcours commence au point initial du
     * premier segment, normalement M0.
     */
    const std::size_t firstSourcePointIndex =
        sourceSegments.front().startIndex;

    Point firstPoint =
        closedGeometry.getPoint(
            firstSourcePointIndex
        );

    firstPoint.id = "M0";

    halfPoints.push_back(
        std::move(firstPoint)
    );

    /*
     * Seuls les premiers halfSegmentCount
     * segments sont conservés.
     */
    for (
        std::size_t segmentIndex = 0;
        segmentIndex < halfSegmentCount;
        ++segmentIndex
    )
    {
        const Segment& sourceSegment =
            sourceSegments[segmentIndex];

        /*
         * Vérification locale de la continuité
         * du chemin extrait.
         */
        if (
            segmentIndex > 0
            && sourceSegments[segmentIndex - 1]
                   .endIndex
                != sourceSegment.startIndex
        )
        {
            throw std::runtime_error(
                "Le contour "
                + closedGeometry.getName()
                + " n'est pas continu au segment "
                + std::to_string(segmentIndex)
                + "."
            );
        }

        Point nextPoint =
            closedGeometry.getPoint(
                sourceSegment.endIndex
            );

        const std::size_t newPointIndex =
            halfPoints.size();

        nextPoint.id =
            "M"
            + std::to_string(newPointIndex);

        halfPoints.push_back(
            std::move(nextPoint)
        );

        halfSegments.push_back(
            Segment{
                newPointIndex - 1,
                newPointIndex
            }
        );
    }

    const std::string endPointId =
        "M"
        + std::to_string(
            halfPoints.size() - 1
        );

    return Pattern(
        closedGeometry.getName()
            + "-first-half",

        closedGeometry.getCategory()
            + "-half",

        "Premiere moitie ordonnee du contour "
            + closedGeometry.getName()
            + ". Nombre de segments source : "
            + std::to_string(
                sourceSegments.size()
            )
            + ". Nombre de segments conserves : "
            + std::to_string(
                halfSegmentCount
            )
            + ".",

        PatternTopology::Open,

        "M0",
        endPointId,

        std::move(halfPoints),
        std::move(halfSegments)
    );
}

Pattern ContourReducer::normalizeOpenPath(
    const Pattern& openPath
)
{
    openPath.validate();

    if (!openPath.isOpen())
    {
        throw std::invalid_argument(
            "ContourReducer::normalizeOpenPath "
            "attend un chemin ouvert."
        );
    }

    const Point& origin =
        openPath.getPointById(
            openPath.getOriginId()
        );

    const Point& endPoint =
        openPath.getPointById(
            openPath.getEndPointId()
        );

    /*
     * U relie le premier point au dernier.
     */
    const double basisUX =
        endPoint.normalizedX
        - origin.normalizedX;

    const double basisUY =
        endPoint.normalizedY
        - origin.normalizedY;

    const double squaredLength =
        basisUX * basisUX
        + basisUY * basisUY;

    constexpr double minimumSquaredLength =
        1e-18;

    if (squaredLength <= minimumSquaredLength)
    {
        throw std::runtime_error(
            "Impossible de normaliser "
            + openPath.getName()
            + " : son origine et son point final "
              "sont confondus."
        );
    }

    /*
     * V est la rotation antihoraire de U
     * d'un angle de 90 degres.
     */
    const double basisVX =
        -basisUY;

    const double basisVY =
        basisUX;

    std::vector<Point> normalizedPoints;

    normalizedPoints.reserve(
        openPath.getPoints().size()
    );

    for (
        const Point& point :
        openPath.getPoints()
    )
    {
        const double translatedX =
            point.normalizedX
            - origin.normalizedX;

        const double translatedY =
            point.normalizedY
            - origin.normalizedY;

        /*
         * Projection de P-A dans la base (U,V).
         *
         * x' = dot(P-A, U) / |U|²
         * y' = dot(P-A, V) / |U|²
         */
        const double normalizedX =
            (
                translatedX * basisUX
                + translatedY * basisUY
            )
            / squaredLength;

        const double normalizedY =
            (
                translatedX * basisVX
                + translatedY * basisVY
            )
            / squaredLength;

        normalizedPoints.push_back(
            Point{
                point.id,
                normalizedX,
                normalizedY
            }
        );
    }

    return Pattern(
        openPath.getName()
            + "-normalized",

        openPath.getCategory(),

        "Version normalisee du chemin "
            + openPath.getName()
            + ", avec origine (0,0) "
              "et point final (1,0).",

        PatternTopology::Open,

        openPath.getOriginId(),
        openPath.getEndPointId(),

        std::move(normalizedPoints),
        openPath.getSegments()
    );
}

Pattern ContourReducer::extractNormalizedHalf(
    const Pattern& closedGeometry
)
{
    Pattern firstHalf =
        extractFirstHalf(
            closedGeometry
        );

    return normalizeOpenPath(
        firstHalf
    );
}