#include "Headers/VisualSensor.h"

#include "AI/Headers/AIEntity.h"
#include "Core/Math/Headers/Transform.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Dynamics/Entities/Units/Headers/NPC.h"

namespace Divide {
using namespace AI;

VisualSensor::VisualSensor(AIEntity* const parentEntity) : Sensor(parentEntity, VISUAL_SENSOR) 
{
}

VisualSensor::~VisualSensor()
{
	for (NodeContainerMap::value_type& container : _nodeContainerMap) {
		container.second.clear();
	}
	_nodeContainerMap.clear();
	_nodePositionsMap.clear();
}

void VisualSensor::followSceneGraphNode(U32 containerID, SceneGraphNode* const node) {
    DIVIDE_ASSERT(node != nullptr, "VisualSensor error: Invalid node specified for follow function");
	NodeContainerMap::iterator container = _nodeContainerMap.find(containerID);

    if (container != _nodeContainerMap.end()) {
        NodeContainer::const_iterator nodeEntry = container->second.find(node->getGUID());
        if (nodeEntry == container->second.end()) {
            hashAlg::emplace(container->second, node->getGUID(), node);
			node->registerDeletionCallback(DELEGATE_BIND(&VisualSensor::unfollowSceneGraphNode,
                                                         this,
                                                         containerID, 
                                                         node->getGUID()));
        } else {
            ERROR_FN("VisualSensor: Added the same node to follow twice!");
        }
    } else {
        NodeContainer& newContainer = _nodeContainerMap[containerID];

        hashAlg::emplace(newContainer, node->getGUID(), node);
		node->registerDeletionCallback(DELEGATE_BIND(&VisualSensor::unfollowSceneGraphNode, 
                                                     this, 
                                                     containerID, 
                                                     node->getGUID() ) );
    }

    NodePositions& positions = _nodePositionsMap[containerID];
    hashAlg::emplace(positions, node->getGUID(), node->getComponent<PhysicsComponent>()->getPosition());
}

void VisualSensor::unfollowSceneGraphNode(U32 containerID, U64 nodeGUID) {
    DIVIDE_ASSERT(nodeGUID != 0, "VisualSensor error: Invalid node GUID specified for unfollow function");
	NodeContainerMap::iterator container = _nodeContainerMap.find(containerID);
    if (container != _nodeContainerMap.end()) {
        NodeContainer::iterator nodeEntry = container->second.find(nodeGUID);
        if (nodeEntry != container->second.end()) {
            container->second.erase(nodeEntry);
        } else {
            ERROR_FN("VisualSensor: Specified node does not exist in specified container!");
        }
    } else {
        ERROR_FN("VisualSensor: Invalid container specified for unfollow!");
    }
}


void VisualSensor::update( const U64 deltaTime ) {
    for (const NodeContainerMap::value_type& container : _nodeContainerMap ) {
        NodePositions& positions = _nodePositionsMap[container.first];
        for (const NodeContainer::value_type& entry : container.second ) {
            positions[entry.first] = entry.second->getComponent<PhysicsComponent>()->getPosition();
        }
    }
}

SceneGraphNode* const VisualSensor::getClosestNode(U32 containerID) {
    NodeContainerMap::iterator container = _nodeContainerMap.find(containerID);
    if ( container != _nodeContainerMap.end() ) {
        NodePositions& positions = _nodePositionsMap[container->first];
        NPC* const unit = _parentEntity->getUnitRef();
        if ( unit ) {
            const vec3<F32>& currentPosition = unit->getCurrentPosition();
            U64 currentNearest = 0;
            U32 currentDistanceSq = 0;
            for (const NodePositions::value_type& entry : positions ) {
                U32 temp = currentPosition.distanceSquared( entry.second );
                if ( temp < currentDistanceSq ) {
                    currentDistanceSq = temp;
                    currentNearest = entry.first;
                }
            }
            if ( currentNearest != 0 ) {
                NodeContainer::const_iterator nodeEntry = container->second.find( currentNearest );
                if ( nodeEntry != container->second.end() ) {
                    return nodeEntry->second;
                }
            }
        }
    }
    return nullptr;
}

F32 VisualSensor::getDistanceToNodeSq(U32 containerID, U64 nodeGUID) {
    DIVIDE_ASSERT(nodeGUID != 0, "VisualSensor error: Invalid node GUID specified for distance request");
    NPC* const unit = _parentEntity->getUnitRef();
    if (unit) {
        return unit->getCurrentPosition().distanceSquared(getNodePosition(containerID, nodeGUID));          
    }

    return std::numeric_limits<F32>::max();
}

vec3<F32> VisualSensor::getNodePosition(U32 containerID, U64 nodeGUID) {
    DIVIDE_ASSERT(nodeGUID != 0, "VisualSensor error: Invalid node GUID specified for position request");
	NodeContainerMap::iterator container = _nodeContainerMap.find(containerID);
    if (container != _nodeContainerMap.end()) {
        NodePositions& positions = _nodePositionsMap[container->first];
        NodePositions::iterator it = positions.find(nodeGUID);
        if (it != positions.end()) {
            return it->second;          
        }
    }

    return vec3<F32>(std::numeric_limits<F32>::max());
}

}; //namespace Divide