#include "stdafx.h"

#include "Headers/Light.h"
#include "Rendering/Lighting/Headers/LightPool.h"

#include "Graphs/Headers/SceneGraph.h"
#include "Managers/Headers/SceneManager.h"
#include "Geometry/Material/Headers/Material.h"
#include "Geometry/Shapes/Predefined/Headers/Sphere3D.h"
#include "ECS/Components/Headers/TransformComponent.h"

namespace Divide {


namespace TypeUtil {
    const char* LightTypeToString(LightType lightType) noexcept {
        return Names::lightType[to_base(lightType)];
    }

    LightType StringToLightType(const stringImpl& name) {
        for (U8 i = 0; i < to_U8(LightType::COUNT); ++i) {
            if (strcmp(name.c_str(), Names::lightType[i]) == 0) {
                return static_cast<LightType>(i);
            }
        }

        return LightType::COUNT;
    }
};

Light::Light(SceneGraphNode& sgn, const F32 range, LightType type, LightPool& parentPool)
    : ECS::Event::IEventListener(sgn.sceneGraph().GetECSEngine()), 
      _parentPool(parentPool),
      _sgn(sgn),
      _type(type),
      _castsShadows(false),
      _shadowIndex(-1)
{
    if (!_parentPool.addLight(*this)) {
        //assert?
    }

    for (U8 i = 0; i < 6; ++i) {
        _shadowProperties._lightVP[i].identity();
        _shadowProperties._lightPosition[i].w = std::numeric_limits<F32>::max();
    }
    
    _shadowProperties._lightDetails.x = to_F32(type);
    setDiffuseColour(FColour3(DefaultColours::WHITE));

    const ECS::CustomEvent evt = {
        ECS::CustomEvent::Type::TransformUpdated,
        sgn.get<TransformComponent>(),
        to_U32(TransformType::ALL)
    };

    updateCache(evt);

    _enabled = true;
}

Light::~Light()
{
    UnregisterAllEventCallbacks();
    _parentPool.removeLight(*this);
}

void Light::registerFields(EditorComponent& comp) {
    EditorComponentField rangeField = {};
    rangeField._name = "Range";
    rangeField._data = &_range;
    rangeField._type = EditorComponentFieldType::PUSH_TYPE;
    rangeField._readOnly = false;
    rangeField._range = { std::numeric_limits<F32>::epsilon(), 10000.f };
    rangeField._basicType = GFX::PushConstantType::FLOAT;
    comp.registerField(std::move(rangeField));


    EditorComponentField colourField = {};
    colourField._name = "Colour";
    colourField._dataGetter = [this](void* dataOut) { static_cast<FColour3*>(dataOut)->set(getDiffuseColour()); };
    colourField._dataSetter = [this](const void* data) { setDiffuseColour(*static_cast<const FColour3*>(data)); };
    colourField._type = EditorComponentFieldType::PUSH_TYPE;
    colourField._readOnly = false;
    colourField._basicType = GFX::PushConstantType::FCOLOUR3;
    comp.registerField(std::move(colourField));
}

void Light::updateCache(const ECS::CustomEvent& data) {
    OPTICK_EVENT();

    TransformComponent* tComp = std::any_cast<TransformComponent*>(data._userData);
    assert(tComp != nullptr);

    if (_type != LightType::DIRECTIONAL && BitCompare(data._flag, to_U32(TransformType::TRANSLATION))) {
        _positionCache = tComp->getPosition();
    }

    if (_type != LightType::POINT && BitCompare(data._flag, to_U32(TransformType::ROTATION))) {
        _directionCache = DirectionFromAxis(tComp->getOrientation(), WORLD_Z_NEG_AXIS);
    }
}

void Light::setDiffuseColour(const UColour3& newDiffuseColour) {
    _colour.rgb(newDiffuseColour);
}

};