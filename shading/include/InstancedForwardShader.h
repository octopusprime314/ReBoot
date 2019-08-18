/*
* InstancedForwardShader is part of the ReBoot distribution (https://github.com/octopusprime314/ReBoot.git).
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
*  InstancedForwardShader class. Draws many instances using the static shader and adds offsets.
*/

#pragma once
#include "ForwardShader.h"
#include "Vector4.h"
#include <vector>
class ViewEventDistributor;
class ShadowRenderer;
class PointShadow;

class InstancedForwardShader : public ForwardShader {

public:
    InstancedForwardShader(std::string shaderName);
    virtual ~InstancedForwardShader();
    virtual void runShader(Entity*               entity,
                           ViewEventDistributor* viewEventDistributor,
                           std::vector<Light*>&  lights);
};
