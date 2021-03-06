#pragma once
#include "EngineManager.h"
#include <string>

enum ItemFlag {
    NoFlag         = 0,
    Tree           = 1,
    House          = 2,
    TileFlagLength = 3
};

enum QuadrantFlag {
    ForestQuadrant  = 0,
    VillageQuadrant = 1,
    CityQuadrant    = 2,
    QuadrantLength  = 3
};

struct QuandrantBuilder {

    QuadrantFlag quadrantType;
    int          probabilityToPlaceHouse;
    int          probabilityToPlaceTree;
    int          probabilityToForkPath;
};

// First define the extents of the world and various other world building constants
constexpr int   tileWidth               = 200;
constexpr int   tileLength              = 200;
constexpr int   tileHalfWidth           = tileWidth  / 2;
constexpr int   tileHalfLength          = tileLength / 2;
constexpr int   widthOfWorld            = tileWidth  * 10;
constexpr int   lengthOfWorld           = tileLength * 10;
constexpr int   numWidthTiles           = widthOfWorld   / tileWidth;
constexpr int   numLengthTiles          = lengthOfWorld  / tileLength;
constexpr int   minWidthValue           = -(widthOfWorld  / 2) + tileHalfWidth;
constexpr int   minLengthValue          = -(lengthOfWorld / 2) + tileHalfLength;
constexpr float maxRandomValue          = static_cast<float>(RAND_MAX);
constexpr int   pathPixelRadius         = 10;
constexpr int   pathPixelDiameter       = pathPixelRadius * 2;
constexpr bool  highlightHeadOfPath     = false;
constexpr int   numWidthPathIds         = widthOfWorld  / pathPixelRadius;
constexpr int   numLengthPathIds        = lengthOfWorld / pathPixelRadius;
constexpr int   widthOfBuilderQuadrant  = widthOfWorld  / 2;
constexpr int   lengthOfBuilderQuadrant = lengthOfWorld / 2;
constexpr int   numWidthQuadrants       = widthOfWorld  / widthOfBuilderQuadrant;
constexpr int   numLengthQuadrants      = lengthOfWorld / lengthOfBuilderQuadrant;
constexpr int   typesOfTrees            = 3;
constexpr int   minPathGenToCreatePath  = 5;

// Radial pathing data
// 360 degrees of rotation for the next path direction
constexpr int   fullCircleInDegrees         = 360;
constexpr int   rotationPathOffsetInDegrees = 5;
constexpr int   rotationPathVariations      = fullCircleInDegrees / rotationPathOffsetInDegrees;
constexpr float degToRad                    = PI / 180.0f;

class Builder {

protected:

    void _paintTile(int          widthLocation,
                    int          lengthLocation,
                    int          tileWidthIndex,
                    int          tileLengthIndex,
                    std::string  name);

    void _paintTiles(int         widthLocation,
                     int         lengthLocation,
                     int         tileWidthIndex,
                     int         tileLengthIndex,
                     std::string name);

    int                          _prevPathLengthLocation;
    int                          _prevPathWidthLocation;
    int                          _currRotationInDegrees;
    int                          _prevTileLengthIndex;
    int                          _prevTileWidthIndex;
    int                          _pathLengthLocation;
    int                          _pathWidthLocation;
    bool                         _proceduralGenDone;
    int                          _tileLengthIndex;
    int                          _tileWidthIndex;
    std::vector<Builder*>        _pathGenerators;
    int                          _pathTileCount;
    int                          _parentPathId;
    std::string                  _sceneName;
    int                          _pathId;

    static bool                  _clutterPassFinished;
    static int                   _prevConcurrentPaths;
    static bool                  _allPathsFinished;
    static int                   _concurrentPaths;
    static int                   _pathIdAllocator;

public:

    Builder(std::string sceneName,
            int         pathId,
            int         parentPathId    = TileFlagLength,
            int         widthLocation   = 0,
            int         lengthLocation  = 0,
            int         pathDirection   = -100000000,          // magic init number for rotation
            int         tileWidthIndex  = numWidthTiles  / 2,
            int         tileLengthIndex = numLengthTiles / 2);

    ~Builder();

    // Debug aid to observe how the path creation is done
    void buildPath();

    static bool isAllPathsFinished()    { return _allPathsFinished; }
    static void initPathingData()       { _concurrentPaths  = 0;
                                          _allPathsFinished = true; }
    static void updatePathCount()       { _prevConcurrentPaths = _concurrentPaths; }
    static int  getNewPathId()          { return _pathIdAllocator++; };
};


class WorldGenerator {

public:

    WorldGenerator(std::string sceneName);
    ~WorldGenerator();
    void generateWorldTiles();
    void clutterPass();

    static void spawnPaths(std::string sceneName);
    static int  getEntityId(int tileWidthIndex,
                            int tileLengthIndex) {

        return _entityIDMap[tileWidthIndex][tileLengthIndex];
    }
    static int  getItemId(int tileWidthIndex,
                          int tileLengthIndex) {

        return _tiledPathIds[tileWidthIndex][tileLengthIndex];
    }
    static bool setItemId(int tileWidthIndex,
                          int tileLengthIndex,
                          int item) {

        if ((tileWidthIndex                                 >= 0)              &&
            (tileLengthIndex                                >= 0)              &&
            (tileWidthIndex                                 < numWidthPathIds) &&
            (tileLengthIndex                                < numWidthPathIds) &&
            (_dirtiedTiles[tileWidthIndex][tileLengthIndex] == false)) {

            _dirtiedTiles[tileWidthIndex][tileLengthIndex] = true;
            _tiledPathIds[tileWidthIndex][tileLengthIndex] = item;
            return true;
        }
        return false;
    }
    static QuandrantBuilder getQuadrant(int tileWidthIndex,
                                        int pathWidthLocation,
                                        int tileLengthIndex,
                                        int pathLengthLocation) {

        int widthIndex  = ((tileWidthIndex  * tileWidth)  + pathWidthLocation ) / widthOfBuilderQuadrant;
        int lengthIndex = ((tileLengthIndex * tileLength) + pathLengthLocation) / lengthOfBuilderQuadrant;
        return _builderQuadrants[widthIndex][lengthIndex];
    }

private:

    std::string     _sceneName;
    static bool     _fileSaved;
    static Builder* _seedPath;
    static bool     _finishedClutterPass;


    // Identifies a tile within the grid's entity ID for access
    static int              _entityIDMap[numWidthTiles][numLengthTiles];
    // Flags used to identify which path id is placed to detect intersections of other paths
    static int              _tiledPathIds[numWidthPathIds][numLengthPathIds];
    // Keeps track of which tiles have already been written
    static bool             _dirtiedTiles[numWidthPathIds][numLengthPathIds];
    // Quadrants within the world to indicate different types of procedural generation
    static QuandrantBuilder _builderQuadrants[numWidthQuadrants][numLengthQuadrants];

    QuandrantBuilder quadrantBuilder[QuadrantLength] =
    {
        //               Quadrant type    House Tree  Path
        QuandrantBuilder{ForestQuadrant,  5000, 200,  100},
        QuandrantBuilder{VillageQuadrant, 150,  1000, 50},
        QuandrantBuilder{CityQuadrant,    50,   5000, 20},
    };

};