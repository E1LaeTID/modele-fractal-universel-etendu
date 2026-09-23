# Modèle fractal universel étendu — cycles entre motifs ouverts et contours fermés

Le **modèle fractal universel étendu** est un moteur C++ de construction géométrique récursive. Il orchestre deux listes — des motifs ouverts et des contours fermés — puis transforme le résultat de chaque cycle en source du cycle suivant.

Il prolonge le [modèle fractal universel initial](https://github.com/E1LaeTID/Un-modele-fractale-universel), qui applique un même motif normalisé à chaque segment d'un chemin.

> English summary: an experimental C++ geometry engine that cycles through open patterns and closed contours, reduces closed results back to open paths and reuses them as inputs for the next recursive stage.

**Technologies :** C++17, CMake, SFML 3, JSON  
**Statut :** prototype expérimental en cours de stabilisation

![Architecture du modèle fractal universel étendu](docs/images/schema-architecture-modele-fractal-universel-etendu.png)

## Ce que la version étendue ajoute

| Modèle initial | Modèle étendu |
|---|---|
| Un motif ouvert | Une liste ordonnée de motifs ouverts |
| Un chemin cible | Une liste ordonnée de contours fermés |
| Substitution répétée | Enchaînement de cycles différents |
| Résultat final affiché | Résultat réduit et réinjecté |
| Croissance récursive | Rétrogradation structurelle contrôlée |

Le modèle étendu sépare quatre opérations :

1. projeter un motif ouvert sur un contour ;
2. obtenir une géométrie fermée substituée ;
3. extraire et normaliser une moitié ouverte ;
4. rétrograder la récursion avant le cycle suivant.

## Les deux listes

```text
OpenPatterns =
[Eau, Feu, Vent, Bois, Terre, Glace, Magmat, Foudre]

ClosedPaths =
[Triangle, Carré, Pentagone, Hexagone,
 Heptagone, Octogone, Ennéagone, Décagone]
```

À chaque cycle, un motif ouvert et un contour fermé sont combinés selon leur position dans les listes.

## Transformation affine fondamentale

Pour un segment cible `[A, B]` :

```text
U = B - A
V = (-Uy, Ux)
P' = A + xU + yV
```

Le motif local commence en `(0,0)` et se termine en `(1,0)`. Son origine est projetée sur `A` et son extrémité sur `B`.

## Pipeline d'un cycle

```text
CurrentMotif[n] + ClosedPath[n]
                ↓
       substitution géométrique
                ↓
        ClosedGeometry[n]
                ↓
    extraction d'une moitié ouverte
                ↓
           HalfContour[n]
                ↓
    rétrogradation de n niveaux
                ↓
           ReducedHalf[n]
                ↓
 normalisation de (0,0) vers (1,0)
                ↓
 substitution par OpenPattern[n+1]
                ↓
        CurrentMotif[n+1]
```

Formulation compacte :

```text
G[n] = Substitute(CurrentMotif[n], ClosedPath[n])
H[n] = Normalize(ExtractHalf(G[n]))
R[n] = Normalize(Rollback(H[n], n))
CurrentMotif[n+1] = Substitute(OpenPattern[n+1], R[n])
```

## Conversion fermé-vers-ouvert

`ContourReducer` extrait la première moitié du contour substitué.

Pour un nombre pair :

```text
24 segments fermés → 12 segments ouverts
```

Pour un nombre impair, le segment frontière non appairé est exclu :

```text
21 segments → 20 segments appairables → 10 segments ouverts
```

Le résultat est ensuite normalisé entre `(0,0)` et `(1,0)`.

## Rétrogradation structurelle

`RecursionReducer` regroupe les segments enfants complets afin de reconstruire leur segment parent.

```text
M segments enfants → 1 segment parent
```

La profondeur demandée suit l'indice du contour :

| Cycle | Contour | Niveaux demandés |
|---:|---|---:|
| 0 | Triangle | 0 |
| 1 | Carré | 1 |
| 2 | Pentagone | 2 |
| 3 | Hexagone | 3 |
| 4 | Heptagone | 4 |
| 5 | Octogone | 5 |
| 6 | Ennéagone | 6 |
| 7 | Décagone | 7 |

La rétrogradation n'est pas un inverse analytique de la géométrie. Elle utilise l'historique structurel des substitutions et ne reconstruit que les groupes complets disponibles.

## Architecture C++

| Composant | Responsabilité |
|---|---|
| `Point` | Coordonnées normalisées et identifiant |
| `Segment` | Connectivité entre deux points |
| `Pattern` | Représentation des motifs ouverts et fermés |
| `PolygonGenerator` | Chargement et validation des contours |
| `Transform` | Projection affine |
| `GeometryMacro` | Substitution sur un chemin cible |
| `ContourReducer` | Extraction fermé-vers-ouvert |
| `RecursionReducer` | Reconstruction des niveaux parents |
| `ProjectionEngine` | Orchestration des listes et cycles |
| `Renderer` | Affichage SFML |

## Organisation JSON

Motif ouvert :

```json
{
  "name": "wood-pattern",
  "element": "wood",
  "origin": "M0",
  "endPoint": "M8",
  "points": [],
  "segments": []
}
```

Contour fermé :

```json
{
  "name": "hexagon-closed-path",
  "closedPathType": "hexagon",
  "origin": "M0",
  "endPoint": "M0",
  "points": [],
  "segments": [
    ["M0", "M1"],
    ["M1", "M2"],
    ["M2", "M3"],
    ["M3", "M4"],
    ["M4", "M5"],
    ["M5", "M0"]
  ]
}
```

## Compilation

Prérequis actuellement documentés :

- Windows 10 ou 11 ;
- compilateur compatible C++17 ;
- CMake ;
- SFML 3.1.0 ;
- Git pour charger `nlohmann/json`.

```bash
cmake -S . -B build
cmake --build build --config Debug
```

Le chemin SFML peut devoir être adapté dans `CMakeLists.txt` selon votre installation.

## Commandes du renderer

| Touche | Action |
|---|---|
| `1` | Contour fermé source |
| `2` | Géométrie fermée substituée |
| `3` | Moitié extraite et normalisée |
| `4` | Motif récursif suivant |
| `Tab` | État suivant du cycle |
| `→` ou `Espace` | Cycle suivant |
| `←` | Cycle précédent |
| `A` | Activer ou arrêter l'animation |
| `R` | Revenir au premier cycle |
| `Échap` | Fermer |

## Limite de sécurité

La croissance peut être très rapide. `ProjectionEngine` applique une limite configurable du nombre de segments. Cette limite doit rester active même lorsque la rétrogradation réduit la géométrie.

Mesures recommandées :

- borner le nombre de segments ;
- vérifier les groupes avant rétrogradation ;
- journaliser la taille de chaque cycle ;
- interrompre proprement une génération excessive ;
- conserver des jeux de données et graines reproductibles.

## Applications documentées

Le modèle étendu sert de base conceptuelle à plusieurs expérimentations :

- [time2d](https://github.com/E1LaeTID/time2d), pour les structures temporelles polyfractales ;
- [Carte calorique multilangage](https://github.com/E1LaeTID/carte-calorique-multilangage), pour la cartographie de listes métier ;
- [ElementChess](https://github.com/E1LaeTID/elementchess-fractal-poc), pour la transformation stochastique d'un jeu de stratégie.

Ces dépôts sont des cas d'application. Ils ne constituent pas des preuves que le modèle convient automatiquement à tous les domaines.

## Questions fréquentes

### Quelle est la différence entre le modèle initial et le modèle étendu ?

Le modèle initial substitue un motif sur des segments. Le modèle étendu orchestre deux listes de géométries, réduit le résultat et le réinjecte dans le cycle suivant.

### Pourquoi utiliser des contours fermés ?

Ils fournissent des supports successifs et clairement identifiables. La conversion en chemin ouvert permet ensuite de produire une nouvelle source normalisée.

### Que signifie rétrograder une récursion ?

Le moteur regroupe des ensembles complets de segments enfants pour retrouver leurs segments parents selon l'historique de génération.

### Le modèle garantit-il l'absence d'intersections ?

Non. La validité dépend des motifs, contours, profondeurs et transformations choisis. Les résultats doivent être contrôlés.

### Pourquoi imposer une limite de segments ?

La substitution peut produire une croissance exponentielle et saturer la mémoire avant que la rétrogradation ne suffise à la contenir.

### Est-ce une théorie mathématique démontrée ?

Le dépôt documente un modèle informatique expérimental et ses invariants de construction. Il ne revendique pas une preuve d'universalité pour tous les systèmes fractals.

### Peut-on utiliser un autre renderer que SFML ?

Oui en principe. Les données JSON et les opérations géométriques sont séparées du rendu, mais un nouvel adaptateur doit être implémenté.

## Feuille de route

- stabiliser les huit cycles ;
- afficher séparément `ReducedHalf` ;
- enregistrer les statistiques par cycle ;
- exporter en SVG ;
- comparer les stratégies de réduction ;
- relier des sections pour produire des maillages 3D ;
- rendre le moteur indépendant du renderer.

## Statut et écosystème

Prototype C++ expérimental.

- [Modèle fractal universel initial](https://github.com/E1LaeTID/Un-modele-fractale-universel)
- [time2d](https://github.com/E1LaeTID/time2d)
- [ElementChess](https://github.com/E1LaeTID/elementchess-fractal-poc)
- [Portail E1LaeTID](https://e1laetid.github.io/)

## Licence

Distribué sous licence **MIT**. Consultez [LICENSE](LICENSE).
