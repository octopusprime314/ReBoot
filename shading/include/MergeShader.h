/*
* MergeShader is part of the ReBoot distribution (https://github.com/octopusprime314/ReBoot.git).
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
*  MergeShader class. Takes multiple frame buffers and merges them
*/

#pragma once
#include "Shader.h"
#include "Matrix.h"
#include "MVP.h"
class Light;
class Water;
class Effect;
class ViewManager;

class MergeShader : public Shader {

protected:
    GLint       _deferredTextureLocation;
    GLint       _bloomTextureLocation;
    GLuint      _dummyVAO;
public:
    MergeShader();
    virtual ~MergeShader();
    virtual void runShader(GLuint deferredTexture, GLuint bloomTexture);
};