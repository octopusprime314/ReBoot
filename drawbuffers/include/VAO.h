/*
* VBO is part of the ReBoot distribution (https://github.com/octopusprime314/ReBoot.git).
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
*  VBO class. Vertex Buffer Object container
*/

#pragma once
#include "GLIncludes.h"
#include <vector>
#include "Vector4.h"
#include "Tex2.h"
#include "RenderBuffers.h"
#include "Animation.h"
#include "Sphere.h"
#include "Triangle.h"
#include "Cube.h"
#include <d3d12.h>
#include "d3dx12.h"
#include <wrl.h>
#include "ResourceBuffer.h"

using namespace Microsoft::WRL;

enum class GeometryConstruction;
enum class ModelClass; //Forward declaration of enumerated type while not including Model class
using TextureMetaData = std::vector<std::pair<std::string, int>>;
class VAO {

    GLuint          _indexBufferContext; // Used as the index buffer
    GLuint          _vertexBufferContext; //Used as the vertex attribute vbo context
    GLuint          _normalBufferContext; //Used as the normal attribute vbo context
    GLuint          _textureBufferContext; //Used as the texture coordinate attribute vbo context
    GLuint          _debugNormalBufferContext; //Used as the debug normal line attribute vbo context
    GLuint          _vaoContext;
    GLuint          _vaoShadowContext;
    GLuint          _indexContext;
    GLuint          _weightContext;
    GLuint          _vertexLength;
    TextureMetaData _textureStride;
    GLuint          _primitiveOffsetId;
    ResourceBuffer*                          _vertexBuffer;
    ResourceBuffer*                          _indexBuffer;
    D3D12_INDEX_BUFFER_VIEW                  _ibv;
    D3D12_VERTEX_BUFFER_VIEW                 _vbv;
    D3D12_INDEX_BUFFER_VIEW                  _shadowIBV;
    D3D12_VERTEX_BUFFER_VIEW                 _shadowVBV;
public:
    VAO();
    VAO(D3D12_VERTEX_BUFFER_VIEW vbv, D3D12_INDEX_BUFFER_VIEW ibv);
    ~VAO();
    GLuint          getVAOContext();
    GLuint          getVAOShadowContext();
    GLuint          getVertexContext();
    GLuint          getNormalContext();
    GLuint          getTextureContext();
    GLuint          getVertexLength();
    GLuint          getPrimitiveOffsetId();
    GLuint          getNormalDebugContext();
    TextureMetaData getTextureStrides();
    D3D12_INDEX_BUFFER_VIEW getIndexBuffer();
    D3D12_VERTEX_BUFFER_VIEW getVertexBuffer();
    void            setVertexContext(GLuint context);
    void            setNormalContext(GLuint context);
    void            setTextureContext(GLuint context);
    void            setNormalDebugContext(GLuint context);
    void            addTextureStride(std::pair<std::string, int> stride);
    void            setPrimitiveOffsetId(GLuint id);
    void            createVAO(RenderBuffers* renderBuffers, ModelClass classId, Animation* = nullptr);
    void            createVAO(RenderBuffers* renderBuffers, int begin, int range); //special for frustum viewing
    void            createVAO(std::vector<Sphere>* spheres, GeometryConstruction geometryType);
    void            createVAO(std::vector<Triangle>* triangles);
    void            createVAO(std::vector<Cube>* cubes, GeometryConstruction geometryType);
};