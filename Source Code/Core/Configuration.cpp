#include "stdafx.h"

#include "Headers/Configuration.h"
#include "Utility/Headers/Localization.h"

namespace Divide {

bool Configuration::fromXML(const char* xmlFile) {
    Console::printfn(Locale::get(_ID("XML_LOAD_CONFIG")), xmlFile);
    if (LoadSave.read(xmlFile)) {
        GET_PARAM(debug.enableRenderAPIDebugging);
        GET_PARAM(debug.useGeometryCache);
        GET_PARAM(debug.useVegetationCache);
        GET_PARAM(debug.enableTreeInstances);
        GET_PARAM(debug.enableGrassInstances);
        GET_PARAM(debug.useShaderTextCache);
        GET_PARAM(debug.useShaderBinaryCache);
        GET_PARAM(debug.memFile);
        GET_PARAM(language);
        GET_PARAM(runtime.targetDisplay);
        GET_PARAM(runtime.targetRenderingAPI);
        GET_PARAM(runtime.maxWorkerThreads);
        GET_PARAM(runtime.windowedMode);
        GET_PARAM(runtime.windowResizable);
        GET_PARAM(runtime.maximizeOnStart);
        GET_PARAM(runtime.frameRateLimit);
        GET_PARAM(runtime.enableVSync);
        GET_PARAM(runtime.adaptiveSync);
        GET_PARAM_ATTRIB(runtime.splashScreenSize, width);
        GET_PARAM_ATTRIB(runtime.splashScreenSize, height);
        GET_PARAM_ATTRIB(runtime.windowSize, width);
        GET_PARAM_ATTRIB(runtime.windowSize, height);
        GET_PARAM_ATTRIB(runtime.resolution, width);
        GET_PARAM_ATTRIB(runtime.resolution, height);
        GET_PARAM(runtime.cameraViewDistance);
        GET_PARAM(runtime.verticalFOV);
        GET_PARAM(gui.cegui.enabled);
        GET_PARAM(gui.cegui.defaultGUIScheme);
        GET_PARAM(gui.imgui.dontMergeFloatingWindows);
        GET_PARAM(gui.consoleLayoutFile);
        GET_PARAM(rendering.MSAASamples);
        GET_PARAM(rendering.anisotropicFilteringLevel);
        GET_PARAM(rendering.reflectionResolutionFactor);
        GET_PARAM(rendering.terrainDetailLevel);
        GET_PARAM(rendering.terrainTextureQuality);
        GET_PARAM(rendering.numLightsPerScreenTile);
        GET_PARAM(rendering.lightThreadGroupSize);
        GET_PARAM(rendering.enableFog);
        GET_PARAM(rendering.fogDensity);
        GET_PARAM_ATTRIB(rendering.fogColour, r);
        GET_PARAM_ATTRIB(rendering.fogColour, g);
        GET_PARAM_ATTRIB(rendering.fogColour, b);
        GET_PARAM_ATTRIB(rendering.lodThresholds, x);
        GET_PARAM_ATTRIB(rendering.lodThresholds, y);
        GET_PARAM_ATTRIB(rendering.lodThresholds, z);
        GET_PARAM_ATTRIB(rendering.lodThresholds, w);
        GET_PARAM(rendering.postFX.postAAType);
        GET_PARAM(rendering.postFX.PostAAQualityLevel);
        GET_PARAM(rendering.postFX.enableAdaptiveToneMapping);
        GET_PARAM(rendering.postFX.enableDepthOfField);
        GET_PARAM(rendering.postFX.enablePerObjectMotionBlur);
        GET_PARAM(rendering.postFX.enableCameraBlur);
        GET_PARAM(rendering.postFX.enableBloom);
        GET_PARAM(rendering.postFX.bloomFactor);
        GET_PARAM(rendering.postFX.bloomThreshold);
        GET_PARAM(rendering.postFX.velocityScale);
        GET_PARAM(rendering.postFX.enableSSAO);
        GET_PARAM(rendering.postFX.ssaoRadius);
        GET_PARAM(rendering.postFX.ssaoKernelSizeIndex);
        GET_PARAM(rendering.postFX.ssaoPower);
        GET_PARAM(rendering.shadowMapping.enabled);
        GET_PARAM(rendering.shadowMapping.softness);
        GET_PARAM(rendering.shadowMapping.csm.shadowMapResolution);
        GET_PARAM(rendering.shadowMapping.csm.MSAASamples);
        GET_PARAM(rendering.shadowMapping.csm.enableBlurring);
        GET_PARAM(rendering.shadowMapping.csm.splitLambda);
        GET_PARAM(rendering.shadowMapping.csm.splitCount);
        GET_PARAM(rendering.shadowMapping.csm.anisotropicFilteringLevel);
        GET_PARAM(rendering.shadowMapping.spot.shadowMapResolution);
        GET_PARAM(rendering.shadowMapping.spot.MSAASamples);
        GET_PARAM(rendering.shadowMapping.spot.enableBlurring);
        GET_PARAM(rendering.shadowMapping.spot.anisotropicFilteringLevel);
        GET_PARAM(rendering.shadowMapping.point.shadowMapResolution);
        GET_PARAM(rendering.shadowMapping.point.MSAASamples);
        GET_PARAM(rendering.shadowMapping.point.enableBlurring);
        GET_PARAM(rendering.shadowMapping.point.anisotropicFilteringLevel);

        GET_PARAM(title);
        GET_PARAM(defaultTextureLocation);
        GET_PARAM(defaultShadersLocation);

        return true;
    }

    return false;
}

bool Configuration::toXML(const char* xmlFile) const {
    if (LoadSave.prepareSaveFile(xmlFile)) {
        PUT_PARAM(debug.enableRenderAPIDebugging);
        PUT_PARAM(debug.useGeometryCache);
        PUT_PARAM(debug.useVegetationCache);
        PUT_PARAM(debug.enableTreeInstances);
        PUT_PARAM(debug.enableGrassInstances);
        PUT_PARAM(debug.useShaderTextCache);
        PUT_PARAM(debug.useShaderBinaryCache);
        PUT_PARAM(debug.memFile);
        PUT_PARAM(language);
        PUT_PARAM(runtime.targetDisplay);
        PUT_PARAM(runtime.targetRenderingAPI);
        PUT_PARAM(runtime.maxWorkerThreads);
        PUT_PARAM(runtime.windowedMode);
        PUT_PARAM(runtime.windowResizable);
        PUT_PARAM(runtime.maximizeOnStart);
        PUT_PARAM(runtime.frameRateLimit);
        PUT_PARAM(runtime.enableVSync);
        PUT_PARAM(runtime.adaptiveSync);
        PUT_PARAM_ATTRIB(runtime.splashScreenSize, width);
        PUT_PARAM_ATTRIB(runtime.splashScreenSize, height);
        PUT_PARAM_ATTRIB(runtime.windowSize, width);
        PUT_PARAM_ATTRIB(runtime.windowSize, height);
        PUT_PARAM_ATTRIB(runtime.resolution, width);
        PUT_PARAM_ATTRIB(runtime.resolution, height);
        PUT_PARAM(runtime.cameraViewDistance);
        PUT_PARAM(runtime.verticalFOV);
        PUT_PARAM(gui.cegui.enabled);
        PUT_PARAM(gui.cegui.defaultGUIScheme);
        PUT_PARAM(gui.imgui.dontMergeFloatingWindows);
        PUT_PARAM(gui.consoleLayoutFile);
        PUT_PARAM(rendering.MSAASamples);
        PUT_PARAM(rendering.anisotropicFilteringLevel);
        PUT_PARAM(rendering.reflectionResolutionFactor);
        PUT_PARAM(rendering.terrainDetailLevel);
        PUT_PARAM(rendering.terrainTextureQuality);
        PUT_PARAM(rendering.numLightsPerScreenTile);
        PUT_PARAM(rendering.lightThreadGroupSize);
        PUT_PARAM(rendering.enableFog);
        PUT_PARAM(rendering.fogDensity);
        PUT_PARAM_ATTRIB(rendering.fogColour, r);
        PUT_PARAM_ATTRIB(rendering.fogColour, g);
        PUT_PARAM_ATTRIB(rendering.fogColour, b);
        PUT_PARAM_ATTRIB(rendering.lodThresholds, x);
        PUT_PARAM_ATTRIB(rendering.lodThresholds, y);
        PUT_PARAM_ATTRIB(rendering.lodThresholds, z);
        PUT_PARAM_ATTRIB(rendering.lodThresholds, w);
        PUT_PARAM(rendering.postFX.postAAType);
        PUT_PARAM(rendering.postFX.PostAAQualityLevel);
        PUT_PARAM(rendering.postFX.enableAdaptiveToneMapping);
        PUT_PARAM(rendering.postFX.enableDepthOfField);
        PUT_PARAM(rendering.postFX.enablePerObjectMotionBlur);
        PUT_PARAM(rendering.postFX.enableCameraBlur);
        PUT_PARAM(rendering.postFX.enableBloom);
        PUT_PARAM(rendering.postFX.bloomThreshold);
        PUT_PARAM(rendering.postFX.bloomFactor);
        PUT_PARAM(rendering.postFX.velocityScale);
        PUT_PARAM(rendering.postFX.enableSSAO);
        PUT_PARAM(rendering.postFX.ssaoRadius);
        PUT_PARAM(rendering.postFX.ssaoPower);
        PUT_PARAM(rendering.postFX.ssaoKernelSizeIndex);
        PUT_PARAM(rendering.shadowMapping.enabled);
        PUT_PARAM(rendering.shadowMapping.softness);
        PUT_PARAM(rendering.shadowMapping.csm.shadowMapResolution);
        PUT_PARAM(rendering.shadowMapping.csm.MSAASamples);
        PUT_PARAM(rendering.shadowMapping.csm.enableBlurring);
        PUT_PARAM(rendering.shadowMapping.csm.splitLambda);
        PUT_PARAM(rendering.shadowMapping.csm.splitCount);
        PUT_PARAM(rendering.shadowMapping.csm.anisotropicFilteringLevel);
        PUT_PARAM(rendering.shadowMapping.spot.shadowMapResolution);
        PUT_PARAM(rendering.shadowMapping.spot.MSAASamples);
        PUT_PARAM(rendering.shadowMapping.spot.enableBlurring);
        PUT_PARAM(rendering.shadowMapping.spot.anisotropicFilteringLevel);
        PUT_PARAM(rendering.shadowMapping.point.shadowMapResolution);
        PUT_PARAM(rendering.shadowMapping.point.MSAASamples);
        PUT_PARAM(rendering.shadowMapping.point.enableBlurring);
        PUT_PARAM(rendering.shadowMapping.point.anisotropicFilteringLevel);

        PUT_PARAM(title);
        PUT_PARAM(defaultTextureLocation);
        PUT_PARAM(defaultShadersLocation);
        LoadSave.write();
        return true;
    }

    return false;
}

void Configuration::save() {
    if (changed() && toXML(LoadSave._loadPath.c_str())) {
        changed(false);
    }
}
}; //namespace Divide
