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

#ifndef _PHYSICS_COMPONENT_H_
#define _PHYSICS_COMPONENT_H_

#include "SGNComponent.h"

#include "Utility/Headers/Vector.h"
#include "Core/Math/Headers/MathClasses.h"

class SceneGraphNode;
class PhysicsComponent : public SGNComponent {
public:
    enum PhysicsGroup {
        NODE_COLLIDE_IGNORE = 0,
        NODE_COLLIDE_NO_PUSH,
        NODE_COLLIDE
    };

    PhysicsComponent(SceneGraphNode* const parentSGN);
    ~PhysicsComponent();

    inline const PhysicsGroup&      getPhysicsGroup()          const { return _physicsCollisionGroup; }
    inline void  setPhysicsGroup(const PhysicsGroup& newGroup)             { _physicsCollisionGroup = newGroup; }
    void cookCollisionMesh(const std::string& sceneName);

protected:
    PhysicsGroup _physicsCollisionGroup;
};
#endif