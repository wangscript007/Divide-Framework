/*
   Copyright (c) 2015 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software
   and associated documentation files (the "Software"), to deal in the Software
   without restriction,
   including without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
   PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
   DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
   IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _WAR_SCENE_H
#define _WAR_SCENE_H

#include "Scenes/Headers/Scene.h"

namespace Divide {

class SkinnedSubMesh;
class NPC;

namespace AI {
class AITeam;
class AIEntity;
class WarSceneOrder;
};

class WarScene : public Scene {
   public:
    WarScene();
    ~WarScene();

    bool load(const stringImpl& name, GUI* const gui);
    bool loadResources(bool continueOnErrors);
    bool initializeAI(bool continueOnErrors);
    bool deinitializeAI(bool continueOnErrors);
    void processTasks(const U64 deltaTime);
    void processGUI(const U64 deltaTime);
    void updateSceneStateInternal(const U64 deltaTime);

    void registerPoint(U8 teamID, const stringImpl& unitName);
    void printMessage(U8 eventId, const stringImpl& unitName);

   private:
    void startSimulation();
    void toggleCamera();
    bool removeUnits(bool removeNodesOnCall);
    bool addUnits();
    bool resetUnits();
    void checkGameCompletion();
    AI::AIEntity* findAI(SceneGraphNode_ptr node);

   private:
    SceneGraphNode_wptr _sun;
    GUIMessageBox* _infoBox;
    vectorImpl<SceneGraphNode_wptr> _lightNodes;
    vectorImpl<std::pair<SceneGraphNode_wptr, bool>> _lightNodes2;
    vectorImpl<SceneGraphNode_wptr> _lightNodes3;

   private:  // Game
    U32  _timeLimitMinutes;
    U32  _scoreLimit;
    U32  _runCount;
    U64  _elapsedGameTime;
    bool _sceneReady;
    bool _resetUnits;
    U64 _lastNavMeshBuildTime;
    /// AIEntities are the "processors" behing the NPC's
    vectorImpl<AI::AIEntity*> _army[2];
    /// NPC's are the actual game entities
    vectorImpl<NPC*> _armyNPCs[2];
    IMPrimitive* _targetLines;
    SceneGraphNode_wptr _flag[2];
    SceneGraphNode_wptr _particleEmitter;
    /// Teams are factions for AIEntites so they can manage friend/foe situations
    AI::AITeam* _faction[2];
};

};  // namespace Divide

#endif