#include "Headers/AnimationController.h"
#include "Hardware/Video/Headers/GFXDevice.h"

#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"

namespace Divide {

void AnimEvaluator::Save(std::ofstream& file){
	uint32_t nsize = static_cast<uint32_t>(_name.size());
	file.write(reinterpret_cast<char*>(&nsize), sizeof(uint32_t));// the size of the animation name
	file.write(_name.c_str(), nsize);// the size of the animation name
	file.write(reinterpret_cast<char*>(&_duration), sizeof(_duration));// the duration
	file.write(reinterpret_cast<char*>(&_ticksPerSecond), sizeof(_ticksPerSecond));// the number of ticks per second
	nsize = static_cast<uint32_t>(_channels.size());// number of animation channels,
	file.write(reinterpret_cast<char*>(&nsize), sizeof(uint32_t));// the number animation channels
	for(vectorAlg::vecSize j(0); j< _channels.size(); j++){// for each channel
		nsize =static_cast<uint32_t>(_channels[j]._name.size());
		file.write(reinterpret_cast<char*>(&nsize), sizeof(uint32_t));// the size of the name
		file.write(_channels[j]._name.c_str(), nsize);// the size of the animation name

		nsize =static_cast<uint32_t>(_channels[j]._positionKeys.size());
		file.write(reinterpret_cast<char*>(&nsize), sizeof(uint32_t));// the number of position keys
		for(vectorAlg::vecSize i(0); i< nsize; i++){// for each channel
			file.write(reinterpret_cast<char*>(&_channels[j]._positionKeys[i].mTime), 
                       sizeof(_channels[j]._positionKeys[i].mTime));// pos key
			file.write(reinterpret_cast<char*>(&_channels[j]._positionKeys[i].mValue), 
                       sizeof(_channels[j]._positionKeys[i].mValue));// pos key
		}

		nsize =static_cast<uint32_t>(_channels[j]._rotationKeys.size());
		file.write(reinterpret_cast<char*>(&nsize), sizeof(uint32_t));// the number of position keys
		for(vectorAlg::vecSize i(0); i< nsize; i++){// for each channel
			file.write(reinterpret_cast<char*>(&_channels[j]._rotationKeys[i].mTime), 
                       sizeof(_channels[j]._rotationKeys[i].mTime));// rot key
			file.write(reinterpret_cast<char*>(&_channels[j]._rotationKeys[i].mValue), 
                       sizeof(_channels[j]._rotationKeys[i].mValue));// rot key
		}

		nsize =static_cast<uint32_t>(_channels[j]._scalingKeys.size());
		file.write(reinterpret_cast<char*>(&nsize), sizeof(uint32_t));// the number of position keys
		for(vectorAlg::vecSize i(0); i< nsize; i++){// for each channel
			file.write(reinterpret_cast<char*>(&_channels[j]._scalingKeys[i].mTime), 
                       sizeof(_channels[j]._scalingKeys[i].mTime));// rot key
			file.write(reinterpret_cast<char*>(&_channels[j]._scalingKeys[i].mValue),
                       sizeof(_channels[j]._scalingKeys[i].mValue));// rot key
		}
	}
}

void AnimEvaluator::Load(std::ifstream& file){
	uint32_t nsize = 0;
	file.read(reinterpret_cast<char*>(&nsize), sizeof(uint32_t));// the size of the animation name
	char temp[250];
	file.read(temp, nsize);// the size of the animation name
	temp[nsize]=0;// null char
	_name=temp;
	D_PRINT_FN(Locale::get("CREATE_ANIMATION_BEGIN"),_name.c_str());
	file.read(reinterpret_cast<char*>(&_duration), sizeof(_duration));// the duration
	file.read(reinterpret_cast<char*>(&_ticksPerSecond), sizeof(_ticksPerSecond));// the number of ticks per second
	file.read(reinterpret_cast<char*>(&nsize), sizeof(uint32_t));// the number animation channels
	_channels.resize(nsize);
	for(vectorAlg::vecSize j(0); j< _channels.size(); j++){// for each channel
		nsize =0;
		file.read(reinterpret_cast<char*>(&nsize), sizeof(uint32_t));// the size of the name
		file.read(temp, nsize);// the size of the animation name
		temp[nsize]=0;// null char
		_channels[j]._name=temp;

		nsize =static_cast<uint32_t>(_channels[j]._positionKeys.size());
		file.read(reinterpret_cast<char*>(&nsize), sizeof(uint32_t));// the number of position keys
		_channels[j]._positionKeys.resize(nsize);
		for(vectorAlg::vecSize i(0); i< nsize; i++){// for each channel
			file.read(reinterpret_cast<char*>(&_channels[j]._positionKeys[i].mTime), 
                      sizeof(_channels[j]._positionKeys[i].mTime));// pos key
			file.read(reinterpret_cast<char*>(&_channels[j]._positionKeys[i].mValue), 
                      sizeof(_channels[j]._positionKeys[i].mValue));// pos key
		}

		nsize =static_cast<uint32_t>(_channels[j]._rotationKeys.size());
		file.read(reinterpret_cast<char*>(&nsize), sizeof(uint32_t));// the number of position keys
		_channels[j]._rotationKeys.resize(nsize);
		for(vectorAlg::vecSize i(0); i< nsize; i++){// for each channel
			file.read(reinterpret_cast<char*>(&_channels[j]._rotationKeys[i].mTime), 
                      sizeof(_channels[j]._rotationKeys[i].mTime));// pos key
			file.read(reinterpret_cast<char*>(&_channels[j]._rotationKeys[i].mValue), 
                      sizeof(_channels[j]._rotationKeys[i].mValue));// pos key
		}

		nsize =static_cast<uint32_t>(_channels[j]._scalingKeys.size());
		file.read(reinterpret_cast<char*>(&nsize), sizeof(uint32_t));// the number of position keys
		_channels[j]._scalingKeys.resize(nsize);
		for(vectorAlg::vecSize i(0); i< nsize; i++){// for each channel
			file.read(reinterpret_cast<char*>(&_channels[j]._scalingKeys[i].mTime), 
                      sizeof(_channels[j]._scalingKeys[i].mTime));// pos key
			file.read(reinterpret_cast<char*>(&_channels[j]._scalingKeys[i].mValue),
                      sizeof(_channels[j]._scalingKeys[i].mValue));// pos key
		}
	}
	_lastPositions.resize( _channels.size(), vec3<U32>());
}

void SceneAnimator::Save(std::ofstream& file){
	// first recursively save the skeleton
	if(_skeleton)
		SaveSkeleton(file, _skeleton);

	uint32_t nsize = static_cast<uint32_t>(_animations.size());
	file.write(reinterpret_cast<char*>(&nsize), sizeof(uint32_t));// the number of animations
	for(uint32_t i(0); i< nsize; i++){
		_animations[i].Save(file);
	}

	nsize = static_cast<uint32_t>(_bones.size());
	file.write(reinterpret_cast<char*>(&nsize), sizeof(uint32_t));// the number of bones

	for(uint32_t i(0); i< _bones.size(); i++){
		nsize = static_cast<uint32_t>(_bones[i]->_name.size());
		file.write(reinterpret_cast<char*>(&nsize), sizeof(uint32_t));// the size of the bone name
		file.write(_bones[i]->_name.c_str(), nsize);// the name of the bone
	}
}

void SceneAnimator::Load(std::ifstream& file){
	Release();// make sure to clear this before writing new data
	_skeleton = LoadSkeleton(file, nullptr);
	uint32_t nsize = 0;
	file.read(reinterpret_cast<char*>(&nsize), sizeof(uint32_t));// the number of animations
	_animations.resize(nsize);
	D_PRINT_FN(Locale::get("LOAD_ANIMATIONS_BEGIN"));
	for(uint32_t i(0); i< nsize; i++){
		_animations[i].Load(file);
	}
    // get all the animation names so I can reference them by name and get the correct id
	for(uint32_t i(0); i< _animations.size(); i++){
		_animationNameToId.insert(hashMapImpl<stringImpl, uint32_t>::value_type(_animations[i]._name, i));
	}

	char bname[250];
	file.read(reinterpret_cast<char*>(&nsize), sizeof(uint32_t));// the number of bones
	_bones.resize(nsize);

	for(uint32_t i(0); i< _bones.size(); i++){
		file.read(reinterpret_cast<char*>(&nsize), sizeof(uint32_t));// the size of the bone name
		file.read(bname, nsize);// the size of the bone name
		bname[nsize]=0;
		hashMapImpl<stringImpl, Bone*>::iterator found = _bonesByName.find(bname);
		Bone* tep = found->second;
		_bonesToIndex[found->first] = i;
		_bones[i]=tep;
	}

	_transforms.resize( _bones.size());
	F32 timestep = 1.0f/(F32)ANIMATION_TICKS_PER_SECOND;// 25.0f per second
	for(vectorAlg::vecSize i(0); i< _animations.size(); i++){// pre calculate the animations
		F32 dt = 0;
		mat4<F32> rotationmat;
		for(F32 ticks = 0; ticks < _animations[i]._duration; ticks += _animations[i]._ticksPerSecond/ANIMATION_TICKS_PER_SECOND){
			dt +=timestep;
            Calculate((I32)i, dt);
			_animations[i]._transforms.push_back(vectorImpl<mat4<F32> >());
			vectorImpl<mat4<F32> >& trans = _animations[i]._transforms.back();
			for( vectorAlg::vecSize a = 0; a < _transforms.size(); ++a){
				AnimUtils::TransformMatrix(aiMatrix4x4(_bones[a]->_globalTransform *  _bones[a]->_offsetMatrix), rotationmat);
				trans.push_back(rotationmat);
			}
		}
	}
	D_PRINT_FN(Locale::get("LOAD_ANIMATIONS_END"), _bones.size());
}

void SceneAnimator::SaveSkeleton(std::ofstream& file, Bone* parent) {
	uint32_t nsize = static_cast<uint32_t>(parent->_name.size());
    // the number of chars
	file.write(reinterpret_cast<char*>(&nsize), sizeof(uint32_t));
    // the name of the bone
	file.write(parent->_name.c_str(), nsize);
    // the bone offsets
	file.write(reinterpret_cast<char*>(&parent->_offsetMatrix), sizeof(parent->_offsetMatrix));
    // original bind pose
	file.write(reinterpret_cast<char*>(&parent->_originalLocalTransform), sizeof(parent->_originalLocalTransform));
    // number of children
	nsize = static_cast<uint32_t>(parent->_children.size());
    // the number of children
	file.write(reinterpret_cast<char*>(&nsize), sizeof(uint32_t));
    // continue for all children
    for (vectorImpl<Bone*>::iterator it = parent->_children.begin(); it != parent->_children.end(); ++it) {
		SaveSkeleton(file, *it);
    }
}

Bone* SceneAnimator::LoadSkeleton(std::ifstream& file, Bone* parent){
    Bone* internalNode = MemoryManager_NEW Bone();// create a node
	internalNode->_parent = parent; //set the parent, in the case this is theroot node, it will be null
	uint32_t nsize=0;
	file.read(reinterpret_cast<char*>(&nsize), sizeof(uint32_t));// the number of chars
	char temp[250];
	file.read(temp, nsize);// the name of the bone
	temp[nsize]=0;
	internalNode->_name = temp;
	_bonesByName[internalNode->_name] = internalNode;// use the name as a key
    // the bone offsets
	file.read(reinterpret_cast<char*>(&internalNode->_offsetMatrix), sizeof(internalNode->_offsetMatrix));
    // original bind pose
	file.read(reinterpret_cast<char*>(&internalNode->_originalLocalTransform), sizeof(internalNode->_originalLocalTransform));
    // a copy saved
	internalNode->_localTransform = internalNode->_originalLocalTransform;
	CalculateBoneToWorldTransform(internalNode);
    // the number of children
	file.read(reinterpret_cast<char*>(&nsize), sizeof(uint32_t));

    // recursivly call this function on all children
	// continue for all child nodes and assign the created internal nodes as our children
	for( U32 a = 0; a < nsize && file; a++) {
		internalNode->_children.push_back(LoadSkeleton(file, internalNode));
	}
	return internalNode;
}

};