#include "stdafx.h"

#include "config.h"

#include "Headers/Material.h"
#include "Headers/ShaderComputeQueue.h"

#include "Rendering/Headers/Renderer.h"
#include "Utility/Headers/Localization.h"
#include "Managers/Headers/SceneManager.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Headers/RenderStateBlock.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"

#include "ECS/Components/Headers/RenderingComponent.h"

namespace Divide {

namespace {
    constexpr size_t g_materialXMLVersion = 1;

    constexpr size_t g_invalidStateHash = std::numeric_limits<size_t>::max();

    constexpr U16 g_numMipsToKeepFromAlphaTextures = 1;
    const char* g_PassThroughMaterialShaderName = "passThrough";
    
    constexpr TextureUsage g_materialTextures[] = {
        TextureUsage::UNIT0,
        TextureUsage::UNIT1,
        TextureUsage::NORMALMAP,
        TextureUsage::OPACITY,
        TextureUsage::OCCLUSION_METALLIC_ROUGHNESS,
    };

    constexpr U32 g_materialTexturesCount = sizeof(g_materialTextures) / sizeof(g_materialTextures[0]);
};


namespace TypeUtil {
    const char* TextureUsageToString(TextureUsage texUsage) noexcept {
        return Names::textureUsage[to_base(texUsage)];
    }

    TextureUsage StringToTextureUsage(const stringImpl& name) {
        for (U8 i = 0; i < to_U8(TextureUsage::COUNT); ++i) {
            if (strcmp(name.c_str(), Names::textureUsage[i]) == 0) {
                return static_cast<TextureUsage>(i);
            }
        }

        return TextureUsage::COUNT;
    }

    const char* BumpMethodToString(BumpMethod bumpMethod) noexcept {
        return Names::bumpMethod[to_base(bumpMethod)];
    }

    BumpMethod StringToBumpMethod(const stringImpl& name) {
        for (U8 i = 0; i < to_U8(BumpMethod::COUNT); ++i) {
            if (strcmp(name.c_str(), Names::bumpMethod[i]) == 0) {
                return static_cast<BumpMethod>(i);
            }
        }

        return BumpMethod::COUNT;
    }

    const char* ShadingModeToString(ShadingMode shadingMode) noexcept {
        return Names::shadingMode[to_base(shadingMode)];
    }

    ShadingMode StringToShadingMode(const stringImpl& name) {
        for (U8 i = 0; i < to_U8(ShadingMode::COUNT); ++i) {
            if (strcmp(name.c_str(), Names::shadingMode[i]) == 0) {
                return static_cast<ShadingMode>(i);
            }
        }

        return ShadingMode::COUNT;
    }

    const char* TextureOperationToString(TextureOperation textureOp) noexcept {
        return Names::textureOperation[to_base(textureOp)];
    }

    TextureOperation StringToTextureOperation(const stringImpl& operation) {
        for (U8 i = 0; i < to_U8(TextureOperation::COUNT); ++i) {
            if (strcmp(operation.c_str(), Names::textureOperation[i]) == 0) {
                return static_cast<TextureOperation>(i);
            }
        }

        return TextureOperation::COUNT;
    }
};

void Material::ApplyDefaultStateBlocks(Material& target) {
    /// Normal state for final rendering
    RenderStateBlock stateDescriptor = {};
    stateDescriptor.setCullMode(target.doubleSided() ? CullMode::NONE : CullMode::BACK);
    stateDescriptor.setZFunc(ComparisonFunction::EQUAL);

    RenderStateBlock oitDescriptor(stateDescriptor);
    oitDescriptor.setZFunc(ComparisonFunction::LEQUAL);
    oitDescriptor.depthTestEnabled(true);

    /// the z-pre-pass descriptor does not process colours
    RenderStateBlock zPrePassDescriptor(stateDescriptor);
    zPrePassDescriptor.setColourWrites(true, true, true, true);
    zPrePassDescriptor.setZFunc(ComparisonFunction::LEQUAL);

    /// A descriptor used for rendering to depth map
    RenderStateBlock shadowDescriptor(stateDescriptor);
    shadowDescriptor.setColourWrites(true, true, false, false);
    shadowDescriptor.setZFunc(ComparisonFunction::LESS);

    target.setRenderStateBlock(zPrePassDescriptor.getHash(), RenderStage::COUNT,  RenderPassType::PRE_PASS);
    target.setRenderStateBlock(stateDescriptor.getHash(),    RenderStage::COUNT,  RenderPassType::MAIN_PASS);
    target.setRenderStateBlock(oitDescriptor.getHash(),      RenderStage::COUNT,  RenderPassType::OIT_PASS);
    target.setRenderStateBlock(shadowDescriptor.getHash(),   RenderStage::SHADOW, RenderPassType::COUNT);
}

Material::Material(GFXDevice& context, ResourceCache* parentCache, size_t descriptorHash, const Str256& name)
    : CachedResource(ResourceType::DEFAULT, descriptorHash, name),
      _context(context),
      _parentCache(parentCache)
{
    const ShaderProgramInfo defaultShaderInfo = {};
    // Could just direct copy the arrays, but this looks cool
    for (U8 s = 0u; s < to_U8(RenderStage::COUNT); ++s) {
        ShaderVariantsPerPass& perPassInfo = _shaderInfo[s];
        StateVariantsPerPass& perPassStates = _defaultRenderStates[s];

        for (U8 p = 0u; p < to_U8(RenderPassType::COUNT); ++p) {
            perPassInfo[p].fill(defaultShaderInfo);
            perPassStates[p].fill(g_invalidStateHash);
        }
    }

    ApplyDefaultStateBlocks(*this);
}

Material_ptr Material::clone(const Str256& nameSuffix) {
    DIVIDE_ASSERT(!nameSuffix.empty(),
                  "Material error: clone called without a valid name suffix!");

    const Material& base = *this;
    Material_ptr cloneMat = CreateResource<Material>(_parentCache, ResourceDescriptor(resourceName() + nameSuffix.c_str()));

    {
        cloneMat->_baseMaterial = this;
        _instances.emplace_back(cloneMat.get());
    }

    cloneMat->_baseColour = base._baseColour;
    cloneMat->_emissive = base._emissive;
    cloneMat->_metallic = base._metallic;
    cloneMat->_roughness = base._roughness;
    cloneMat->_parallaxFactor = base._parallaxFactor;
    cloneMat->_shadingMode = base._shadingMode;
    cloneMat->_translucencySource = base._translucencySource;
    cloneMat->_textureOperation = base._textureOperation;
    cloneMat->_bumpMethod = base._bumpMethod;
    cloneMat->_doubleSided = base._doubleSided;
    cloneMat->_transparencyEnabled = base._transparencyEnabled;
    cloneMat->_receivesShadows = base._receivesShadows;
    cloneMat->_isStatic = base._isStatic;
    cloneMat->_hardwareSkinning = base._hardwareSkinning;
    cloneMat->_isRefractive = base._isRefractive;
    cloneMat->_textureUseForDepth = _textureUseForDepth;
    cloneMat->_extraShaderDefines = base._extraShaderDefines;

    cloneMat->baseShaderData(base.baseShaderData());
    cloneMat->ignoreXMLData(base.ignoreXMLData());

    // Could just direct copy the arrays, but this looks cool
    for (U8 s = 0u; s < to_U8(RenderStage::COUNT); ++s) {
        for (U8 p = 0u; p < to_U8(RenderPassType::COUNT); ++p) {
            for (U8 v = 0u; v < g_maxVariantsPerPass; ++v) {
                cloneMat->_shaderInfo[s][p][v] = _shaderInfo[s][p][v];
                cloneMat->_defaultRenderStates[s][p][v] = _defaultRenderStates[s][p][v];
            }
        }
    }

    for (U8 i = 0; i < to_U8(base._textures.size()); ++i) {
        const Texture_ptr& tex = base._textures[i];
        if (tex) {
            cloneMat->setTexture(static_cast<TextureUsage>(i), tex);
        }
    }
    cloneMat->_needsNewShader = true;

    return cloneMat;
}

bool Material::update(const U64 deltaTimeUS) {
    if (_needsNewShader) {
        recomputeShaders();
        _needsNewShader = false;
        return true;
    }

    return false;
}

// base = base texture
// second = second texture used for multitexturing
// bump = bump map
bool Material::setTexture(TextureUsage textureUsageSlot,
                          const Texture_ptr& texture,
                          const TextureOperation& op,
                          bool applyToInstances) 
{
    if (applyToInstances) {
        for (Material* instance : _instances) {
            instance->setTexture(textureUsageSlot, texture, op, true);
        }
    }


    bool computeShaders = false;
    const U32 slot = to_U32(textureUsageSlot);

    if (textureUsageSlot == TextureUsage::UNIT1) {
        _textureOperation = op;
    }
    
    assert(textureUsageSlot != TextureUsage::REFLECTION_PLANAR &&
           textureUsageSlot != TextureUsage::REFRACTION_PLANAR &&
           textureUsageSlot != TextureUsage::REFLECTION_CUBE);

    {
        UniqueLock<SharedMutex> w_lock(_textureLock);
        if (!_textures[slot]) {
            // if we add a new type of texture recompute shaders
            computeShaders = true;
        }
        else {
            // Skip adding same texture
            if (texture != nullptr && _textures[slot]->getGUID() == texture->getGUID()) {
                return true;
            }
        }

        _textures[slot] = texture;


        if (textureUsageSlot == TextureUsage::NORMALMAP ||
            textureUsageSlot == TextureUsage::HEIGHTMAP)
        {
            const bool isHeight = textureUsageSlot == TextureUsage::HEIGHTMAP;

            // If we have the opacity texture is the albedo map, we don't need it. We can just use albedo alpha
            const Texture_ptr& otherTex = _textures[isHeight ? to_base(TextureUsage::NORMALMAP)
                                                             : to_base(TextureUsage::HEIGHTMAP)];

            if (otherTex != nullptr && texture != nullptr && otherTex->data()._textureHandle == texture->data()._textureHandle) {
                _textures[to_base(TextureUsage::HEIGHTMAP)] = nullptr;
            }
        }

        if (textureUsageSlot == TextureUsage::UNIT0 ||
            textureUsageSlot == TextureUsage::OPACITY)
        {
            bool isOpacity = true;
            if (textureUsageSlot == TextureUsage::UNIT0) {
                _textureKeyCache = texture == nullptr ? -1 : texture->data()._textureHandle;
                isOpacity = false;
            }

            // If we have the opacity texture is the albedo map, we don't need it. We can just use albedo alpha
            const Texture_ptr& otherTex = _textures[isOpacity ? to_base(TextureUsage::UNIT0)
                                                              : to_base(TextureUsage::OPACITY)];

            if (otherTex != nullptr && texture != nullptr && otherTex->data()._textureHandle == texture->data()._textureHandle) {
                _textures[to_base(TextureUsage::OPACITY)] = nullptr;
            }

            updateTransparency();
        }
    }

    _needsNewShader = true;

    return true;
}

void Material::setShaderProgramInternal(const ShaderProgram_ptr& shader,
                                        ShaderProgramInfo& shaderInfo,
                                        RenderStage stage,
                                        RenderPassType pass,
                                        U8 variant)
{
    if (shader != nullptr) {
        ShaderProgram* oldShader = shaderInfo._shaderRef.get();
        if (oldShader != nullptr) {
            const char* newShaderName = shader == nullptr ? nullptr : shader->resourceName().c_str();

            if (newShaderName == nullptr || strlen(newShaderName) == 0 || oldShader->resourceName().compare(newShaderName) != 0) {
                // We cannot replace a shader that is still loading in the background
                WAIT_FOR_CONDITION(oldShader->getState() == ResourceState::RES_LOADED);
                    Console::printfn(Locale::get(_ID("REPLACE_SHADER")),
                        oldShader->resourceName().c_str(),
                        newShaderName != nullptr ? newShaderName : "NULL",
                        TypeUtil::RenderStageToString(stage),
                        TypeUtil::RenderPassTypeToString(pass),
                        variant);
            }
        }
    }

    shaderInfo._shaderRef = shader;
    shaderInfo._shaderCompStage = (shader == nullptr || shader->getState() == ResourceState::RES_LOADED)
                                                     ? ShaderBuildStage::READY
                                                     : ShaderBuildStage::COMPUTED;
}

void Material::setShaderProgramInternal(const ResourceDescriptor& shaderDescriptor,
                                        const RenderStagePass& stagePass,
                                        const bool computeOnAdd) {
    OPTICK_EVENT();

    ShaderProgramInfo& info = shaderInfo(stagePass);
    // if we already have a different shader assigned ...
    if (info._shaderRef != nullptr && info._shaderRef->resourceName().compare(shaderDescriptor.resourceName()) != 0)
    {
        // We cannot replace a shader that is still loading in the background
        WAIT_FOR_CONDITION(info._shaderRef->getState() == ResourceState::RES_LOADED);
        Console::printfn(Locale::get(_ID("REPLACE_SHADER")),
            info._shaderRef->resourceName().c_str(),
            shaderDescriptor.resourceName().c_str(),
            TypeUtil::RenderStageToString(stagePass._stage),
            TypeUtil::RenderPassTypeToString(stagePass._passType),
            stagePass._variant);
    }

    ShaderComputeQueue::ShaderQueueElement shaderElement{ info._shaderRef, shaderDescriptor };
    if (computeOnAdd) {
        _context.shaderComputeQueue().process(shaderElement);
        info._shaderCompStage = ShaderBuildStage::COMPUTED;
    } else {
        _context.shaderComputeQueue().addToQueueBack(shaderElement);
        info._shaderCompStage = ShaderBuildStage::QUEUED;
    }
}

void Material::recomputeShaders() {
    OPTICK_EVENT();

    for (U8 s = 0u; s < to_U8(RenderStage::COUNT); ++s) {
        for (U8 p = 0u; p < to_U8(RenderPassType::COUNT); ++p) {
            RenderStagePass stagePass{ static_cast<RenderStage>(s), static_cast<RenderPassType>(p) };
            ShaderPerVariant& variantMap = _shaderInfo[s][p];

            for (U8 v = 0; v < g_maxVariantsPerPass; ++v) {
                ShaderProgramInfo& shaderInfo = variantMap[v];
                if (!shaderInfo._customShader && shaderInfo._shaderCompStage != ShaderBuildStage::COUNT) {
                    stagePass._variant = v;
                    shaderInfo._shaderCompStage = ShaderBuildStage::REQUESTED;
                    computeShader(stagePass);
                }
            }
        }
    }
}

I64 Material::computeAndGetProgramGUID(const RenderStagePass& renderStagePass) {
    constexpr U8 maxRetries = 250;

    if (!canDraw(renderStagePass)) {
        computeShader(renderStagePass);
    }

    for (U8 i = 0; i < maxRetries; ++i) {
        if (!canDraw(renderStagePass)) {
            _context.shaderComputeQueue().stepQueue();
        } else {
            return getProgramGUID(renderStagePass);
        }
    }

    return ShaderProgram::defaultShader()->getGUID();
}

I64 Material::getProgramGUID(const RenderStagePass& renderStagePass) const {

    const ShaderProgramInfo& info = shaderInfo(renderStagePass);

    if (info._shaderRef != nullptr && info._shaderRef->getState() == ResourceState::RES_LOADED) {
        return info._shaderRef->getGUID();
    }
    DIVIDE_UNEXPECTED_CALL();

    return ShaderProgram::defaultShader()->getGUID();
}

bool Material::canDraw(const RenderStagePass& renderStagePass) {
    OPTICK_EVENT();

    ShaderProgramInfo& info = shaderInfo(renderStagePass);
    if (info._shaderCompStage == ShaderBuildStage::QUEUED && info._shaderRef != nullptr) {
        info._shaderCompStage = ShaderBuildStage::COMPUTED;
    }

    if (info._shaderCompStage == ShaderBuildStage::COMPUTED) {
        assert(info._shaderRef != nullptr);
        if (info._shaderRef->getState() != ResourceState::RES_LOADED) {
            return false;
        }

        info._shaderCompStage = ShaderBuildStage::READY;
    }

    if (info._shaderCompStage != ShaderBuildStage::READY) {
        return info._customShader ? false : computeShader(renderStagePass);
    }

    return true;
}

/// If the current material doesn't have a shader associated with it, then add the default ones.
bool Material::computeShader(const RenderStagePass& renderStagePass) {
    OPTICK_EVENT();

    const bool isDepthPass = renderStagePass.isDepthPass();
    const bool isShadowPass = renderStagePass._stage == RenderStage::SHADOW;

    ShaderProgramInfo& info = shaderInfo(renderStagePass);
    // If shader's invalid, try to request a recompute as it might fix it
    if (info._shaderCompStage == ShaderBuildStage::COUNT) {
        info._shaderCompStage = ShaderBuildStage::REQUESTED;
    } else if (info._shaderCompStage != ShaderBuildStage::REQUESTED) {
        // If the shader is valid and a recompute wasn't requested, just return true
        return info._shaderCompStage == ShaderBuildStage::READY;
    }

    // At this point, only computation requests are processed
    constexpr U32 slot0 = to_base(TextureUsage::UNIT0);
    constexpr U32 slot1 = to_base(TextureUsage::UNIT1);
    constexpr U32 slotOpacity = to_base(TextureUsage::OPACITY);

    DIVIDE_ASSERT(_shadingMode != ShadingMode::COUNT, "Material computeShader error: Invalid shading mode specified!");


    ModuleDefines vertDefines = {};
    ModuleDefines fragDefines = {};
    ModuleDefines globalDefines = {};

    vertDefines.insert(std::cend(vertDefines),
                       std::cbegin(_extraShaderDefines[to_base(ShaderType::VERTEX)]),
                       std::cend(_extraShaderDefines[to_base(ShaderType::VERTEX)]));

    fragDefines.insert(std::cend(fragDefines),
                       std::cbegin(_extraShaderDefines[to_base(ShaderType::FRAGMENT)]),
                       std::cend(_extraShaderDefines[to_base(ShaderType::FRAGMENT)]));
    
    vertDefines.emplace_back("USE_CUSTOM_CLIP_PLANES", true);

    if (_textures[slot1]) {
        if (!_textures[slot0]) {
            std::swap(_textures[slot0], _textures[slot1]);
            updateTransparency();
        }
    }
    const Str64 vertSource = isDepthPass ? baseShaderData()._depthShaderVertSource : baseShaderData()._colourShaderVertSource;
    const Str64 fragSource = isDepthPass ? baseShaderData()._depthShaderFragSource : baseShaderData()._colourShaderFragSource;

    Str32 vertVariant = isDepthPass 
                            ? isShadowPass 
                                ? baseShaderData()._shadowShaderVertVariant
                                : baseShaderData()._depthShaderVertVariant
                            : baseShaderData()._colourShaderVertVariant;
    Str32 fragVariant = isDepthPass ? baseShaderData()._depthShaderFragVariant : baseShaderData()._colourShaderFragVariant;
    Str256 shaderName = vertSource + "_" + fragSource;


    if (isDepthPass) {
        if (renderStagePass._stage == RenderStage::SHADOW) {
            shaderName += ".SHDW";
            vertVariant += "Shadow";
            fragVariant += "Shadow.VSM";
            globalDefines.emplace_back("SHADOW_PASS", true);
        } else {
            shaderName += ".PP";
            vertVariant += "PrePass";
            fragVariant += "PrePass";
            globalDefines.emplace_back("PRE_PASS", true);
        }
    }

    if (renderStagePass._passType == RenderPassType::OIT_PASS) {
        shaderName += ".OIT";
        fragDefines.emplace_back("OIT_PASS", true);
    }

    if (!_textures[slot0]) {
        fragDefines.emplace_back("SKIP_TEX0", true);
        shaderName += ".NTex0";
    }

    // Display pre-pass caches normal maps in a GBuffer, so it's the only exception
    if ((!isDepthPass || renderStagePass._stage == RenderStage::DISPLAY) &&
        _textures[to_base(TextureUsage::NORMALMAP)] != nullptr &&
        _bumpMethod != BumpMethod::NONE) 
    {
        // Bump mapping?
        globalDefines.emplace_back("COMPUTE_TBN", true);
    }

    if (!isDepthPass && _textures[to_base(TextureUsage::OCCLUSION_METALLIC_ROUGHNESS)] != nullptr) {
        shaderName += ".MRgh";
        fragDefines.emplace_back("USE_OCCLUSION_METALLIC_ROUGHNESS_MAP", true);
    }

    updateTransparency();
    if (hasTransparency()) {
        if (renderStagePass._passType != RenderPassType::OIT_PASS) {
            shaderName += ".AD";
            fragDefines.emplace_back("USE_ALPHA_DISCARD", true);
        }

        switch (_translucencySource) {
            case TranslucencySource::OPACITY_MAP_A:
            case TranslucencySource::OPACITY_MAP_R: {
                fragDefines.emplace_back("USE_OPACITY_MAP", true);
                if (_translucencySource == TranslucencySource::OPACITY_MAP_R) {
                    shaderName += ".OMapR";
                    fragDefines.emplace_back("USE_OPACITY_MAP_RED_CHANNEL", true);
                } else {
                    shaderName += ".OMapA";
                }
            } break;
            case TranslucencySource::ALBEDO: {
                shaderName += ".AAlpha";
                fragDefines.emplace_back("USE_ALBEDO_ALPHA", true);
            } break;
            default: break;
        };
    }

    if (doubleSided()) {
        shaderName += ".2Sided";
        fragDefines.emplace_back("USE_DOUBLE_SIDED", true);
    }

    if (!receivesShadows()) {
        shaderName += ".NSHDW";
        fragDefines.emplace_back("DISABLE_SHADOW_MAPPING", true);
    }

    if (!_isStatic) {
        shaderName += ".D";
        vertDefines.emplace_back("NODE_DYNAMIC", true);
        fragDefines.emplace_back("NODE_DYNAMIC", true);
    } else {
        shaderName += ".S";
        vertDefines.emplace_back("NODE_STATIC", true);
        fragDefines.emplace_back("NODE_STATIC", true);
    }

    if (!isDepthPass) {
        switch (_shadingMode) {
            default:
            case ShadingMode::FLAT: {
                fragDefines.emplace_back("USE_SHADING_FLAT", true);
                shaderName += ".Flat";
            } break;
            case ShadingMode::PHONG:
            case ShadingMode::BLINN_PHONG: {
                fragDefines.emplace_back("USE_SHADING_BLINN_PHONG", true);
                shaderName += ".Phong";
            } break;
            case ShadingMode::TOON: {
                fragDefines.emplace_back("USE_SHADING_TOON", true);
                shaderName += ".Toon";
            } break;
            case ShadingMode::OREN_NAYAR: {
                fragDefines.emplace_back("USE_SHADING_OREN_NAYAR", true);
                shaderName += ".OrenN";
            } break;
            case ShadingMode::COOK_TORRANCE: {
                fragDefines.emplace_back("USE_SHADING_COOK_TORRANCE", true);
                shaderName += ".CookT";
            } break;
        }
    }

    // Add the GPU skinning module to the vertex shader?
    if (_hardwareSkinning) {
        vertDefines.emplace_back("USE_GPU_SKINNING", true);
        shaderName += ".Sknd";
    }

    if (renderStagePass._stage == RenderStage::DISPLAY) {
        if (!isDepthPass) {
            fragDefines.emplace_back("USE_SSAO", true);
            shaderName += ".SSAO";
        }
        fragDefines.emplace_back("USE_DEFERRED_NORMALS", true);
        shaderName += ".DNrmls";
        if (!hasTransparency()) {
            fragDefines.emplace_back("HAS_VELOCITY", true);
            vertDefines.emplace_back("HAS_VELOCITY", true);
            shaderName += ".Vel";
        }
    }

    globalDefines.emplace_back("DEFINE_PLACEHOLDER", false);

    vertDefines.insert(std::cend(vertDefines), std::cbegin(globalDefines), std::cend(globalDefines));
    fragDefines.insert(std::cend(fragDefines), std::cbegin(globalDefines), std::cend(globalDefines));

    shaderName.append(
        Util::StringFormat("_%zu_%zu",
                           ShaderProgram::definesHash(vertDefines),
                           ShaderProgram::definesHash(fragDefines))
    );

    ShaderModuleDescriptor vertModule = {};
    vertModule._variant = vertVariant;
    vertModule._sourceFile = (vertSource + ".glsl").c_str();
    vertModule._batchSameFile = false;
    vertModule._moduleType = ShaderType::VERTEX;
    vertModule._defines = vertDefines;

    ShaderModuleDescriptor fragModule = {};
    fragModule._variant = fragVariant;
    fragModule._sourceFile = (fragSource + ".glsl").c_str();
    fragModule._moduleType = ShaderType::FRAGMENT;
    fragModule._defines = fragDefines;

    ShaderProgramDescriptor shaderDescriptor = {};
    shaderDescriptor._modules.push_back(vertModule);
    shaderDescriptor._modules.push_back(fragModule);

    ResourceDescriptor shaderResDescriptor(shaderName);
    shaderResDescriptor.propertyDescriptor(shaderDescriptor);
    shaderResDescriptor.flag(true);

    setShaderProgramInternal(shaderResDescriptor, renderStagePass, false);

    return false;
}

bool Material::getTextureData(TextureUsage slot, TextureDataContainer<>& container) {
    const U8 slotValue = to_U8(slot);

    TextureData data = {};
    {
        SharedLock<SharedMutex> r_lock(_textureLock);
        const Texture_ptr& crtTexture = _textures[slotValue];
        if (crtTexture != nullptr) {
            data = crtTexture->data();
        }
    }

    if (data._textureType != TextureType::COUNT) {
        return container.setTexture(data, slotValue) != TextureUpdateState::NOTHING;
    }

    return false;
}

bool Material::getTextureData(const RenderStagePass& renderStagePass, TextureDataContainer<>& textureData) {
    OPTICK_EVENT();

    if (textureData.count() == 0) {
        return getTextureDataFast(renderStagePass, textureData);
    }

    const bool isDepthPass = renderStagePass.isDepthPass();

    bool ret = false;
    if (!isDepthPass || hasTransparency() || _textureUseForDepth[to_base(TextureUsage::UNIT0)]) {
        ret = getTextureData(TextureUsage::UNIT0, textureData) || ret;
    }

    if (!isDepthPass || hasTransparency() || _textureUseForDepth[to_base(TextureUsage::OPACITY)]) {
        ret = getTextureData(TextureUsage::OPACITY, textureData) || ret;
    }

    if (!isDepthPass || _textureUseForDepth[to_base(TextureUsage::HEIGHTMAP)]) {
        ret = getTextureData(TextureUsage::HEIGHTMAP, textureData) || ret;
    }
    if (!isDepthPass || _textureUseForDepth[to_base(TextureUsage::PROJECTION)]) {
        ret = getTextureData(TextureUsage::PROJECTION, textureData) || ret;
    }

    if (!isDepthPass || _textureUseForDepth[to_base(TextureUsage::UNIT1)]) {
        ret = getTextureData(TextureUsage::UNIT1, textureData) || ret;
    }

    if (!isDepthPass || _textureUseForDepth[to_base(TextureUsage::OCCLUSION_METALLIC_ROUGHNESS)]) {
        ret = getTextureData(TextureUsage::OCCLUSION_METALLIC_ROUGHNESS, textureData) || ret;
    }

    if (renderStagePass._stage != RenderStage::SHADOW && // not shadow pass
       !(renderStagePass._stage == RenderStage::DISPLAY && renderStagePass._passType != RenderPassType::PRE_PASS)) //not Display::PrePass
    {
        ret = getTextureData(TextureUsage::NORMALMAP, textureData) || ret;
    }

    return ret;
}

bool Material::getTextureDataFast(const RenderStagePass& renderStagePass, TextureDataContainer<>& textureData) {
    OPTICK_EVENT();

    constexpr U8 transparentSlots[] = {
        to_base(TextureUsage::UNIT0),
        to_base(TextureUsage::OPACITY)
    };

    constexpr U8 extraSlots[] = {
        to_base(TextureUsage::UNIT1),
        to_base(TextureUsage::OCCLUSION_METALLIC_ROUGHNESS),
        to_base(TextureUsage::HEIGHTMAP),
        to_base(TextureUsage::PROJECTION)
    };

    bool ret = false;
    SharedLock<SharedMutex> r_lock(_textureLock);

    const bool depthStage = renderStagePass.isDepthPass();
    for (const U8 slot : transparentSlots) {
        if (!depthStage || hasTransparency() || _textureUseForDepth[slot]) {
            const Texture_ptr& crtTexture = _textures[slot];
            if (crtTexture != nullptr) {
                textureData.setTexture(crtTexture->data(), slot);
                ret = true;
            }
        }
    }

    for (const U8 slot : extraSlots) {
        if (!depthStage || _textureUseForDepth[slot]) {
            const Texture_ptr& crtTexture = _textures[slot];
            if (crtTexture != nullptr) {
                textureData.setTexture(crtTexture->data(), slot);
                ret = true;
            }
        }
    }

    if (renderStagePass._stage != RenderStage::SHADOW && // not shadow pass
        !(renderStagePass._stage == RenderStage::DISPLAY && renderStagePass._passType != RenderPassType::PRE_PASS)) //everything apart from Display::PrePass
    {
        constexpr U8 normalSlot = to_base(TextureUsage::NORMALMAP);
        const Texture_ptr& crtNormalTexture = _textures[normalSlot];
        if (crtNormalTexture != nullptr) {
            textureData.setTexture(crtNormalTexture->data(), normalSlot);
            ret = true;
        }
    }

    return ret;
}

bool Material::unload() {
    _textures.fill(nullptr);

    static ShaderProgramInfo defaultShaderInfo = {};

    for (U8 s = 0u; s < to_U8(RenderStage::COUNT); ++s) {
        ShaderVariantsPerPass& passMapShaders = _shaderInfo[s];
        StateVariantsPerPass& passMapStates = _defaultRenderStates[s];
        for (U8 p = 0u; p < to_U8(RenderPassType::COUNT); ++p) {
            passMapShaders[p].fill(defaultShaderInfo);
            passMapStates[p].fill(g_invalidStateHash);
        }
    }
    
    if (_baseMaterial != nullptr) {
        const I64 guid = getGUID();
        eastl::erase_if(_baseMaterial->_instances,
                        [guid](Material* instance) {
                            return instance->getGUID() == guid;
                        });
    }

    for (Material* instance : _instances) {
        instance->_baseMaterial = nullptr;
    }

    return true;
}

void Material::refractive(const bool state, bool applyToInstances) {
    if (_isRefractive != state) {
        _isRefractive = state;
        _needsNewShader = true;
    }

    if (applyToInstances) {
        for (Material* instance : _instances) {
            instance->refractive(state, true);
        }
    }
}

void Material::doubleSided(const bool state, bool applyToInstances) {
    if (_doubleSided != state) {
        _doubleSided = state;
        _needsNewShader = true;
    }

    if (applyToInstances) {
        for (Material* instance : _instances) {
            instance->doubleSided(state, true);
        }
    }
}

void Material::receivesShadows(const bool state, bool applyToInstances) {
    if (_receivesShadows != state) {
        _receivesShadows = state;
        _needsNewShader = true;
    }

    if (applyToInstances) {
        for (Material* instance : _instances) {
            instance->receivesShadows(state, true);
        }
    }
}

void Material::isStatic(const bool state, bool applyToInstances) {
    if (_isStatic != state) {
        _isStatic = state;
        _needsNewShader = true;
    }

    if (applyToInstances) {
        for (Material* instance : _instances) {
            instance->isStatic(state, true);
        }
    }
}

void Material::hardwareSkinning(const bool state, bool applyToInstances) {
    if (_hardwareSkinning != state) {
        _hardwareSkinning = state;
        _needsNewShader = true;
    }

    if (applyToInstances) {
        for (Material* instance : _instances) {
            instance->hardwareSkinning(state, true);
        }
    }
}

void Material::textureUseForDepth(TextureUsage slot, bool state, bool applyToInstances) {
    _textureUseForDepth[to_base(slot)] = state;

    if (applyToInstances) {
        for (Material* instance : _instances) {
            instance->textureUseForDepth(slot, state, true);
        }
    }
}

void Material::setShaderProgram(const ShaderProgram_ptr& shader, RenderStage stage, RenderPassType pass, U8 variant) {
    assert(variant == g_AllVariantsID || variant < g_maxVariantsPerPass);

    for (U8 s = 0u; s < to_U8(RenderStage::COUNT); ++s) {
        for (U8 p = 0u; p < to_U8(RenderPassType::COUNT); ++p) {
            const RenderStage crtStage = static_cast<RenderStage>(s);
            const RenderPassType crtPass = static_cast<RenderPassType>(p);
            if ((stage == RenderStage::COUNT || stage == crtStage) && (pass == RenderPassType::COUNT || pass == crtPass)) {
                if (variant == g_AllVariantsID) {
                    for (U8 i = 0u; i < g_maxVariantsPerPass; ++i) {
                        ShaderProgramInfo& shaderInfo = _shaderInfo[s][p][i];
                        shaderInfo._customShader = true;
                        shaderInfo._shaderCompStage = ShaderBuildStage::COUNT;
                        setShaderProgramInternal(shader, shaderInfo, crtStage, crtPass, variant);
                    }
                } else {
                    ShaderProgramInfo& shaderInfo = _shaderInfo[s][p][variant];
                    shaderInfo._customShader = true;
                    shaderInfo._shaderCompStage = ShaderBuildStage::COUNT;
                    setShaderProgramInternal(shader, shaderInfo, crtStage, crtPass, variant);
                }
            }
        }
    }
}

void Material::setRenderStateBlock(size_t renderStateBlockHash, RenderStage stage, RenderPassType pass, U8 variant) {
    assert(variant == g_AllVariantsID || variant < g_maxVariantsPerPass);

    for (U8 s = 0u; s < to_U8(RenderStage::COUNT); ++s) {
        for (U8 p = 0u; p < to_U8(RenderPassType::COUNT); ++p) {
            const RenderStage crtStage = static_cast<RenderStage>(s);
            const RenderPassType crtPass = static_cast<RenderPassType>(p);
            if ((stage == RenderStage::COUNT || stage == crtStage) && (pass == RenderPassType::COUNT || pass == crtPass)) {
                if (variant == g_AllVariantsID) {
                    _defaultRenderStates[s][p].fill(renderStateBlockHash);
                } else {
                    _defaultRenderStates[s][p][variant] = renderStateBlockHash;
                }
            }
        }
    }
}

void Material::toggleTransparency(const bool state, bool applyToInstances) {
    if (_transparencyEnabled != state) {
        _transparencyEnabled = state;
        _needsNewShader = true;
    }

    if (applyToInstances) {
        for (Material* instance : _instances) {
            instance->toggleTransparency(state, true);
        }
    }
}

void Material::baseColour(const FColour4& colour, bool applyToInstances) {
    _baseColour = colour;
    updateTransparency();

    if (applyToInstances) {
        for (Material* instance : _instances) {
            instance->baseColour(colour, true);
        }
    }
}

void Material::emissiveColour(const FColour3& colour, bool applyToInstances) {
    _emissive = colour;

    if (applyToInstances) {
        for (Material* instance : _instances) {
            instance->emissiveColour(colour, true);
        }
    }
}

void Material::metallic(F32 value, bool applyToInstances) {
    _metallic = CLAMPED_01(value);

    if (applyToInstances) {
        for (Material* instance : _instances) {
            instance->metallic(value, true);
        }
    }
}

void Material::roughness(F32 value, bool applyToInstances) {
    _roughness = CLAMPED_01(value);

    if (applyToInstances) {
        for (Material* instance : _instances) {
            instance->roughness(value, true);
        }
    }
}

void Material::parallaxFactor(F32 factor, bool applyToInstances) {
    _parallaxFactor = CLAMPED_01(factor);

    if (applyToInstances) {
        for (Material* instance : _instances) {
            instance->parallaxFactor(factor, true);
        }
    }
}

void Material::bumpMethod(BumpMethod newBumpMethod, bool applyToInstances) {
    if (_bumpMethod != newBumpMethod) {
        _bumpMethod = newBumpMethod;
        _needsNewShader = true;
    }

    if (applyToInstances) {
        for (Material* instance : _instances) {
            instance->bumpMethod(newBumpMethod, true);
        }
    }
}

void Material::shadingMode(ShadingMode mode, bool applyToInstances) {
    if (_shadingMode != mode) {
        _shadingMode = mode;
        _needsNewShader = true;
    }

    if (applyToInstances) {
        for (Material* instance : _instances) {
            instance->shadingMode(mode, true);
        }
    }
}

void Material::updateTransparency() {
    if (!_transparencyEnabled) {
        return;
    }

    const TranslucencySource oldSource = _translucencySource;
    _translucencySource = TranslucencySource::COUNT;

    // In order of importance (less to more)!
    // diffuse channel alpha
    if (baseColour().a < 0.95f) {
        _translucencySource = TranslucencySource::ALBEDO;
    }

    bool usingAlbedoTexAlpha = false;
    bool usingOpacityTexAlpha = false;
    // base texture is translucent
    const Texture_ptr& albedo = _textures[to_base(TextureUsage::UNIT0)];
    if (albedo && albedo->hasTransparency()) {
        _translucencySource = TranslucencySource::ALBEDO;

        usingAlbedoTexAlpha = oldSource != _translucencySource;
    }

    // opacity map
    const Texture_ptr& opacity = _textures[to_base(TextureUsage::OPACITY)];
    if (opacity && opacity->hasTransparency()) {
        const U8 channelCount = opacity->descriptor().numChannels();
        DIVIDE_ASSERT(channelCount == 1 || channelCount == 4, "Material::updateTranslucency: Opacity textures must be either single-channel or RGBA!");

        _translucencySource = channelCount == 4 ? TranslucencySource::OPACITY_MAP_A : TranslucencySource::OPACITY_MAP_R;
        usingAlbedoTexAlpha = false;
        usingOpacityTexAlpha = oldSource != _translucencySource;
    }

    if (usingAlbedoTexAlpha || usingOpacityTexAlpha) {
        Texture* tex = (usingOpacityTexAlpha ? opacity.get() : albedo.get());

        const U16 baseLevel = tex->getBaseMipLevel();
        const U16 maxLevel = tex->getMaxMipLevel();
        const U16 baseOffset = maxLevel > g_numMipsToKeepFromAlphaTextures ? g_numMipsToKeepFromAlphaTextures : maxLevel;
        STUBBED("HACK! Limit mip range for textures that have alpha values used by the material -Ionut");
        if (tex->getMipCount() == maxLevel) {
            tex->setMipMapRange(baseLevel, baseOffset);
        }
    }

    _needsNewShader = oldSource != _translucencySource;
}

size_t Material::getRenderStateBlock(const RenderStagePass& renderStagePass) const {
    size_t ret = _context.getDefaultStateBlock(false);

    const StatesPerVariant& variantMap = _defaultRenderStates[to_base(renderStagePass._stage)][to_base(renderStagePass._passType)];

    assert(renderStagePass._variant < g_maxVariantsPerPass);

    ret = variantMap[renderStagePass._variant];
    // If we haven't defined a state for this variant, use the default one
    if (ret == g_invalidStateHash) {
        ret = variantMap[0u];
    }

    if (ret != g_invalidStateHash) {
        // We don't need to update cull params for shadow mapping unless this is a directional light
        // since CSM splits cause all sorts of errors
        if (_doubleSided) {
            RenderStateBlock tempBlock = RenderStateBlock::get(ret);
            tempBlock.setCullMode(CullMode::NONE);
            ret = tempBlock.getHash();
        }
    }

    return ret;
}

void Material::getSortKeys(const RenderStagePass& renderStagePass, I64& shaderKey, I32& textureKey) const {
    textureKey = _textureKeyCache == -1 ? std::numeric_limits<I32>::lowest() : _textureKeyCache;

    const ShaderProgramInfo& info = shaderInfo(renderStagePass);
    if (info._shaderCompStage == ShaderBuildStage::READY) {
        shaderKey = info._shaderRef->getGUID();
    } else {
        shaderKey = std::numeric_limits<I64>::lowest();
    }
}

FColour4 Material::getBaseColour(bool& hasTextureOverride, Texture*& textureOut) const noexcept {
    textureOut = nullptr;
    hasTextureOverride = _textures[to_base(TextureUsage::UNIT0)] != nullptr;
    if (hasTextureOverride) {
        textureOut = _textures[to_base(TextureUsage::UNIT0)].get();
    }
    return baseColour();
}

FColour3 Material::getEmissive(bool& hasTextureOverride, Texture*& textureOut) const noexcept {
    textureOut = nullptr;
    hasTextureOverride = false;

    return emissive();
}

F32 Material::getMetallic(bool& hasTextureOverride, Texture*& textureOut) const noexcept {
    textureOut = nullptr;
    hasTextureOverride = _textures[to_base(TextureUsage::OCCLUSION_METALLIC_ROUGHNESS)] != nullptr;
    if (hasTextureOverride) {
        textureOut = _textures[to_base(TextureUsage::OCCLUSION_METALLIC_ROUGHNESS)].get();
    }
    return metallic();
}

F32 Material::getRoughness(bool& hasTextureOverride, Texture*& textureOut) const noexcept {
    textureOut = nullptr;
    hasTextureOverride = _textures[to_base(TextureUsage::OCCLUSION_METALLIC_ROUGHNESS)] != nullptr;
    if (hasTextureOverride) {
        textureOut = _textures[to_base(TextureUsage::OCCLUSION_METALLIC_ROUGHNESS)].get();
    }
    return roughness();
}

void Material::getMaterialMatrix(mat4<F32>& retMatrix) const {
    retMatrix.setRow(0, baseColour());
    retMatrix.setRow(1, vec4<F32>{1.0f, metallic(), roughness(), 0.0f});
    retMatrix.setRow(2, emissive());
    retMatrix.setRow(3, vec4<F32>(0.0f, parallaxFactor(), 0.0f, 0.0f));
}

void Material::rebuild() {
    recomputeShaders();

    // Alternatively we could just copy the maps directly
    for (U8 s = 0u; s < to_U8(RenderStage::COUNT); ++s) {
        for (U8 p = 0u; p < to_U8(RenderPassType::COUNT); ++p) {
            ShaderPerVariant& shaders = _shaderInfo[s][p];
            for (ShaderProgramInfo& info : shaders) {
                if (info._shaderRef != nullptr && info._shaderRef->getState() == ResourceState::RES_LOADED) {
                    info._shaderRef->recompile(true);
                }
            }
        }
    }
}

void Material::saveToXML(const stringImpl& entryName, boost::property_tree::ptree& pt) const {
    pt.put(entryName + ".version", g_materialXMLVersion);

    pt.put(entryName + ".shadingMode", TypeUtil::ShadingModeToString(shadingMode()));

    pt.put(entryName + ".colour.<xmlattr>.r", baseColour().r);
    pt.put(entryName + ".colour.<xmlattr>.g", baseColour().g);
    pt.put(entryName + ".colour.<xmlattr>.b", baseColour().b);
    pt.put(entryName + ".colour.<xmlattr>.a", baseColour().a);

    pt.put(entryName + ".emissive.<xmlattr>.r", emissive().r);
    pt.put(entryName + ".emissive.<xmlattr>.g", emissive().g);
    pt.put(entryName + ".emissive.<xmlattr>.b", emissive().b);
    pt.put(entryName + ".metallic", metallic());
    pt.put(entryName + ".roughness", roughness());

    pt.put(entryName + ".doubleSided", doubleSided());

    pt.put(entryName + ".receivesShadows", receivesShadows());

    pt.put(entryName + ".bumpMethod", TypeUtil::BumpMethodToString(bumpMethod()));

    pt.put(entryName + ".parallaxFactor", parallaxFactor());

    pt.put(entryName + ".transparencyEnabled", transparencyEnabled());

    pt.put(entryName + ".isRefractive", _isRefractive);

    saveRenderStatesToXML(entryName, pt);
    saveTextureDataToXML(entryName, pt);
}

void Material::loadFromXML(const stringImpl& entryName, const boost::property_tree::ptree& pt) {
    if (ignoreXMLData()) {
        return;
    }
    
    const size_t detectedVersion = pt.get<size_t>(entryName + ".version", 0);
    if (detectedVersion != g_materialXMLVersion) {
        Console::printfn(Locale::get(_ID("MATERIAL_WRONG_VERSION")), assetName().c_str(), detectedVersion, g_materialXMLVersion);
        return;
    }
    
    shadingMode(TypeUtil::StringToShadingMode(pt.get<stringImpl>(entryName + ".shadingMode", "FLAT")), false);
    
    baseColour(FColour4(pt.get<F32>(entryName + ".colour.<xmlattr>.r", 0.6f),
                        pt.get<F32>(entryName + ".colour.<xmlattr>.g", 0.6f),
                        pt.get<F32>(entryName + ".colour.<xmlattr>.b", 0.6f),
                        pt.get<F32>(entryName + ".colour.<xmlattr>.a", 1.f)), false);
                          
    emissiveColour(FColour3(pt.get<F32>(entryName + ".emissive.<xmlattr>.r", 0.f),
                            pt.get<F32>(entryName + ".emissive.<xmlattr>.g", 0.f),
                            pt.get<F32>(entryName + ".emissive.<xmlattr>.b", 0.f)), false);
                           
    metallic(pt.get<F32>(entryName + ".metallic", 0.2f), false);
    roughness(pt.get<F32>(entryName + ".roughness", 0.8f), false);
    
    doubleSided(pt.get<bool>(entryName + ".doubleSided", false), false);

    receivesShadows(pt.get<bool>(entryName + ".receivesShadows", true), false);

    bumpMethod(TypeUtil::StringToBumpMethod(pt.get<stringImpl>(entryName + ".bumpMethod", "NORMAL")), false);

    parallaxFactor(pt.get<F32>(entryName + ".parallaxFactor", 1.0f), false);

    toggleTransparency(pt.get<bool>(entryName + ".transparencyEnabled", true), false);

    refractive(pt.get<bool>(entryName + ".isRefractive", true), false);

    loadRenderStatesFromXML(entryName, pt);
    loadTextureDataFromXML(entryName, pt);
}

void Material::saveRenderStatesToXML(const stringImpl& entryName, boost::property_tree::ptree& pt) const {
    std::set<size_t> previousHashValues = {};
    for (U8 s = 0u; s < to_U8(RenderStage::COUNT); ++s) {
        for (U8 p = 0u; p < to_U8(RenderPassType::COUNT); ++p) {
            for (U8 v = 0u; v < g_maxVariantsPerPass; ++v) {
                // we could just use _defaultRenderStates[s][p][v] for a direct lookup, but this handles the odd double-sided / no cull case
                const size_t stateHash = getRenderStateBlock(
                    RenderStagePass{
                        static_cast<RenderStage>(s),
                        static_cast<RenderPassType>(p),
                        v
                    }
                );
                pt.put(Util::StringFormat("%s.%s.%s.%d.hash",
                            entryName.c_str(),
                            TypeUtil::RenderStageToString(static_cast<RenderStage>(s)),
                            TypeUtil::RenderPassTypeToString(static_cast<RenderPassType>(p)),
                            v), 
                        stateHash);

                if (stateHash != g_invalidStateHash && previousHashValues.find(stateHash) == std::cend(previousHashValues)) {
                    RenderStateBlock::saveToXML(
                        RenderStateBlock::get(stateHash),
                        Util::StringFormat("%s.RenderStates.%zu",
                                           entryName.c_str(),
                                           stateHash),
                        pt);
                    previousHashValues.insert(stateHash);
                }
            }
        }
    }
}

void Material::loadRenderStatesFromXML(const stringImpl& entryName, const boost::property_tree::ptree& pt) {
    RenderStateBlock block = {};

    std::set<size_t> loadedHashes = {};
    for (U8 s = 0u; s < to_U8(RenderStage::COUNT); ++s) {
        for (U8 p = 0u; p < to_U8(RenderPassType::COUNT); ++p) {
            for (U8 v = 0u; v < g_maxVariantsPerPass; ++v) {
                const size_t stateHash = pt.get<size_t>(
                    Util::StringFormat("%s.%s.%s.%d.hash", 
                            entryName.c_str(),
                            TypeUtil::RenderStageToString(static_cast<RenderStage>(s)),
                            TypeUtil::RenderPassTypeToString(static_cast<RenderPassType>(p)),
                            v
                        ),
                    g_invalidStateHash
                );

                if (stateHash != g_invalidStateHash) {
                    if (loadedHashes.find(stateHash) == std::cend(loadedHashes)) {
                        if (RenderStateBlock::loadFromXML(
                            stateHash, 
                            Util::StringFormat("%s.RenderStates.%zu",
                                                entryName.c_str(),
                                                stateHash),
                            pt))
                        {
                            //Cache miss
                        }
                        loadedHashes.insert(stateHash);
                    }
                }
            }
        }
    }
}

void Material::saveTextureDataToXML(const stringImpl& entryName, boost::property_tree::ptree& pt) const {
    std::set<size_t> savedSamplers = {};
    for (U8 i = 0; i < g_materialTexturesCount; ++i) {
        const TextureUsage usage = g_materialTextures[i];

        Texture_wptr tex = getTexture(usage);
        if (!tex.expired()) {
            Texture_ptr texture = tex.lock();


            const stringImpl textureNode = entryName + ".texture." + TypeUtil::TextureUsageToString(usage);

            pt.put(textureNode + ".name", texture->assetName());
            pt.put(textureNode + ".path", texture->assetLocation());
            pt.put(textureNode + ".flipped", texture->flipped());

            if (usage == TextureUsage::UNIT1) {
                pt.put(textureNode + ".usage", TypeUtil::TextureOperationToString(_textureOperation));
            }

            const SamplerDescriptor& sampler = texture->getCurrentSampler();
            const size_t hash = sampler.getHash();

            pt.put(textureNode + ".Sampler.hash", hash);

            if (savedSamplers.find(hash) == std::cend(savedSamplers)) {
                XMLParser::saveToXML(sampler, Util::StringFormat("%s.SamplerDescriptors.%zu", entryName.c_str(), hash), pt);
                savedSamplers.insert(hash);
            }
        }
    }
}

void Material::loadTextureDataFromXML(const stringImpl& entryName, const boost::property_tree::ptree& pt) {
    std::atomic_uint loadTasks = 0;

    for (U8 i = 0; i < g_materialTexturesCount; ++i) {
        const TextureUsage usage = g_materialTextures[i];

        SamplerDescriptor sampDesc = {};
        if (auto child = pt.get_child_optional(((entryName + ".texture.") + TypeUtil::TextureUsageToString(usage)) + ".name")) {
            const stringImpl textureNode = entryName + ".texture." + TypeUtil::TextureUsageToString(usage);

            const stringImpl texName = pt.get<stringImpl>(textureNode + ".name", "");
            const stringImpl texPath = pt.get<stringImpl>(textureNode + ".path", "");
            const bool flipped = pt.get(textureNode + ".flipped", false);

            if (!texName.empty()) {
                const size_t hash = pt.get<size_t>(textureNode + ".Sampler.hash", 0);
                sampDesc = XMLParser::loadFromXML(hash, Util::StringFormat("%s.SamplerDescriptors.%zu", entryName.c_str(), hash), pt);

                TextureOperation op = TextureOperation::NONE;
                if (usage == TextureUsage::UNIT1) {
                    _textureOperation = TypeUtil::StringToTextureOperation(pt.get<stringImpl>(textureNode + ".usage", "REPLACE"));
                }
                {
                    UniqueLock<SharedMutex> w_lock(_textureLock);
                    const Texture_ptr& crtTex = _textures[to_base(usage)];
                    if (crtTex != nullptr) {
                        if (crtTex->flipped() == flipped &&
                            crtTex->getCurrentSampler().getHash() == hash &&
                            crtTex->assetName() == texName &&
                            crtTex->assetLocation() == texPath)
                        {
                            continue;
                        }
                    }
                }
                TextureDescriptor texDesc(TextureType::TEXTURE_2D);
                texDesc.samplerDescriptor(sampDesc);

                loadTasks.fetch_add(1u);
                ResourceDescriptor texture(texName);
                texture.assetName(texName);
                texture.assetLocation(texPath);
                texture.propertyDescriptor(texDesc);
                texture.waitForReady(false);
                texture.flag(!flipped);

                Texture_ptr tex =  CreateResource<Texture>(_context.parent().resourceCache(), texture);
                tex->addStateCallback(ResourceState::RES_LOADED, [this, usage, op, &loadTasks, &tex](CachedResource* res) {
                    setTexture(usage, tex, op);
                    loadTasks.fetch_sub(1u);
                });
                
            }
        }

        WAIT_FOR_CONDITION(loadTasks.load() == 0u);
    }
}

};