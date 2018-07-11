/*
* Model is part of the ReBoot distribution (https://github.com/octopusprime314/ReBoot.git).
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
*  Model class. Contains vertex and normal data which will get populated in opengl
*  buffers to be rendered to the screen
*/

#pragma once
#include "Matrix.h"
#include "Tex2.h"
#include "StateVector.h"
#include <vector>
#include "GLIncludes.h"
#include "StaticShader.h"
#include "DebugShader.h"
#include <iostream>
#include "FbxLoader.h"
#include "MasterClock.h"
#include "TextureBroker.h"
#include "Geometry.h"
#include "VAO.h"
#include "MVP.h"
#include "RenderBuffers.h"
#include "ForwardShader.h"
#include <mutex>
class Entity;
class SimpleContext;

enum class ModelClass {
    ModelType = 0,
    AnimatedModelType
};

using TextureMetaData = std::vector<std::pair<std::string, int>>;

class Model {

public:

    //Default model to type to base class
    Model(std::string name, ModelClass classId = ModelClass::ModelType);
    virtual ~Model();
    VAO*                        getVAO();
    RenderBuffers*              getRenderBuffers();
    ModelClass                  getClassType();
    size_t                      getArrayCount();
    void                        addTexture(std::string textureName, int stride);
    void                        addLayeredTexture(std::vector<std::string> textureNames, int stride);
    AssetTexture*               getTexture(std::string textureName);
    LayeredTexture*             getLayeredTexture(std::string textureName);
    TextureMetaData&            getTextureStrides();
    GeometryType                getGeometryType();
    Geometry*                   getGeometry();
    void                        addGeometryTriangle(Triangle triangle);
    void                        addGeometrySphere(Sphere sphere);
    void                        setInstances(std::vector<Vector4> offsets); //is this model used for instancing
    bool                        getIsInstancedModel();
    float*                      getInstanceOffsets();
    void                        runShader(Entity* entity);
    virtual void                updateModel(Model* model);

protected:
    RenderBuffers               _renderBuffers; //Manages vertex, normal and texture data
    VAO                         _vao; //Vao container
    StaticShader*               _shaderProgram; //Container object of the Model's shader
    FbxLoader*                  _fbxLoader; //Used to load fbx data and parse it into engine format
    ModelClass                  _classId; //Used to identify which class is being used
    static TextureBroker*       _textureManager; //Static texture manager for texture reuse purposes, all models have access
    TextureMetaData             _textureStrides; //Keeps track of which set of vertices use a certain texture within the large vertex set
    GeometryType                _geometryType; //Indicates whether the collision geometry is sphere or triangle based
    Geometry                    _geometry; //Geometry object that contains all collision information for a model
    bool                        _isInstanced;
    float                       _offsets[900]; //300 x, y and z offsets
    int                         _instances;
    std::string                 _getModelName(std::string name);
    std::mutex                  _updateLock;
};