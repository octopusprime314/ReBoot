/*
* ModelFactory is part of the ReBoot distribution (https://github.com/octopusprime314/ReBoot.git).
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
*  MasterClock class. A singleton responsible for updating certain parties
*  who subscribe to various clock feeds.  A clock feed could be a 60 Hz screen
*  refresh update that MasterClock would trigger an event every 16.7 milliSeconds.
*/
#pragma once
#include <vector>
#include <functional>
#include <chrono>
#include <thread>

const int DEFAULT_FRAME_TIME = 16; //frame time in milliseconds which is 60 frames per second
const int KINEMATICS_TIME = 5; //kinematics time in milliseconds

class MasterClock {
    //Make constructor/destructor private so it can't be instantiated
    MasterClock();
    static MasterClock*                   _clock;

    //Clock feed subscriber's function pointers
    std::vector<std::function<void(int)>> _kinematicsRateFuncs;
    //Clock feed subscriber's function pointers
    std::vector<std::function<void(int)>> _animationRateFuncs;
    //Clock feed subscriber's function pointers
    std::vector<std::function<void(int)>> _frameRateFuncs;

    unsigned int                          _milliSecondCounter;
    std::thread*                          _animationThread;
    int                                   _animationTime;
    std::thread*                          _physicsThread;
    std::thread*                          _fpsThread;
    int                                   _frameTime;

    void                                  _animationProcess();
    void                                  _physicsProcess();
    void                                  _fpsProcess();

public:

    ~MasterClock();
    static MasterClock* instance();

    //Physics clock time update
    void subscribeKinematicsRate(std::function<void(int)> func);
    //Frame rate update
    void subscribeAnimationRate(std::function<void(int)> func);
    //Frame rate update
    void subscribeFrameRate(std::function<void(int)> func);
    //Gives programmer adjustable framerate
    void setFrameRate(int framesPerSecond);
    //Kicks off the master clock thread that will asynchronously updates subscribers with clock events
    void run();
};