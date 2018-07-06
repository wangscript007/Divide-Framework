/*
   Copyright (c) 2014 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
   and associated documentation files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _TEXTURE_DESCRIPTOR_H_
#define _TEXTURE_DESCRIPTOR_H_

#include "core.h"
#include "Core/Resources/Headers/ResourceDescriptor.h"
#include "Hardware/Video/Headers/RenderAPIEnums.h"
#include <boost/functional/hash.hpp>

///This class is used to define all of the sampler settings needed to use a texture
///Apply a sampler descriptor to either a texture's ResourceDescriptor or to a TextureDescriptor to use it
///We do not definy copy constructors as we must define descriptors only with POD
class SamplerDescriptor : public PropertyDescriptor {
public:
    ///The constructer specifies the type so it can be used later for downcasting if needed
    SamplerDescriptor() : PropertyDescriptor(DESCRIPTOR_SAMPLER)
    {
        setDefaultValues();
    }

    ///All of these are default values that should be safe for any kind of texture usage
    inline void setDefaultValues() {
        setWrapMode();
        setFilters();
        setAnisotropy(16);
        setLOD();
        toggleMipMaps(true);
        //The following 2 are mainly used by depthmaps for hardware comparisons
        _cmpFunc = CMP_FUNC_LEQUAL;
        _useRefCompare  = false;
        _borderColor.set(DefaultColors::BLACK());
    }

    SamplerDescriptor* clone() const {return New SamplerDescriptor(*this);}

    /*
    *  Sampler states (LOD, wrap modes, anisotropy levels, etc
    */
    inline void setAnisotropy(U8 value = 0) {_anisotropyLevel = value;}

    inline void setLOD(F32 minLOD = -1000.f, F32 maxLOD = 1000.f, F32 biasLOD = 0.f){
        _minLOD = minLOD; _maxLOD = maxLOD; _biasLOD = biasLOD;
    }
    
    inline void setBorderColor(const vec4<F32>& color) {
        _borderColor.set(color);
    }

    inline void setWrapMode(TextureWrap wrapUVW = TEXTURE_REPEAT){
        setWrapModeU(wrapUVW);
        setWrapModeV(wrapUVW);
        setWrapModeW(wrapUVW);
    }

    inline void setWrapMode(TextureWrap wrapU, TextureWrap wrapV, TextureWrap wrapW = TEXTURE_REPEAT){
        setWrapModeU(wrapU);
        setWrapModeV(wrapV);
        setWrapModeW(wrapW);
    }

    inline void setWrapMode(I32 wrapU, I32 wrapV, I32 wrapW){
        setWrapMode(static_cast<TextureWrap>(wrapU), static_cast<TextureWrap>(wrapV), static_cast<TextureWrap>(wrapW));
    }

    inline void setWrapModeU(TextureWrap wrapU) { _wrapU = wrapU; }
    inline void setWrapModeV(TextureWrap wrapV) { _wrapV = wrapV; }
    inline void setWrapModeW(TextureWrap wrapW) { _wrapW = wrapW; }

    inline void setFilters(TextureFilter minFilter = TEXTURE_FILTER_LINEAR, TextureFilter magFilter = TEXTURE_FILTER_LINEAR){
        setMinFilter(minFilter);
        setMagFilter(magFilter);
    }

    inline void setMinFilter(TextureFilter minFilter) {_minFilter = minFilter;}
    inline void setMagFilter(TextureFilter magFilter) {_magFilter = magFilter;}

    inline void toggleMipMaps(bool state) {
        _generateMipMaps = state;
        if(state){
            if(_minFilter == TEXTURE_FILTER_LINEAR)
                _minFilter = TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR;
        }else{
            if(_minFilter == TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR)
                _minFilter = TEXTURE_FILTER_LINEAR;
        }
    }

    inline size_t getHash() const {
        size_t hash = 0;
        boost::hash_combine(hash, _cmpFunc);
        boost::hash_combine(hash, _useRefCompare);
        boost::hash_combine(hash, _wrapU);
        boost::hash_combine(hash, _wrapV);
        boost::hash_combine(hash, _wrapW);
        boost::hash_combine(hash, _minFilter);
        boost::hash_combine(hash, _magFilter);
        boost::hash_combine(hash, _minLOD);
        boost::hash_combine(hash, _maxLOD);
        boost::hash_combine(hash, _biasLOD);
        boost::hash_combine(hash, _anisotropyLevel);
        boost::hash_combine(hash, _generateMipMaps);
        boost::hash_combine(hash, _borderColor.r);
        boost::hash_combine(hash, _borderColor.g);
        boost::hash_combine(hash, _borderColor.b);
        boost::hash_combine(hash, _borderColor.a);
        return hash;
    }
    /*
    *  "Internal" data
    */

    //HW comparison settings
    ComparisonFunction _cmpFunc;           ///<Used by RefCompare
    bool               _useRefCompare;     ///<use red channel as comparison (e.g. for shadows)

    inline TextureWrap   wrapU()            const {return _wrapU;}
    inline TextureWrap   wrapV()            const {return _wrapV;}
    inline TextureWrap   wrapW()            const {return _wrapW;}
    inline TextureFilter minFilter()        const {return _minFilter;}
    inline TextureFilter magFilter()        const {return _magFilter;}
    inline F32           minLOD()           const {return _minLOD;}
    inline F32           maxLOD()           const {return _maxLOD;}
    inline F32           biasLOD()          const {return _biasLOD;}
    inline U8            anisotropyLevel()  const {return _anisotropyLevel;}
    inline bool          generateMipMaps()  const {return _generateMipMaps;}
    inline vec4<F32>     borderColor()      const {return _borderColor;}

protected:
    //Sampler states
    TextureFilter  _minFilter, _magFilter; ///Texture filtering mode
    TextureWrap    _wrapU, _wrapV, _wrapW; ///<Or S-R-T
    bool           _generateMipMaps;       ///<If it's set to true we create automatic MipMaps
    U8             _anisotropyLevel;       ///<The value must be in the range [0...255] and is automatically clamped by the max HW supported level
    F32            _minLOD,_maxLOD;        ///<OpenGL eg: used by TEXTURE_MIN_LOD and TEXTURE_MAX_LOD
    F32            _biasLOD;               ///<OpenGL eg: used by TEXTURE_LOD_BIAS
    vec4<F32>      _borderColor;           ///<Used with CLAMP_TO_BORDER as the background color outside of the texture border
};

///Use to define a texture with details such as type, image formats, etc
///We do not definy copy constructors as we must define descriptors only with POD
class TextureDescriptor : public PropertyDescriptor {
public:
    ///This enum is used when creating Frame Buffers to define the channel that the texture will attach to
    enum AttachmentType{
        Color0 = 0,
        Color1,
        Color2,
        Color3,
        Depth
    };

    TextureDescriptor() : TextureDescriptor(TextureType_PLACEHOLDER, GFXImageFormat_PLACEHOLDER, GDF_PLACEHOLDER)
    {
    }

    TextureDescriptor(TextureType type,
                      GFXImageFormat internalFormat,
                      GFXDataFormat dataType) : PropertyDescriptor(DESCRIPTOR_TEXTURE),
                                                _type(type),
                                                _internalFormat(internalFormat),
                                                _dataType(dataType)
    {
        setDefaultValues();
    }

    TextureDescriptor* clone() const {return New TextureDescriptor(*this);}

    ///Pixel alignment and miplevels are set to match what the HW sets by default
    inline void setDefaultValues(){
        setAlignment();
        setMipLevels();
        _layerCount = 1;
    }

    inline void setAlignment(U8 packAlignment = 1, U8 unpackAlignment = 1) {
        _packAlignment = packAlignment; _unpackAlignment = unpackAlignment;
    }

    inline void setMipLevels(U16 mipMinLevel = 0, U16 mipMaxLevel = 0) {
        _mipMinLevel = mipMinLevel; _mipMaxLevel = mipMaxLevel;
    }
    
    inline void setLayerCount(U8 layerCount){
        _layerCount = layerCount;
    }
    
    inline bool isCubeTexture() const {
        return (_type == TEXTURE_CUBE_MAP || _type == TEXTURE_CUBE_ARRAY);
    }

    ///A TextureDescriptor will always have a sampler, even if it is the default one
    inline       void               setSampler(const SamplerDescriptor& descriptor) {_samplerDescriptor = descriptor;}
    inline const SamplerDescriptor& getSampler()                              const {return _samplerDescriptor;}

    U8  _layerCount;
    U8  _packAlignment, _unpackAlignment; ///<Pixel store information
    U16 _mipMinLevel, _mipMaxLevel;       ///<MipMap interval selection

    ///Texture data information
    GFXImageFormat    _internalFormat;
    GFXDataFormat     _dataType;
    TextureType       _type;
    ///The sampler used to initialize this texture with
    SamplerDescriptor _samplerDescriptor;
};

#endif