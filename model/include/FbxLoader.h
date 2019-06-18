/*
* FbxLoader is part of the ReBoot distribution (https://github.com/octopusprime314/ReBoot.git).
* Copyright (c) 2017 Peter Morley.
*
* ReBoot is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, version 3.
*
* ReBoot is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

/**
*  FbxLoader class. Loads and converts fbx data into ReBoot engine format
*/

#pragma once
#include <string>
#include "fbxsdk.h"
#include <vector>
#include "Tex2.h"
#include <map>
#include "Matrix.h"

class Model;
class AnimatedModel;
class SkinningData;
class Vector4;
class Entity;

struct FbxExporterType {
    FbxManager*     manager;
    FbxIOSettings*  ioSettings;
    FbxScene*       scene;
    FbxExporter*    exporter;
};

class FbxLoader {
    using ClonedCount = std::map<std::string, unsigned int>;
    using ClonedMatrices = std::map<std::string, Matrix>;
    using TileTextures = std::map<std::string, std::vector<std::string>>;
    FbxManager*     _fbxManager;
    FbxIOSettings*  _ioSettings;
    FbxScene*       _scene;
    FbxExporterType _export;
    std::string     _fileName;
    int             _strideIndex;
    bool            _copiedOverFlag; //indicates the scene has been cloned over to export fbx to prevent doing it again!
    void            _loadTextures(Model* model, FbxMesh* meshNode, FbxNode* childNode);
    void            _buildTriangles(Model* model, std::vector<Vector4>& vertices, std::vector<Vector4>& normals,
        std::vector<Tex2>& textures, std::vector<int>& indices, FbxNode* node);
    void            _buildModelData(Model* model, FbxMesh* meshNode, FbxNode* childNode, std::vector<Vector4>& vertices,
        std::vector<Vector4>& normals, std::vector<Tex2>& textures);
    void            _buildGeometryData(Model* model, std::vector<Vector4>& vertices, std::vector<int>& indices, FbxNode* node);
    void            _nodeExists(std::string modelName, FbxNode* node, std::vector<FbxNode*>& nodes);
    void            _generateTextureStrides(FbxMesh* meshNode, std::vector<std::pair<int, int>>& textureStrides);
    bool            _loadTexture(Model* model, int textureStride, FbxFileTexture* textureFbx);
    bool            _loadLayeredTexture(Model* model, int textureStride, FbxLayeredTexture* layered_texture);
    void            _loadTextureUVs(FbxMesh* meshNode, std::vector<Tex2>& textures);
    void            _loadNormals(FbxMesh* meshNode, int* indices, std::vector<Vector4>& normals);
    void            _loadVertices(FbxMesh* meshNode, std::vector<Vector4>& vertices);
    void            _loadIndices(Model* model, FbxMesh* meshNode, int*& indices);
    void            _cloneFbxNode(Model* modelAddedTo, FbxLoader* fbxToAdd, Vector4 location, Vector4 rotation);
    int             _getASCIIFormatIndex(FbxManager* fbxManager);
    void            _parseTags(FbxNode* node);
    void            _searchAndEditNode(FbxNode* rootNode, FbxNode* childNode, std::string modelName, 
                                       Vector4 location, Vector4 rotation);
    void            _searchAndEditMesh(FbxNode* childNode, std::string modelName, 
                                       Vector4 location, Vector4 rotation);
    Matrix          _objectSpaceTransform;
    ClonedCount     _clonedInstances;
    ClonedMatrices  _clonedWorldTransforms;
    TileTextures    _tileTextures;

public:
    FbxLoader(std::string name);
    ~FbxLoader();
    FbxScene*       getScene();
    void            loadAnimatedModel(AnimatedModel* model, FbxNode* node = nullptr);
    void            loadAnimatedModelData(AnimatedModel* model, FbxSkin* pSkin, FbxNode* node, FbxMesh* mesh);
    void            loadModel(Model* model, FbxNode* node = nullptr);
    void            loadModelData(Model* model, FbxMesh* meshNode, FbxNode* childNode);
    void            buildAnimationFrames(AnimatedModel* model, std::vector<SkinningData>& skins);
    void            buildAABB(Model* model);
    void            loadGeometry(Model* model, FbxNode* node);
    void            loadGeometryData(Model* model, FbxMesh* meshNode, FbxNode* childNode);
    void            addToScene(Model* modelAddedTo, FbxLoader* modelToLoad, 
                               Vector4 location, Vector4 rotation);
    void            removeFromScene(Entity* entityToRemove, FbxLoader* modelRemovedFrom);
    void            addTileToScene(Model* modelAddedTo, FbxLoader* modelToLoad, 
                                   Vector4 location, std::vector<std::string> textures);
    void            saveScene();
    void            clearScene();
    std::string     getModelName();
    Matrix          getObjectSpaceTransform();
};