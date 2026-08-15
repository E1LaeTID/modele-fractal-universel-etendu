#pragma once

#include "Pattern.hpp"

#include <cstddef>
#include <string>
#include <vector>

struct GeometryMacroResult
{
    std::size_t recursionIndex = 0;

    std::string openPatternName;
    std::string targetPathName;

    Pattern geometry;
};

class GeometryMacro
{
public:
    /*
     * Interface temporairement conservée pour que
     * l'ancien ProjectionEngine continue à compiler.
     */
    GeometryMacro(
        Pattern openPattern,
        Pattern targetPath,
        std::size_t recursionIndex
    );

    GeometryMacroResult execute() const;

    const Pattern& getOpenPattern() const;
    const Pattern& getTargetPath() const;

    /*
     * Alias temporaire utilisé par l'ancienne
     * version du moteur.
     */
    const Pattern& getClosedPath() const;

    std::size_t getRecursionIndex() const;

    /*
     * Nouvelle interface générique.
     *
     * sourcePattern doit être ouvert.
     * targetPath peut être ouvert ou fermé.
     *
     * La topologie du résultat est celle
     * du chemin cible.
     */
    static Pattern substitute(
        const Pattern& sourcePattern,
        const Pattern& targetPath,
        const std::string& resultName = {}
    );

private:
    Pattern openPattern;
    Pattern targetPath;

    std::size_t recursionIndex = 0;

    static void validateInputs(
        const Pattern& sourcePattern,
        const Pattern& targetPath
    );

    static std::vector<std::size_t>
    buildOrderedPointIndices(
        const Pattern& pattern
    );

    static void validateNormalizedOpenPattern(
        const Pattern& pattern,
        double tolerance = 1e-9
    );
};