#include "Headers/WarSceneAISceneImpl.h"

#include "Headers/WarScene.h"

#include "AI/Sensors/Headers/VisualSensor.h"
#include "AI/ActionInterface/Headers/AITeam.h"

#include "Managers/Headers/AIManager.h"
#include "Managers/Headers/SceneManager.h"
#include "Core/Headers/Console.h"
#include "Core/Math/Headers/Transform.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Dynamics/Entities/Units/Headers/NPC.h"

namespace Divide {
namespace AI {

static const D32 g_ATTACK_RADIUS = 5;
static const D32 g_ATTACK_RADIUS_SQ = g_ATTACK_RADIUS * g_ATTACK_RADIUS;
static const U32 g_myTeamContainer = 0;
static const U32 g_enemyTeamContainer = 1;
static const U32 g_flagContainer = 2;
vec3<F32> WarSceneAISceneImpl::_initialFlagPositions[2];
GlobalWorkingMemory WarSceneAISceneImpl::_globalWorkingMemory;


WarSceneAISceneImpl::WarSceneAISceneImpl(AIType type)
    : AISceneImpl(),
      _type(type),
      _tickCount(0),
      _orderRequestTryCount(0),
      _visualSensorUpdateCounter(0),
      _attackTimer(0ULL),
      _deltaTime(0ULL),
      _visualSensor(nullptr),
      _audioSensor(nullptr)
{
}

WarSceneAISceneImpl::~WarSceneAISceneImpl()
{
#if defined(PRINT_AI_TO_FILE)
    _WarAIOutputStream.close();
#endif
}

void WarSceneAISceneImpl::initInternal() {
#if defined(PRINT_AI_TO_FILE)
    _WarAIOutputStream.open(
        Util::stringFormat("AILogs/%s_.txt", _entity->getName().c_str()),
        std::ofstream::out | std::ofstream::trunc);
#endif
    _visualSensor =
        dynamic_cast<VisualSensor*>(
            _entity->getSensor(SensorType::VISUAL_SENSOR));
    DIVIDE_ASSERT(_visualSensor != nullptr,
                  "WarSceneAISceneImpl error: No visual sensor found for "
                  "current AI entity!");

    AITeam* currentTeam = _entity->getTeam();
    AITeam* enemyTeam = AIManager::getInstance().getTeamByID(currentTeam->getEnemyTeamID(0));
    const AITeam::TeamMap& teamAgents = currentTeam->getTeamMembers();
    const AITeam::TeamMap& enemyMembers = enemyTeam->getTeamMembers();

    for (const AITeam::TeamMap::value_type& member : teamAgents) {
        SceneGraphNode* node = member.second->getUnitRef()->getBoundNode();
        _nodeToUnitMap[g_myTeamContainer].insert(
            hashAlg::makePair(node->getGUID(), member.second));
        _visualSensor->followSceneGraphNode(g_myTeamContainer, *node);
    }

    for (const AITeam::TeamMap::value_type& enemy : enemyMembers) {
        SceneGraphNode* node = enemy.second->getUnitRef()->getBoundNode();
        _nodeToUnitMap[g_enemyTeamContainer].insert(
            hashAlg::makePair(node->getGUID(), enemy.second));
        _visualSensor->followSceneGraphNode(g_enemyTeamContainer, *node);
    }

    _visualSensor->followSceneGraphNode(
        g_flagContainer, *_globalWorkingMemory._flags[0].value());
    _visualSensor->followSceneGraphNode(
        g_flagContainer, *_globalWorkingMemory._flags[1].value());

    _initialFlagPositions[0].set(_globalWorkingMemory._flags[0]
                                     .value()
                                     ->getComponent<PhysicsComponent>()
                                     ->getPosition());
    _initialFlagPositions[1].set(_globalWorkingMemory._flags[1]
                                     .value()
                                     ->getComponent<PhysicsComponent>()
                                     ->getPosition());

    
    _globalWorkingMemory._teamAliveCount[g_myTeamContainer].value(static_cast<U8>(teamAgents.size()));
    _globalWorkingMemory._teamAliveCount[g_enemyTeamContainer].value(static_cast<U8>(enemyMembers.size()));
}

bool WarSceneAISceneImpl::DIE() {
    if (_entity->getUnitRef()->getAttribute(to_uint(UnitAttributes::ALIVE_FLAG)) == 0) {
        return false;
    }
     
    AITeam* const currentTeam = _entity->getTeam();
    U8 ownTeamID = currentTeam->getTeamID();
    U8 enemyTeamID = 1 - ownTeamID;

    _entity->getUnitRef()->setAttribute(to_uint(UnitAttributes::ALIVE_FLAG), 0);
    U8 teamCount = _globalWorkingMemory._teamAliveCount[ownTeamID].value();
    _globalWorkingMemory._teamAliveCount[ownTeamID].value(std::max(teamCount - 1, 0));

    bool hadFlag = _localWorkingMemory._hasEnemyFlag.value();
    if (hadFlag == true) {
        _globalWorkingMemory._flags[enemyTeamID].value()->setParent(
        GET_ACTIVE_SCENEGRAPH().getRoot());
        PhysicsComponent* pComp = _globalWorkingMemory._flags[enemyTeamID]
                                  .value()
                                  ->getComponent<PhysicsComponent>();
         pComp->popTransforms();
         pComp->pushTransforms();
         pComp->setPosition(_entity->getPosition());

        _localWorkingMemory._hasEnemyFlag.value(false);
        _globalWorkingMemory._flagCarriers[ownTeamID].value(nullptr);
    }

    AITeam* enemyTeam = AIManager::getInstance().getTeamByID(currentTeam->getEnemyTeamID(0));
    const AITeam::TeamMap& teamAgents = currentTeam->getTeamMembers();
    const AITeam::TeamMap& enemyMembers = enemyTeam->getTeamMembers();

    for (const AITeam::TeamMap::value_type& member : teamAgents) {
        _entity->sendMessage(member.second, AIMsg::HAVE_DIED, hadFlag);
    }

    for (const AITeam::TeamMap::value_type& enemy : enemyMembers) {
        _entity->sendMessage(enemy.second, AIMsg::HAVE_DIED, hadFlag);
    }

    currentTeam->removeTeamMember(_entity);

    _entity->getUnitRef()->getBoundNode()->setActive(false);
    
    return true;
}

AIEntity* WarSceneAISceneImpl::getUnitForNode(U32 teamID, SceneGraphNode* node) const {
    if (node) {
        NodeToUnitMap::const_iterator it = _nodeToUnitMap[g_enemyTeamContainer].find(node->getGUID());
        if (it != std::end(_nodeToUnitMap[g_enemyTeamContainer])) {
            return it->second;
        }
    }

    return nullptr;
}

void WarSceneAISceneImpl::requestOrders() {
    const AITeam::OrderList& orders = _entity->getTeam()->requestOrders();
    std::array<U8, to_const_uint(WarSceneOrder::WarOrder::COUNT)> priority = { 0 };

    resetActiveGoals();
    printWorkingMemory();

    const AITeam* const currentTeam = _entity->getTeam();
    U8 ownTeamID = currentTeam->getTeamID();
    U8 enemyTeamID = 1 - ownTeamID;
    U32 weight = 0;
    WarSceneOrder::WarOrder orderID;
    for (const AITeam::OrderPtr& order : orders) {
        orderID = static_cast<WarSceneOrder::WarOrder>(order->getID());
        weight = 0;
        switch (orderID) {
            case WarSceneOrder::WarOrder::CAPTURE_ENEMY_FLAG: {
                if (_globalWorkingMemory._flagCarriers[ownTeamID].value() == nullptr) {
                    weight = 3;
                }
            } break;

            case WarSceneOrder::WarOrder::SCORE_ENEMY_FLAG: {
                if (_localWorkingMemory._hasEnemyFlag.value()) {
                    weight = atHomeBase() ? 4 : 3;
                }
            } break;

            case WarSceneOrder::WarOrder::IDLE: {
                weight = 1;
                // If we have the enemy flag and the enemy has our flag,
                // we MUST wait for someone else to retrieve it
                if (atHomeBase() &&
                    _localWorkingMemory._hasEnemyFlag.value() && 
                    _localWorkingMemory._enemyHasFlag.value()) {
                    weight = 5;
                }
            } break;

            case WarSceneOrder::WarOrder::KILL_ENEMY: {
                if (_globalWorkingMemory._teamAliveCount[enemyTeamID].value() > 0) {
                    weight = 2;
                }
                if (_localWorkingMemory._currentTarget.value() != nullptr) {
                    weight = 6;
                }
            } break;

            case WarSceneOrder::WarOrder::PROTECT_FLAG_CARRIER: {
                if (_globalWorkingMemory._flagCarriers[ownTeamID].value() != nullptr &&
                    _localWorkingMemory._hasEnemyFlag.value() == false) {
                    weight = 8;
                }
            } break;
        }
        priority[to_uint(orderID)] = weight;
    }

    for (GOAPGoal& goal : goalList()) {
        goal.relevancy(priority[goal.getID()]);
    }
    GOAPGoal* goal = findRelevantGoal();
    
    if (goal != nullptr) {
        PRINT("Current goal: [ %s ]", goal->getName().c_str());

        // Hack to loop in idle
        worldState().setVariable(GOAPFact(Fact::IDLING), GOAPValue(false));
        worldState().setVariable(GOAPFact(Fact::NEAR_ENEMY_FLAG) , GOAPValue(nearEnemyFlag()));

        if (replanGoal()) {
            PRINT("The plan for goal [ %s ] involves [ %d ] actions.",
                  goal->getName().c_str(),
                  getActiveGoal()->getCurrentPlan().size());
            printPlan();
        } else {
            PRINT("%s", getPlanLog().c_str());
            PRINT("Can't generate plan for goal [ %s ]",
                  goal->getName().c_str());
        }
    }
}

bool WarSceneAISceneImpl::preAction(ActionType type,
                                    const WarSceneAction* warAction) {
    const AITeam* const currentTeam = _entity->getTeam();
    DIVIDE_ASSERT(currentTeam != nullptr,
                  "WarScene error: INVALID TEAM FOR INPUT UPDATE");

    U8 ownTeamID = currentTeam->getTeamID();
    U8 enemyTeamID = 1 - ownTeamID;

    switch (type) {
        case ActionType::IDLE: {
            PRINT("Starting idle action");
            assert(atHomeBase());
            _entity->stop();
        } break;
        case ActionType::APPROACH_ENEMY_FLAG: {
            PRINT("Starting approach flag action");
            _entity->updateDestination(_globalWorkingMemory._teamFlagPosition[enemyTeamID].value());
        } break;
        case ActionType::CAPTURE_ENEMY_FLAG: {
            PRINT("Starting capture flag action");
            assert(nearEnemyFlag());
        } break;
        case ActionType::SCORE_FLAG: {
            PRINT("Starting score action");
            assert(atHomeBase() && nearOwnFlag());
        } break;
        case ActionType::RETURN_TO_BASE: {
            PRINT("Starting return to base action");
            _entity->updateDestination(_initialFlagPositions[ownTeamID]);
        } break;
        case ActionType::ATTACK_ENEMY: {
            PRINT("Starting attack enemy action");
            if (_localWorkingMemory._currentTarget.value() == nullptr &&
                _globalWorkingMemory._teamAliveCount[enemyTeamID].value() > 0) {
                SceneGraphNode* enemy = _visualSensor->getClosestNode(g_enemyTeamContainer);
                _entity->updateDestination(enemy->getComponent<PhysicsComponent>()->getPosition(), true);
                _localWorkingMemory._currentTarget.value(enemy);
            }
        } break;
        default: {
            assert(false);
        } break;
    };

    return true;
}

bool WarSceneAISceneImpl::postAction(ActionType type,
                                     const WarSceneAction* warAction) {
    const AITeam* const currentTeam = _entity->getTeam();
    
    DIVIDE_ASSERT(currentTeam != nullptr,
                  "WarScene error: INVALID TEAM FOR INPUT UPDATE");

    U8 ownTeamID = currentTeam->getTeamID();
    U8 enemyTeamID = 1 - ownTeamID;

    switch (type) {
        case ActionType::IDLE: {
            PRINT("Idle action over");
        } break;
        case ActionType::APPROACH_ENEMY_FLAG: {
            PRINT("Approach flag action over");
        } break;
        case ActionType::SCORE_FLAG: {
            assert(_localWorkingMemory._hasEnemyFlag.value());

            U8 score = _globalWorkingMemory._score[ownTeamID].value();
            _globalWorkingMemory._score[ownTeamID].value(score + 1);
            _localWorkingMemory._hasEnemyFlag.value(false);

            _globalWorkingMemory._flags[enemyTeamID].value()->setParent(
                GET_ACTIVE_SCENEGRAPH().getRoot());
            PhysicsComponent* pComp = _globalWorkingMemory._flags[enemyTeamID]
                                  .value()
                                  ->getComponent<PhysicsComponent>();
            pComp->popTransforms();

            for (const AITeam::TeamMap::value_type& member : currentTeam->getTeamMembers()) {
                _entity->sendMessage(member.second, AIMsg::HAVE_SCORED, _entity);
            }

            dynamic_cast<WarScene*>(GET_ACTIVE_SCENE())->registerPoint(ownTeamID);

        } break;
        case ActionType::CAPTURE_ENEMY_FLAG: {
            PRINT("Capture flag action over");
            assert(!_localWorkingMemory._hasEnemyFlag.value());

            // Attach flag to entity
            {
                PhysicsComponent* pComp = _globalWorkingMemory._flags[enemyTeamID]
                    .value()->getComponent<PhysicsComponent>();
                vec3<F32> prevScale(pComp->getScale());
                vec3<F32> prevPosition(pComp->getPosition());

                SceneGraphNode* targetNode = _entity->getUnitRef()->getBoundNode();
                _globalWorkingMemory._flags[enemyTeamID].value()->setParent(*targetNode);
                pComp->pushTransforms();
                pComp->setPosition(vec3<F32>(0.0f, 0.75f, 1.5f));
                pComp->setScale(
                    prevScale /
                    targetNode->getComponent<PhysicsComponent>()->getScale());
            }

            _localWorkingMemory._hasEnemyFlag.value(true);
            _globalWorkingMemory._flagCarriers[ownTeamID].value(_entity);

            for (const AITeam::TeamMap::value_type& member :
                 currentTeam->getTeamMembers()) {
                _entity->sendMessage(member.second, AIMsg::HAVE_FLAG, _entity);
            }
            
            const AITeam* const enemyTeam = AIManager::getInstance().getTeamByID(currentTeam->getEnemyTeamID(0));
            for (const AITeam::TeamMap::value_type& enemy : enemyTeam->getTeamMembers()) {
                _entity->sendMessage(enemy.second, AIMsg::ENEMY_HAS_FLAG, _entity);
            }
        } break;
        case ActionType::RETURN_TO_BASE: {
            PRINT("Return to base action over");
            assert(atHomeBase());
        } break;
        case ActionType::ATTACK_ENEMY: {
            PRINT("Attack enemy action over");
            _localWorkingMemory._currentTarget.value(nullptr);
        } break;
        default: {
            assert(false);
        } break;
    };

    GOAPAction::operationsIterator o;
    for (o = std::begin(warAction->effects()); o != std::end(warAction->effects()); ++o) {
        performActionStep(o);
    }

    PRINT("\n");

    return advanceGoal();
}

bool WarSceneAISceneImpl::checkCurrentActionComplete(const GOAPAction& planStep) {
    const AITeam* const currentTeam = _entity->getTeam();
   
    U8 ownTeamID = currentTeam->getTeamID();
    U8 enemyTeamID = 1 - ownTeamID;

    const SceneGraphNode& currentNode = *(_entity->getUnitRef()->getBoundNode());
    const SceneGraphNode& enemyFlag = *(_globalWorkingMemory._flags[enemyTeamID].value());

    const WarSceneAction& warAction = static_cast<const WarSceneAction&>(planStep);

    bool state = false;

    const BoundingBox& bb1 = currentNode.getBoundingBoxConst();
    switch (warAction.actionType()) {
        case ActionType::APPROACH_ENEMY_FLAG:
        case ActionType::CAPTURE_ENEMY_FLAG: {
            state = nearEnemyFlag();
            if (!state) {
                _entity->updateDestination(_globalWorkingMemory._teamFlagPosition[enemyTeamID].value(), true);
            }
        } break;

        case ActionType::SCORE_FLAG: {
            if (_localWorkingMemory._enemyHasFlag.value()) {
                invalidateCurrentPlan();
                return false;
            }

            state = atHomeBase();
        } break;

        case ActionType::IDLE:
        case ActionType::RETURN_TO_BASE: {
            state = atHomeBase();
            if (!state && _entity->destinationReached()) {
                _entity->updateDestination(_initialFlagPositions[ownTeamID]);
            }
        } break;
        case ActionType::ATTACK_ENEMY: {
            SceneGraphNode* enemy = _localWorkingMemory._currentTarget.value();
            if (!enemy) {
                state = true;
            } else {
                NPC* targetNPC = getUnitForNode(g_enemyTeamContainer, enemy)->getUnitRef();
                state = targetNPC->getAttribute(to_uint(UnitAttributes::ALIVE_FLAG)) == 0;
                if (!state) {
                    if (_entity->getPosition().distanceSquared(targetNPC->getPosition()) >= g_ATTACK_RADIUS_SQ) {
                        _entity->updateDestination(targetNPC->getPosition(), true);
                    }
                }
            }
        } break;
        default: {
            assert(false);
        } break;
    };

    if (state) {
        return warAction.postAction(*this);
    }
    return state;
}

void WarSceneAISceneImpl::processMessage(AIEntity* sender, AIMsg msg,
                                         const cdiggins::any& msg_content) {
    switch (msg) {
        case AIMsg::RETURNED_FLAG:
        case AIMsg::HAVE_FLAG: {
            invalidateCurrentPlan();
        } break;
        case AIMsg::ENEMY_HAS_FLAG: {
            _localWorkingMemory._enemyHasFlag.value(true);
            worldState().setVariable(GOAPFact(Fact::ENEMY_HAS_FLAG), GOAPValue(true));
        } break;
        case AIMsg::ATTACK: {
            if (!_localWorkingMemory._currentTarget.value() || 
                _localWorkingMemory._currentTarget.value()->getGUID() != 
                sender->getUnitRef()->getBoundNode()->getGUID())
            {
                invalidateCurrentPlan();
                _localWorkingMemory._currentTarget.value(sender->getUnitRef()->getBoundNode());
            }
        } break;

        case AIMsg::HAVE_DIED: {
            bool hadFlag = msg_content.constant_cast<bool>();
            if (hadFlag) {
                U8 senderTeamID = sender->getTeamID();
                U8 ownTeamID = _entity->getTeamID();
                if (ownTeamID == senderTeamID) {
                    _localWorkingMemory._hasEnemyFlag.value(false);
                    worldState().setVariable(GOAPFact(Fact::HAS_ENEMY_FLAG), GOAPValue(false));
                } else{
                    _localWorkingMemory._enemyHasFlag.value(false);
                    worldState().setVariable(GOAPFact(Fact::ENEMY_HAS_FLAG), GOAPValue(false));
                }
                invalidateCurrentPlan();
            }

             SceneGraphNode* node = sender->getUnitRef()->getBoundNode();
            _visualSensor->unfollowSceneGraphNode(g_myTeamContainer, node->getGUID());
            _visualSensor->unfollowSceneGraphNode(g_enemyTeamContainer, node->getGUID());
            if (_localWorkingMemory._currentTarget.value() != nullptr &&
                _localWorkingMemory._currentTarget.value()->getGUID() == node->getGUID()) {
                _localWorkingMemory._currentTarget.value(nullptr);
                invalidateCurrentPlan();
            }
        } break;
    };
}

bool WarSceneAISceneImpl::atHomeBase() const {
    return _entity->getPosition().distanceSquared(
               _initialFlagPositions[_entity->getTeam()->getTeamID()]) <=
           g_ATTACK_RADIUS_SQ;
}

bool WarSceneAISceneImpl::atEnemyBase() const {
    return _entity->getPosition().distanceSquared(
               _initialFlagPositions[1 - _entity->getTeam()->getTeamID()]) <=
           g_ATTACK_RADIUS_SQ;
}

bool WarSceneAISceneImpl::nearOwnFlag() const {
    return _entity->getPosition().distanceSquared(
               _globalWorkingMemory
                   ._teamFlagPosition[_entity->getTeam()->getTeamID()]
                   .value()) <= g_ATTACK_RADIUS_SQ;
}

bool WarSceneAISceneImpl::nearEnemyFlag() const {
    return _entity->getPosition().distanceSquared(
               _globalWorkingMemory
                   ._teamFlagPosition[1 - _entity->getTeam()->getTeamID()]
                   .value()) <= g_ATTACK_RADIUS_SQ;
}

void WarSceneAISceneImpl::updatePositions() {
    if (_globalWorkingMemory._flags[0].value() != nullptr &&
        _globalWorkingMemory._flags[1].value() != nullptr) {

        _globalWorkingMemory._teamFlagPosition[0].value(
            _visualSensor->getNodePosition(
                g_flagContainer,
                _globalWorkingMemory._flags[0].value()->getGUID()));

        _globalWorkingMemory._teamFlagPosition[1].value(
            _visualSensor->getNodePosition(
                g_flagContainer,
                _globalWorkingMemory._flags[1].value()->getGUID()));
    }

    
    AITeam* currentTeam = _entity->getTeam();
    AITeam* enemyTeam = AIManager::getInstance().getTeamByID(currentTeam->getEnemyTeamID(0));

    // If the enemy dropped the flag and we are near it, return it to base
    if (!_localWorkingMemory._enemyHasFlag.value()) {
        if (nearOwnFlag() && !atHomeBase()) {
            U8 teamID = currentTeam->getTeamID();
            _globalWorkingMemory._flags[teamID].value()->setParent(
                GET_ACTIVE_SCENEGRAPH().getRoot());
            PhysicsComponent* pComp = _globalWorkingMemory._flags[teamID]
                                  .value()
                                  ->getComponent<PhysicsComponent>();
            pComp->popTransforms();

           for (const AITeam::TeamMap::value_type& member :
                 currentTeam->getTeamMembers()) {
                _entity->sendMessage(member.second, AIMsg::RETURNED_FLAG, teamID);
            }
            for (const AITeam::TeamMap::value_type& enemy : enemyTeam->getTeamMembers()) {
                _entity->sendMessage(enemy.second, AIMsg::RETURNED_FLAG, teamID);
            }
        }
    }

    if (_type != AIType::HEAVY) {
        BoundingSphere boundingSphere(_entity->getUnitRef()->getBoundNode()->getBoundingSphereConst());
        const AITeam::TeamMap& enemyMembers = enemyTeam->getTeamMembers();
        for (const AITeam::TeamMap::value_type& enemy : enemyMembers) {
            if (boundingSphere.Collision(enemy.second->getUnitRef()->getBoundNode()->getBoundingSphereConst())) {
                if (_localWorkingMemory._currentTarget.value() != nullptr &&
                    _localWorkingMemory._currentTarget.value()->getGUID() != enemy.second->getGUID()) {
                    invalidateCurrentPlan();
                }
            }
        }
    }
}

void WarSceneAISceneImpl::processInput(const U64 deltaTime) {
    if (!_entity) {
        return;
    }

    _deltaTime += deltaTime;
    // wait 1 and a half seconds at the destination
    if (_deltaTime > Time::SecondsToMicroseconds(1.5)) {
        _deltaTime = 0;
    }

    if (_entity->getUnitRef()->getAttribute(to_uint(UnitAttributes::HEALTH_POINTS)) == 0) {
        DIE();
    }

    #if defined(PRINT_AI_TO_FILE)
        _WarAIOutputStream.flush();
    #endif
}

void WarSceneAISceneImpl::processData(const U64 deltaTime) {
    if (!_entity) {
        return;
    }

    if (!AIManager::getInstance().getNavMesh(_entity->getAgentRadiusCategory())) {
        return;
    }

    updatePositions();

    _attackTimer += deltaTime;

    SceneGraphNode* enemy = _localWorkingMemory._currentTarget.value();
    if (enemy != nullptr) {
        AIEntity* targetUnit = getUnitForNode(g_enemyTeamContainer, enemy);
        if (targetUnit != nullptr) {
            NPC* targetNPC = targetUnit->getUnitRef();
            bool alive = targetNPC->getAttribute(to_uint(UnitAttributes::ALIVE_FLAG)) > 0;
            if (alive && _entity->getPosition().distanceSquared(targetNPC->getPosition()) < g_ATTACK_RADIUS_SQ) {
                _entity->stop();
                _entity->getUnitRef()->lookAt(targetNPC->getPosition());
                if (_attackTimer >= Time::SecondsToMicroseconds(0.5)) {
                    I32 HP = targetNPC->getAttribute(to_uint(UnitAttributes::HEALTH_POINTS));
                    I32 Damage = _entity->getUnitRef()->getAttribute(to_uint(UnitAttributes::DAMAGE));
                    targetNPC->setAttribute(to_uint(UnitAttributes::HEALTH_POINTS), std::max(HP - Damage, 0));
                    _entity->sendMessage(targetUnit, AIMsg::ATTACK, 0);
                    _attackTimer = 0ULL;
                }
            } else {
                _attackTimer = 0ULL;
            }
        }
    }else {
        _attackTimer = 0ULL;
    }

    if (!getActiveGoal()) {
        if (!goalList().empty() && _orderRequestTryCount < 3) {
            requestOrders();
            _orderRequestTryCount++;
        }
    } else {
        _orderRequestTryCount = 0;
        checkCurrentActionComplete(*getActiveAction());
    }
}

void WarSceneAISceneImpl::update(const U64 deltaTime, NPC* unitRef) {
    U8 visualSensorUpdateFreq = 10;
    _visualSensorUpdateCounter =
        (_visualSensorUpdateCounter + 1) % (visualSensorUpdateFreq + 1);
    // Update sensor information
    if (_visualSensor && _visualSensorUpdateCounter == visualSensorUpdateFreq) {
        _visualSensor->update(deltaTime);
    }
    if (_audioSensor) {
        _audioSensor->update(deltaTime);
    }
}

bool WarSceneAISceneImpl::performAction(const GOAPAction& planStep) {
    return static_cast<const WarSceneAction&>(planStep).preAction(*this);
}

bool WarSceneAISceneImpl::performActionStep(
    GOAPAction::operationsIterator step) {
    GOAPFact crtFact = step->first;
    GOAPValue newVal = step->second;
    GOAPValue oldVal = worldState().getVariable(crtFact);
    if (oldVal != newVal) {
        PRINT("\t\t Changing \"%s\" from \"%s\" to \"%s\"",
                         WarSceneFactName(crtFact),
                         GOAPValueName(oldVal),
                         GOAPValueName(newVal));
    }
    worldState().setVariable(crtFact, newVal);
    return true;
}

bool WarSceneAISceneImpl::printActionStats(const GOAPAction& planStep) const {
    PRINT("Action [ %s ]", planStep.name().c_str());
    return true;
}

void WarSceneAISceneImpl::printWorkingMemory() const {
    PRINT("--------------- Working memory state BEGIN ----------------------------");
    PRINT("        Current position: - [ %4.1f , %4.1f, %4.1f]",
                     _entity->getPosition().x, _entity->getPosition().y,
                     _entity->getPosition().z);
    PRINT(
        "        Flag Positions - 0 : [ %4.1f , %4.1f, %4.1f] | 1 : [ %4.1f , "
        "%4.1f, %4.1f]",
        _globalWorkingMemory._teamFlagPosition[0].value().x,
        _globalWorkingMemory._teamFlagPosition[0].value().y,
        _globalWorkingMemory._teamFlagPosition[0].value().z,
        _globalWorkingMemory._teamFlagPosition[1].value().x,
        _globalWorkingMemory._teamFlagPosition[1].value().y,
        _globalWorkingMemory._teamFlagPosition[1].value().z);
    PRINT("        Has enemy flag: [ %s ]",
          _localWorkingMemory._hasEnemyFlag.value() ? "true" : "false");
    for (std::pair<GOAPFact, GOAPValue> var : worldState().vars_) {
        PRINT("        World state fact [ %s ] : [ %s ]",
              WarSceneFactName(var.first), var.second ? "true" : "false");
    }
    PRINT("--------------- Working memory state END ----------------------------");
}

void WarSceneAISceneImpl::registerGOAPPackage(const GOAPPackage& package) {
    worldState() = package._worldState;
    registerGoalList(package._goalList);
    for (const WarSceneAction& action : package._actionSet) {
        _actionList.push_back(action);
    }
    for (const WarSceneAction& action : _actionList) {
        registerAction(action);
    }
}

};  // namespace AI
};  // namespace Divide
