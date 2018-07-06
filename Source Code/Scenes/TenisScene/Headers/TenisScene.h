/*
   Copyright (c) 2014 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
   and associated documentation files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _AITENIS_SCENE_H
#define _AITENIS_SCENE_H

#include "Scenes/Headers/Scene.h"
#include "AI/Headers/GOAPContext.h"

class AIEntity;
class AITeam;
class Sphere3D;
class NPC;

class TenisScene : public Scene {
public:
    TenisScene() : Scene(),
        _aiPlayer1(nullptr),
        _aiPlayer2(nullptr),
        _aiPlayer3(nullptr),
        _aiPlayer4(nullptr),
        _player1(nullptr),
        _player2(nullptr),
        _player3(nullptr),
        _player4(nullptr),
        _floor(nullptr),
        _net(nullptr),
        _ballSGN(nullptr),
        _ball(nullptr){
        _sideImpulseFactor = 0;
        _directionTeam1ToTeam2 = true;
        _upwardsDirection = true;
        _touchedTerrainTeam1 = false;
        _touchedTerrainTeam2 = false;
        _lostTeam1 = false;
        _applySideImpulse = false;
        _scoreTeam1 = 0;
        _scoreTeam2 = 0;
        _gamePlaying = false;
        _gameGUID = 0;
    }

    void preRender();

    bool load(const std::string& name, CameraManager* const cameraMgr, GUI* const gui);
    bool loadResources(bool continueOnErrors);
    bool initializeAI(bool continueOnErrors);
    bool deinitializeAI(bool continueOnErrors);
    void processInput(const U64 deltaTime);
    void processTasks(const U64 deltaTime);
    void processGUI(const U64 deltaTime);
    bool onKeyUp(const OIS::KeyEvent& key);
    bool onMouseMove(const OIS::MouseEvent& key);
    bool onMouseClickUp(const OIS::MouseEvent& key,OIS::MouseButtonID button);

private:
    //ToDo: replace with Physics system collision detection
    void checkCollisions();
    void playGame(cdiggins::any a, CallbackParam b);
    void startGame();
    void resetGame();

private:
    vec3<F32> _sunvector;
    Sphere3D* _ball;
    SceneGraphNode* _ballSGN;
    SceneGraphNode* _net;
    SceneGraphNode* _floor;

private: //Game stuff
    mutable SharedLock _gameLock;
    mutable boost::atomic_bool _collisionPlayer1;
    mutable boost::atomic_bool _collisionPlayer2;
    mutable boost::atomic_bool _collisionPlayer3;
    mutable boost::atomic_bool _collisionPlayer4;
    mutable boost::atomic_bool _collisionNet;
    mutable boost::atomic_bool _collisionFloor;
    mutable boost::atomic_bool _directionTeam1ToTeam2;
    mutable boost::atomic_bool _upwardsDirection;
    mutable boost::atomic_bool _touchedTerrainTeam1;
    mutable boost::atomic_bool _touchedTerrainTeam2;
    mutable boost::atomic_bool _lostTeam1;
    mutable boost::atomic_bool _applySideImpulse;
    mutable boost::atomic_bool _gamePlaying;
    mutable boost::atomic_int  _scoreTeam1;
    mutable boost::atomic_int  _scoreTeam2;
    F32 _sideImpulseFactor;
    U32 _gameGUID;
    ///AIEntities are the "processors" behing the NPC's
    AIEntity *_aiPlayer1, *_aiPlayer2, *_aiPlayer3, *_aiPlayer4;
    ///NPC's are the actual game entities
    NPC *_player1, *_player2, *_player3, *_player4;
    ///Team's are factions for AIEntites so they can manage friend/foe situations
    AITeam *_team1, *_team2;
    ///GOAP context (mainly for logging)
    GOAPContext _GOAPContext;
};
#endif