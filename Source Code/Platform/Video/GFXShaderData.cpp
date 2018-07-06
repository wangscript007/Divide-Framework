#include "stdafx.h"

#include "Headers/GFXShaderData.h"

namespace Divide {

GFXShaderData::GFXShaderData()
    : _needsUpload(true)
{
}

GFXShaderData::GPUData::GPUData()
{
    _ProjectionMatrix.identity();
    _InvProjectionMatrix.identity();
    _ViewMatrix.identity();
    _ViewProjectionMatrix.identity();
    _cameraPosition.set(0.0f);
    _ViewPort.set(1.0f);
    _renderProperties.set(0.01f, 100.0f, 0.0f, 0.0f);
    for (U8 i = 0; i < to_base(Frustum::FrustPlane::COUNT); ++i) {
        _frustumPlanes[i].set(0.0f);
        _clipPlanes[i].set(0.0f);
    }
}

bool GFXShaderData::GPUData::operator==(const GPUData& other) const {
    for (U32 i = 0; i < to_base(Frustum::FrustPlane::COUNT); ++i) {
        if (_frustumPlanes[i] != other._frustumPlanes[i] || _clipPlanes[i] != other._clipPlanes[i]) {
            return false;
        }
    }

    return _cameraPosition == other._cameraPosition &&
           _ViewPort == other._ViewPort &&
           _renderProperties == other._renderProperties &&
           _ProjectionMatrix == other._ProjectionMatrix &&
           _InvProjectionMatrix == other._InvProjectionMatrix &&
           _ViewMatrix == other._ViewMatrix &&
           _ViewProjectionMatrix == other._ViewProjectionMatrix;
}

bool GFXShaderData::GPUData::operator!=(const GPUData& other) const {
    for (U32 i = 0; i < to_base(Frustum::FrustPlane::COUNT); ++i) {
        if (_frustumPlanes[i] != other._frustumPlanes[i] || _clipPlanes[i] != other._clipPlanes[i]) {
            return true;
        }
    }

    return _cameraPosition != other._cameraPosition ||
           _ViewPort != other._ViewPort ||
           _renderProperties != other._renderProperties ||
           _ProjectionMatrix != other._ProjectionMatrix ||
           _InvProjectionMatrix != other._InvProjectionMatrix ||
           _ViewMatrix != other._ViewMatrix ||
           _ViewProjectionMatrix != other._ViewProjectionMatrix;
}

}; //namespace Divide