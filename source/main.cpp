#include "Pattern.hpp"
#include "PolygonGenerator.hpp"
#include "ProjectionEngine.hpp"
#include "Renderer.hpp"

#include <SFML/Graphics.hpp>

#include <exception>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace
{
    std::vector<Pattern> loadOpenPatterns(
        const std::filesystem::path& directory
    )
    {
        const std::vector<std::string> fileNames{
            "water-pattern.json",
            "fire-pattern.json",
            "wind-pattern.json",
            "wood-pattern.json",
            "earth-pattern.json",
            "ice-pattern.json",
            "magma-pattern.json",
            "lightning-pattern.json"
        };

        std::vector<Pattern> patterns;

        patterns.reserve(
            fileNames.size()
        );

        for (
            const std::string& fileName :
            fileNames
        )
        {
            Pattern pattern =
                Pattern::loadFromJson(
                    directory / fileName
                );

            if (!pattern.isOpen())
            {
                throw std::runtime_error(
                    fileName
                    + " ne definit pas un "
                      "motif ouvert."
                );
            }

            patterns.push_back(
                std::move(pattern)
            );
        }

        return patterns;
    }

    std::string getStageName(
        RenderStage stage
    )
    {
        switch (stage)
        {
            case RenderStage::ClosedPath:
                return "1: closed path";

            case RenderStage::ClosedGeometry:
                return "2: closed geometry";

            case RenderStage::HalfContour:
                return "3: normalized half";

            case RenderStage::NextMotif:
                return "4: next recursive motif";
        }

        return "unknown";
    }

    void updateWindowTitle(
        sf::RenderWindow& window,
        const ProjectionCycleResult& cycle,
        std::size_t displayedCycleIndex,
        std::size_t cycleCount,
        RenderStage stage,
        bool automaticAnimation,
        bool safetyLimitReached
    )
    {
        std::string title =
            "Geometry Projection Builder | Cycle "
            + std::to_string(
                displayedCycleIndex + 1
            )
            + "/"
            + std::to_string(cycleCount)
            + " | "
            + cycle.inputMotif.getName()
            + " -> "
            + cycle.closedPath.getName()
            + " | "
            + getStageName(stage);

        if (automaticAnimation)
        {
            title += " | AUTO";
        }

        if (safetyLimitReached)
        {
            title += " | SEGMENT LIMIT";
        }

        window.setTitle(
            sf::String(title)
        );
    }

    RenderStage nextStage(
        RenderStage stage
    )
    {
        switch (stage)
        {
            case RenderStage::ClosedPath:
                return RenderStage::ClosedGeometry;

            case RenderStage::ClosedGeometry:
                return RenderStage::HalfContour;

            case RenderStage::HalfContour:
                return RenderStage::NextMotif;

            case RenderStage::NextMotif:
                return RenderStage::ClosedPath;
        }

        return RenderStage::ClosedGeometry;
    }
}

int main()
{
    try
    {
        const std::filesystem::path projectRoot =
            PROJECT_SOURCE_DIR_PATH;

        const std::filesystem::path
            openPatternDirectory =
                projectRoot
                / "patterns"
                / "openPath";

        const std::filesystem::path
            closedPathDirectory =
                projectRoot
                / "patterns"
                / "closePath";

        std::vector<Pattern> openPatterns =
            loadOpenPatterns(
                openPatternDirectory
            );

        std::vector<Pattern> closedPaths =
            PolygonGenerator::loadDirectory(
                closedPathDirectory,
                3,
                10
            );

        /*
         * La limite empêche la création accidentelle
         * de dizaines de millions de segments.
         */
        ProjectionEngine engine(
            std::move(openPatterns),
            std::move(closedPaths),
            2'000'000
        );

        engine.run();

        const std::vector<ProjectionCycleResult>&
            cycles =
                engine.getCycles();

        if (cycles.empty())
        {
            throw std::runtime_error(
                "Le moteur n'a produit aucun cycle."
            );
        }

        if (engine.hasReachedSafetyLimit())
        {
            std::cerr
                << "Limite de securite atteinte apres "
                << cycles.size()
                << " cycle(s). Limite : "
                << engine.getMaximumSegmentCount()
                << " segments.\n";
        }

        sf::RenderWindow window(
            sf::VideoMode(
                {
                    1280,
                    720
                }
            ),
            "Geometry Projection Builder"
        );

        window.setFramerateLimit(60);

        Renderer renderer(70.0f);

        std::size_t displayedCycleIndex = 0;

        RenderStage displayedStage =
            RenderStage::ClosedGeometry;

        bool automaticAnimation = false;

        constexpr float automaticDelay =
            2.0f;

        sf::Clock automaticClock;

        updateWindowTitle(
            window,
            cycles[displayedCycleIndex],
            displayedCycleIndex,
            cycles.size(),
            displayedStage,
            automaticAnimation,
            engine.hasReachedSafetyLimit()
        );

        while (window.isOpen())
        {
            while (
                const auto event =
                    window.pollEvent()
            )
            {
                if (
                    event->is<
                        sf::Event::Closed
                    >()
                )
                {
                    window.close();
                }

                if (
                    const auto* keyPressed =
                        event->getIf<
                            sf::Event::KeyPressed
                        >()
                )
                {
                    const auto key =
                        keyPressed->scancode;

                    bool displayChanged = false;

                    if (
                        key
                        == sf::Keyboard::Scancode::Escape
                    )
                    {
                        window.close();
                    }

                    if (
                        key
                            == sf::Keyboard::Scancode::Right
                        || key
                            == sf::Keyboard::Scancode::Space
                    )
                    {
                        displayedCycleIndex =
                            (
                                displayedCycleIndex + 1
                            )
                            % cycles.size();

                        automaticClock.restart();
                        displayChanged = true;
                    }

                    if (
                        key
                        == sf::Keyboard::Scancode::Left
                    )
                    {
                        if (displayedCycleIndex == 0)
                        {
                            displayedCycleIndex =
                                cycles.size() - 1;
                        }
                        else
                        {
                            --displayedCycleIndex;
                        }

                        automaticClock.restart();
                        displayChanged = true;
                    }

                    if (
                        key
                        == sf::Keyboard::Scancode::Num1
                    )
                    {
                        displayedStage =
                            RenderStage::ClosedPath;

                        displayChanged = true;
                    }

                    if (
                        key
                        == sf::Keyboard::Scancode::Num2
                    )
                    {
                        displayedStage =
                            RenderStage::ClosedGeometry;

                        displayChanged = true;
                    }

                    if (
                        key
                        == sf::Keyboard::Scancode::Num3
                    )
                    {
                        displayedStage =
                            RenderStage::HalfContour;

                        displayChanged = true;
                    }

                    if (
                        key
                        == sf::Keyboard::Scancode::Num4
                    )
                    {
                        displayedStage =
                            RenderStage::NextMotif;

                        displayChanged = true;
                    }

                    if (
                        key
                        == sf::Keyboard::Scancode::Tab
                    )
                    {
                        displayedStage =
                            nextStage(displayedStage);

                        displayChanged = true;
                    }

                    if (
                        key
                        == sf::Keyboard::Scancode::A
                    )
                    {
                        automaticAnimation =
                            !automaticAnimation;

                        automaticClock.restart();
                        displayChanged = true;
                    }

                    if (
                        key
                        == sf::Keyboard::Scancode::R
                    )
                    {
                        displayedCycleIndex = 0;

                        displayedStage =
                            RenderStage::ClosedGeometry;

                        automaticAnimation = false;

                        automaticClock.restart();
                        displayChanged = true;
                    }

                    if (displayChanged)
                    {
                        updateWindowTitle(
                            window,
                            cycles[
                                displayedCycleIndex
                            ],
                            displayedCycleIndex,
                            cycles.size(),
                            displayedStage,
                            automaticAnimation,
                            engine
                                .hasReachedSafetyLimit()
                        );
                    }
                }
            }

            if (
                automaticAnimation
                && automaticClock
                       .getElapsedTime()
                       .asSeconds()
                    >= automaticDelay
            )
            {
                displayedStage =
                    nextStage(displayedStage);

                if (
                    displayedStage
                    == RenderStage::ClosedPath
                )
                {
                    displayedCycleIndex =
                        (
                            displayedCycleIndex + 1
                        )
                        % cycles.size();
                }

                automaticClock.restart();

                updateWindowTitle(
                    window,
                    cycles[displayedCycleIndex],
                    displayedCycleIndex,
                    cycles.size(),
                    displayedStage,
                    automaticAnimation,
                    engine.hasReachedSafetyLimit()
                );
            }

            window.clear(
                sf::Color(18, 21, 28)
            );

            renderer.drawCycle(
                window,
                cycles[displayedCycleIndex],
                displayedStage
            );

            window.display();
        }

        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr
            << "\nErreur pendant l'initialisation :\n"
            << error.what()
            << "\n\nAppuyez sur Entree pour fermer...\n";

        std::cin.get();

        return 1;
    }
}
