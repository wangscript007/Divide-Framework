/*
Copyright (c) 2018 DIVIDE-Studio
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

#ifndef _MATERIAL_INL_
#define _MATERIAL_INL_

namespace Divide {

inline void Material::setColourData(const ColourData& other) {
    _colourData = other;
    updateTranslucency();
}
inline void Material::setHardwareSkinning(const bool state) {
    _needsNewShader = true;
    _hardwareSkinning = state;
}

inline void Material::setTextureUseForDepth(ShaderProgram::TextureUsage slot, bool state) {
    _textureUseForDepth[to_base(slot)] = state;
}

inline void Material::setShaderProgram(const ShaderProgram_ptr& shader) {
    for (RenderStagePass::StagePassIndex i = 0; i < RenderStagePass::count(); ++i) {
        setShaderProgram(shader, RenderStagePass::stagePass(i));
    }
}

inline void Material::disableTranslucency() {
    _translucencyDisabled = true;
}

inline void Material::setRenderStateBlock(size_t renderStateBlockHash, I32 variant) {
    for (RenderStagePass::StagePassIndex i = 0; i < RenderStagePass::count(); ++i) {
        setRenderStateBlock(renderStateBlockHash, RenderStagePass::stagePass(i), variant);
    }
}

inline void Material::setRenderStateBlock(size_t renderStateBlockHash, RenderStage renderStage, I32 variant) {
    for (U8 pass = 0; pass < to_base(RenderPassType::COUNT); ++pass) {
        RenderStagePass renderStagePass(renderStage, static_cast<RenderPassType>(pass));

        if (variant < 0) {
            renderStagePass._variant = 0;
            for (size_t& state : defaultRenderStates(renderStagePass)) {
                state = renderStateBlockHash;
                ++renderStagePass._variant;
            }
        } else {
            assert(variant < std::numeric_limits<U8>::max());
            renderStagePass._variant = to_U8(variant);
            defaultRenderStates(renderStagePass)[variant] = renderStateBlockHash;
        }
    }
}

inline void Material::setRenderStateBlock(size_t renderStateBlockHash, RenderPassType renderPassType, I32 variant) {
    for (U8 stage = 0; stage < to_base(RenderStage::COUNT); ++stage) {
        RenderStagePass renderStagePass(static_cast<RenderStage>(stage), renderPassType);

        if (variant < 0 ) {
            renderStagePass._variant = 0;
            for (size_t& state : defaultRenderStates(renderStagePass)) {
                state = renderStateBlockHash;
                ++renderStagePass._variant;
            }
        } else {
            assert(variant < std::numeric_limits<U8>::max());
            renderStagePass._variant = to_U8(variant);
            defaultRenderState(renderStagePass) = renderStateBlockHash;
        }
    }
}

inline void Material::setRenderStateBlock(size_t renderStateBlockHash, RenderStagePass renderStagePass, I32 variant) {
    if (variant < 0) {
        renderStagePass._variant = 0;
        for (size_t& state : defaultRenderStates(renderStagePass)) {
            state = renderStateBlockHash;
            ++renderStagePass._variant;
        }
    } else {
        assert(variant < std::numeric_limits<U8>::max());
        renderStagePass._variant = to_U8(variant);
        defaultRenderStates(renderStagePass)[variant] = renderStateBlockHash;
    }
}

inline void Material::setParallaxFactor(F32 factor) {
    _parallaxFactor = std::min(0.01f, factor);
}

inline F32 Material::getParallaxFactor() const {
    return _parallaxFactor;
}

inline std::weak_ptr<Texture> Material::getTexture(ShaderProgram::TextureUsage textureUsage) const {
    SharedLock r_lock(_textureLock);
    return _textures[to_U32(textureUsage)];
}

inline const Material::TextureOperation& Material::getTextureOperation() const {
    return _operation;
}

inline Material::ColourData& Material::getColourData() {
    return _colourData;
}

inline const Material::ColourData&  Material::getColourData()  const {
    return _colourData;
}

inline const Material::ShadingMode& Material::getShadingMode() const {
    return _shadingMode;
}

inline const Material::BumpMethod&  Material::getBumpMethod()  const {
    return _bumpMethod;
}

inline bool Material::hasTransparency() const {
    return _translucencySource != TranslucencySource::COUNT;
}

inline bool Material::hasTranslucency() const {
    assert(hasTransparency());

    return _translucent;
}

inline bool Material::isPBRMaterial() const {
    return getShadingMode() == ShadingMode::OREN_NAYAR || getShadingMode() == ShadingMode::COOK_TORRANCE;
}

inline bool Material::isDoubleSided() const {
    return _doubleSided;
}

inline bool Material::receivesShadows() const {
    return _receivesShadows;
}

inline bool Material::isReflective() const {
    if (_isReflective) {
        return true;
    }
    if (getShadingMode() == ShadingMode::BLINN_PHONG ||
        getShadingMode() == ShadingMode::PHONG ||
        getShadingMode() == ShadingMode::FLAT ||
        getShadingMode() == ShadingMode::TOON)
    {
        return _colourData.shininess() > PHONG_REFLECTIVITY_THRESHOLD;
    }

    return  _colourData.reflectivity() > 0.0f;
}

inline bool Material::isRefractive() const {
    return hasTransparency() && _isRefractive;
}

inline void Material::setBumpMethod(const BumpMethod& newBumpMethod) {
    _bumpMethod = newBumpMethod;
    _needsNewShader = true;
}

inline void Material::setShadingMode(const ShadingMode& mode) {
    _shadingMode = mode;
    _needsNewShader = true;
}

inline ShaderProgramInfo& Material::getShaderInfo(RenderStagePass renderStagePass) {
    return shaderInfo(renderStagePass);
}

inline ShaderProgramInfo& Material::shaderInfo(RenderStagePass renderStagePass) {
    return _shaderInfo[renderStagePass.index()];
}

inline const ShaderProgramInfo& Material::shaderInfo(RenderStagePass renderStagePass) const {
    return _shaderInfo[renderStagePass.index()];
}

inline void Material::setShaderProgram(const ShaderProgram_ptr& shader, RenderStagePass renderStagePass) {
    shaderInfo(renderStagePass)._customShader = true;
    setShaderProgramInternal(shader, renderStagePass);
}

inline void Material::setShaderProgram(const ShaderProgram_ptr& shader, RenderStage stage) {
    for (U8 pass = 0; pass < to_base(RenderPassType::COUNT); ++pass) {
        setShaderProgram(shader, RenderStagePass(stage, static_cast<RenderPassType>(pass)));
    }
}

inline void Material::setShaderProgram(const ShaderProgram_ptr& shader, RenderPassType passType) {
    for (U8 stage = 0; stage < to_base(RenderStage::COUNT); ++stage) {
        setShaderProgram(shader, RenderStagePass(static_cast<RenderStage>(stage), passType));
    }
}

inline size_t& Material::defaultRenderState(RenderStagePass renderStagePass) {
    return _defaultRenderStates[renderStagePass.index()][renderStagePass._variant];
}

inline std::array<size_t, 3>& Material::defaultRenderStates(RenderStagePass renderStagePass) {
    return _defaultRenderStates[renderStagePass.index()];
}

inline void Material::ignoreXMLData(const bool state) {
    _ignoreXMLData = state;
}

inline bool Material::ignoreXMLData() const {
    return _ignoreXMLData;
}

inline void Material::setBaseShaderData(const ShaderData& data) {
    _baseShaderSources = data;
}

inline const Material::ShaderData& Material::getBaseShaderData() const {
    return _baseShaderSources;
}

inline void Material::addShaderDefine(ShaderType type, const Str128& define, bool addPrefix) {
    if (type != ShaderType::COUNT) {
        addShaderDefineInternal(type, define, addPrefix);
    } else {
        for (U8 i = 0; i < to_U8(ShaderType::COUNT); ++i) {
            addShaderDefine(static_cast<ShaderType>(i), define, addPrefix);
        }
    }
}

inline void Material::addShaderDefineInternal(ShaderType type, const Str128& define, bool addPrefix) {
    ModuleDefines& defines = _extraShaderDefines[to_base(type)];

    if (eastl::find_if(eastl::cbegin(defines),
                       eastl::cend(defines),
                       [&define, addPrefix](const auto& it) {
                            return it.second == addPrefix &&
                                   it.first.compare(define.c_str()) == 0;
                        }) == eastl::cend(defines))
    {
        defines.emplace_back(define, addPrefix);
    }
}

inline const ModuleDefines& Material::shaderDefines(ShaderType type) const {
    assert(type != ShaderType::COUNT);

    return _extraShaderDefines[to_base(type)];
}

}; //namespace Divide

#endif //_MATERIAL_INL_
