#include "stdafx.h"

#include "Headers/IKComponent.h"

namespace Divide {

IKComponent::IKComponent(SceneGraphNode& parentSGN)
 : SGNComponent(parentSGN, "IK")
{
}

};