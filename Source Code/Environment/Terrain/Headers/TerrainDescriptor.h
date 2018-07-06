/*
   Copyright (c) 2016 DIVIDE-Studio
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

#ifndef _TERRAIN_DESCRIPTOR_H_
#define _TERRAIN_DESCRIPTOR_H_

#include "Core/Resources/Headers/ResourceDescriptor.h"

namespace Divide {

class TerrainDescriptor : public PropertyDescriptor {
   public:
    explicit TerrainDescriptor(const stringImpl& name)
        : PropertyDescriptor(PropertyDescriptor::DescriptorType::DESCRIPTOR_TERRAIN_INFO)
    {
        setDefaultValues();
    }

    ~TerrainDescriptor() { _variables.clear(); }

    TerrainDescriptor* clone() const {
        return MemoryManager_NEW TerrainDescriptor(*this);
    }

    inline void setDefaultValues() {
        _variables.clear();
        _variablesf.clear();
        _grassDensity = _treeDensity = 0.0f;
        _grassScale = _treeScale = 1.0f;
        _is16Bit = _active = false;
        _chunkSize = 0;
        _textureLayers = 1;
        _position.reset();
        _scale.set(1.0f);
        _altitudeRange.set(0, 1);
        _dimensions.set(1);
    }

    void addVariable(const stringImpl& name, const stringImpl& value) {
        hashAlg::insert(_variables, std::make_pair(_ID_RT(name), value));
    }

    void addVariable(const stringImpl& name, F32 value) {
        hashAlg::insert(_variablesf, std::make_pair(_ID_RT(name), value));
    }

    void setTextureLayerCount(U8 count) { _textureLayers = count; }
    void setDimensions(const vec2<U16>& dim) { _dimensions = dim; }
    void setAltitudeRange(const vec2<F32>& dim) { _altitudeRange = dim; }
    void setPosition(const vec3<F32>& position) { _position = position; }
    void setScale(const vec2<F32>& scale) { _scale = scale; }
    void setGrassDensity(F32 grassDensity) { _grassDensity = grassDensity; }
    void setTreeDensity(F32 treeDensity) { _treeDensity = treeDensity; }
    void setGrassScale(F32 grassScale) { _grassScale = grassScale; }
    void setTreeScale(F32 treeScale) { _treeScale = treeScale; }
    void setActive(bool active) { _active = active; }
    void setChunkSize(U32 size) { _chunkSize = size; }
    void set16Bit(bool state) { _is16Bit = state; }

    U8 getTextureLayerCount() const { return _textureLayers; }
    F32 getGrassDensity() const { return _grassDensity; }
    F32 getTreeDensity() const { return _treeDensity; }
    F32 getGrassScale() const { return _grassScale; }
    F32 getTreeScale() const { return _treeScale; }
    bool getActive() const { return _active; }
    U32 getChunkSize() const { return _chunkSize; }
    bool is16Bit() const { return _is16Bit; }

    const vec2<F32>& getAltitudeRange() const { return _altitudeRange; }
    const vec2<U16>& getDimensions() const { return _dimensions; }
    const vec3<F32>& getPosition() const { return _position; }
    const vec2<F32>& getScale() const { return _scale; }

    stringImpl getVariable(const stringImpl& name) const {
        hashMapImpl<ULL, stringImpl>::const_iterator it =
            _variables.find(_ID_RT(name));
        if (it != std::end(_variables)) {
            return it->second;
        }
        return "";
    }

    F32 getVariablef(const stringImpl& name) const {
        hashMapImpl<ULL, F32>::const_iterator it =
            _variablesf.find(_ID_RT(name));
        if (it != std::end(_variablesf)) {
            return it->second;
        }
        return 0.0f;
    }

   private:
    hashMapImpl<ULL, stringImpl> _variables;
    hashMapImpl<ULL, F32> _variablesf;
    F32 _grassDensity;
    U32 _chunkSize;
    F32 _treeDensity;
    F32 _grassScale;
    F32 _treeScale;
    bool _is16Bit;
    bool _active;
    U8 _textureLayers;
    vec3<F32> _position;
    vec2<F32> _scale;
    vec2<F32> _altitudeRange;
    vec2<U16> _dimensions;
};

template <typename T>
inline T TER_COORD(T x, T y, T w) {
    return ((y) * (w) + (x));
}

};  // namespace Divide

#endif