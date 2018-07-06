#include "Headers/Light.h"

#include "Graphs/Headers/SceneGraph.h"
#include "Core/Headers/ParamHandler.h"
#include "Core/Math/Headers/Transform.h"
#include "Managers/Headers/LightManager.h"
#include "Managers/Headers/SceneManager.h"
#include "Geometry/Material/Headers/Material.h"
#include "Rendering/Camera/Headers/FreeFlyCamera.h"
#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"

namespace Divide {

Light::Light(const F32 range, const LightType& type)
    : SceneNode(SceneNodeType::TYPE_LIGHT),
      _type(type),
      _rangeChanged(true),
      _drawImpostor(false),
      _impostor(nullptr),
      _lightSGN(nullptr),
      _shadowMapInfo(nullptr),
      _shadowCamera(nullptr),
      _castsShadows(false),
      _spotPropertiesChanged(false),
      _spotCosOuterConeAngle(0.0f)
{
    for (U8 i = 0; i < Config::Lighting::MAX_SPLITS_PER_LIGHT; ++i) {
        _shadowProperties._lightVP[i].identity();
        _shadowProperties._floatValues[i].set(-1.0f);
    }
    
    setDiffuseColor(DefaultColors::WHITE());
    setRange(1.0f);

    _renderState.addToDrawExclusionMask(RenderStage::SHADOW);

    _enabled = true;
}

Light::~Light()
{
}

bool Light::load(const stringImpl& name) {
    setName(name);
    _shadowCamera =
        Application::getInstance()
        .getKernel()
        .getCameraMgr()
        .createCamera(name + "_shadowCamera", Camera::CameraType::FREE_FLY);

    _shadowCamera->setMoveSpeedFactor(0.0f);
    _shadowCamera->setTurnSpeedFactor(0.0f);
    _shadowCamera->setFixedYawAxis(true);

    if (LightManager::getInstance().addLight(*this)) {
        return Resource::load();
    }

    return false;
}

bool Light::unload() {
    LightManager::getInstance().removeLight(getGUID(), getLightType());

    removeShadowMapInfo();

    return SceneNode::unload();
}

void Light::postLoad(SceneGraphNode& sgn) {
    // Hold a pointer to the light's location in the SceneGraph
    DIVIDE_ASSERT(
        _lightSGN == nullptr,
        "Light error: Lights can only be bound to a single SGN node!");

    _lightSGN = &sgn;
    _lightSGN->getComponent<PhysicsComponent>()->setPosition(_positionAndRange.xyz());
    _lightSGN->lockBBTransforms(true);
    computeBoundingBox();
    SceneNode::postLoad(sgn);
}

void Light::setDiffuseColor(const vec3<U8>& newDiffuseColor) {
    _color.rgb(newDiffuseColor);
}

void Light::setRange(F32 range) {
    _positionAndRange.w = range;
    _rangeChanged = true;
}

void Light::setSpotAngle(F32 newAngle) {
    _spotProperties.w = newAngle;
    _spotPropertiesChanged = true;
}

void Light::setSpotCosOuterConeAngle(F32 newCosAngle) {
    _spotCosOuterConeAngle = newCosAngle;
}

void Light::sceneUpdate(const U64 deltaTime, SceneGraphNode& sgn, SceneState& sceneState) {
    vec3<F32> dir(_lightSGN->getComponent<PhysicsComponent>()->getOrientation() * WORLD_Z_NEG_AXIS);
    dir.normalize();
    _spotProperties.xyz(dir);
    _positionAndRange.xyz(_lightSGN->getComponent<PhysicsComponent>()->getPosition());
    computeBoundingBox();

    SceneNode::sceneUpdate(deltaTime, sgn, sceneState);
}

void Light::computeBoundingBox() {
    if (_type == LightType::DIRECTIONAL) {
        vec3<F32> directionalLightPosition =
            _positionAndRange.xyz() * 
            (to_const_float(Config::Lighting::DIRECTIONAL_LIGHT_DISTANCE) * -1.0f);
        _boundingBox.first.set(directionalLightPosition - 10.0f,
                               directionalLightPosition + 10.0f);
    } else {
        _boundingBox.first.set(vec3<F32>(-getRange()), vec3<F32>(getRange()));
        _boundingBox.first.multiply(0.5f);
    }

    if (_type == LightType::SPOT) {
        _boundingBox.first.multiply(0.5f);
    }

    _boundingBox.second = true;
}

bool Light::onDraw(SceneGraphNode& sgn, RenderStage currentStage) {
    if (!_drawImpostor) {
        return true;
    }

    if (!_impostor) {
        _impostor = CreateResource<ImpostorSphere>(ResourceDescriptor(_name + "_impostor"));
        _impostor->setRadius(_positionAndRange.w);
        _impostor->renderState().setDrawState(true);
        _impostorSGN = _lightSGN->addNode(*_impostor);
        _impostorSGN.lock()->setActive(true);
    }

    Material* const impostorMaterialInst = _impostorSGN.lock()->getComponent<RenderingComponent>()->getMaterialInstance();
    impostorMaterialInst->setDiffuse(getDiffuseColor());

    updateImpostor();

    return true;
}

void Light::updateImpostor() {
    if (_type == LightType::DIRECTIONAL) {
        return;
    }
    // Updating impostor range is expensive, so check if we need to
    if (_rangeChanged) {
        F32 range = getRange();
        if (_type == LightType::SPOT) {
            range *= 0.5f;
            // Spot light's bounding sphere extends from the middle of the light's range outwards,
            // touching the light's position on one end and the cone at the other
            // so we need to offest the impostor's position a bit
            PhysicsComponent* pComp = _impostorSGN.lock()->getComponent<PhysicsComponent>();
            pComp->setPosition(getSpotDirection() * range);
        }
        _impostor->setRadius(range);
        _rangeChanged = false;
    }
}

void Light::addShadowMapInfo(ShadowMapInfo* shadowMapInfo) {
    MemoryManager::SAFE_UPDATE(_shadowMapInfo, shadowMapInfo);
}

bool Light::removeShadowMapInfo() {
    MemoryManager::DELETE(_shadowMapInfo);
    return true;
}

void Light::validateOrCreateShadowMaps(SceneRenderState& sceneRenderState) {
    _shadowMapInfo->createShadowMap(sceneRenderState, shadowCamera());
}

void Light::generateShadowMaps(SceneRenderState& sceneRenderState) {
    ShadowMap* sm = _shadowMapInfo->getShadowMap();

    DIVIDE_ASSERT(sm != nullptr,
                  "Light::generateShadowMaps error: Shadow casting light "
                  "with no shadow map found!");

    _shadowProperties._arrayOffset.set(sm->getArrayOffset());
    sm->render(sceneRenderState);

}

};