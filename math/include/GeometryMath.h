/*
* GeometryMath is part of the ReBoot distribution (https://github.com/octopusprime314/ReBoot.git).
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
*  static GeometryMath class. Contains algorithms for geometry interactions
*/

#pragma once
#include "Model.h"
#include "Cube.h"
class GeometryMath {

    static float   _max(float a, float b);
    static float   _min(float a, float b);
    static Vector4 _closestPoint(Sphere* sphere, Triangle* triangle);
public:
    //Early out collision helper functions
    static bool sphereProtrudesCube(Sphere* sphere, Cube* cube); //Returns true if the sphere is not completely enclosed within a cube

    //Collision detection functions
    static bool spheresSpheresDetection(Geometry *spheresGeometryA, Geometry *spheresGeometryB); //Test all model A's spheres against all model B's spheres
    static bool spheresTrianglesDetection(Geometry* spheresGeometry, Geometry* triangleGeometry); //Test all model A's spheres against all model B's triangles
    static bool sphereCubeDetection(Sphere *sphere, Cube *cube); //Test a single sphere against a single cube
    static bool triangleCubeDetection(Triangle* triangle, Cube* cube); //Test a single triangle against a single cube
    static bool sphereTriangleDetection(Sphere& sphere, Triangle& triangle); //Returns true if a sphere and triangle overlap
    static bool sphereSphereDetection(Sphere& sphereA, Sphere& sphereB); //Returns true if a sphere and a sphere overlap
    static bool frustumAABBDetection(std::vector<Vector4> planes, Vector4 &mins, Vector4 &maxs);
    static void getFrustumPlanes(Matrix inverseViewProjection, std::vector<Vector4>& planes);

    //Collision resolution functions
    static void sphereTriangleResolution(Entity* modelA, Sphere& sphere, Entity* modelB, Triangle& triangle); //Resolve collision math
    static void sphereSphereResolution(Entity* modelA, Sphere& sphereA, Entity* modelB, Sphere& sphereB); //resolve collision math

    static Sphere transform(Sphere* sphere, Matrix transform);
    static Triangle transform(Triangle* triangle, Matrix transform);
};