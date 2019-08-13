/*
* PointShadow is part of the ReBoot distribution (https://github.com/octopusprime314/ReBoot.git).
* Copyright (c) 2018 Peter Morley.
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
*  PointShadow class. Uses a cube map texture class from point lights in the scene
*  to generate shadows.
*/

#pragma once

#include "ShadowAnimatedPointShader.h"
#include "CubeMapRenderer.h"
class Entity;

class PointShadow : public CubeMapRenderer {

     //Animated Shader that generates point light cube map shadows
    ShadowAnimatedPointShader* _pointAnimatedShadowShader;
    //Shader that generates point light cube map shadows
    ShadowPointShader*         _pointShadowShader;

public:
    PointShadow(GLuint width,
                GLuint height);
    ~PointShadow();

    void                       render(std::vector<Entity*> entityList,
                                      Light*               light);
};