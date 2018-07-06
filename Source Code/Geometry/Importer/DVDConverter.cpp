#include "Headers/DVDConverter.h"

#include <aiScene.h> 
#include <assimp.hpp>      // C++ importer interface
#include <aiPostProcess.h> // Post processing flags
#include "Managers/Headers/ResourceManager.h"
#include "Utility/Headers/XMLParser.h"
#include "Geometry/Animations/Headers/cAnimationController.h"
#include "Core/Headers/ParamHandler.h"

using namespace std;

Mesh* DVDConverter::load(const string& file){	
	Mesh* tempMesh = New Mesh();
	_fileLocation = file;
	_modelName = _fileLocation.substr( _fileLocation.find_last_of( '/' ) + 1 );
	tempMesh->setName(_modelName);
	tempMesh->setResourceLocation(_fileLocation);
	_ppsteps = aiProcess_CalcTangentSpace         | // calculate tangents and bitangents if possible
			   aiProcess_JoinIdenticalVertices    | // join identical vertices/ optimize indexing
			   aiProcess_ValidateDataStructure    | // perform a full validation of the loader's output
			   aiProcess_ImproveCacheLocality     | // improve the cache locality of the output vertices
			   aiProcess_RemoveRedundantMaterials | // remove redundant materials
			   aiProcess_FindDegenerates          | // remove degenerated polygons from the import
			   aiProcess_FindInvalidData          | // detect invalid model data, such as invalid normal vectors
			   aiProcess_GenUVCoords              | // convert spherical, cylindrical, box and planar mapping to proper UVs
			   aiProcess_TransformUVCoords        | // preprocess UV transformations (scaling, translation ...)
			   aiProcess_FindInstances            | // search for instanced meshes and remove them by references to one master
			   aiProcess_LimitBoneWeights         | // limit bone weights to 4 per vertex
			   aiProcess_OptimizeMeshes	          | // join small meshes, if possible;
			   aiProcess_OptimizeGraph            | // Nodes with no animations, bones, lights or cameras assigned are collapsed and joined.
			   0;
	bool removeLinesAndPoints = true;
	Assimp::Importer importer;
	importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE , removeLinesAndPoints ? aiPrimitiveType_LINE | aiPrimitiveType_POINT : 0 );

	if(GFX_DEVICE.getApi() > 6)
		_ppsteps |= aiProcess_ConvertToLeftHanded;

	scene = importer.ReadFile( file, _ppsteps                    |
								aiProcess_Triangulate            |
								aiProcess_SplitLargeMeshes	     |
								aiProcess_SortByPType            |
								aiProcess_GenSmoothNormals);

	if( !scene){
		ERROR_FN("DVDFile::load( %s ): %s", file.c_str(), importer.GetErrorString());
		return false;
	}

	for(U16 n = 0; n < scene->mNumMeshes; n++){
		
		//Skip points and lines ... for now -Ionut
		//ToDo: Fix this -Ionut
		if(scene->mMeshes[n]->mPrimitiveTypes == aiPrimitiveType_LINE || 
			scene->mMeshes[n]->mPrimitiveTypes == aiPrimitiveType_POINT )
				continue;
		if(scene->mMeshes[n]->mNumVertices == 0) continue; 
		SubMesh* s = loadSubMeshGeometry(scene->mMeshes[n], n);
		if(s){
			if(s->getRefCount() == 1){
				Material* m = loadSubMeshMaterial(scene->mMaterials[scene->mMeshes[n]->mMaterialIndex],string(s->getName()+ "_material"));
				s->setMaterial(m);
			}//else the Resource manager created a copy of the material
			s->getGeometryVBO()->Create();
			tempMesh->addSubMesh(s->getName());
		}
			
	}
	
	tempMesh->setDrawState(true);
	return tempMesh;
}

SubMesh* DVDConverter::loadSubMeshGeometry(aiMesh* source,U8 count){
	string temp;
	char a;
	for(U16 j = 0; j < source->mName.length; j++){
		a = source->mName.data[j];
		temp += a;
	}
	
	if(temp.empty()){
		char number[3];
		sprintf(number,"%d",count);
		temp = _fileLocation.substr(_fileLocation.rfind("/")+1,_fileLocation.length())+"-submesh-"+number;
	}

	//Submesh is created as a resource when added to the scenegraph
	ResourceDescriptor submeshdesc(temp);
	submeshdesc.setFlag(true);
	SubMesh* tempSubMesh = CreateResource<SubMesh>(submeshdesc);

	if(!tempSubMesh->getGeometryVBO()->getPosition().empty()){
		return tempSubMesh;
	}
	temp.clear();

	tempSubMesh->getGeometryVBO()->getPosition().reserve(source->mNumVertices);
	tempSubMesh->getGeometryVBO()->getNormal().reserve(source->mNumVertices);
	tempSubMesh->getGeometryVBO()->getTangent().reserve(source->mNumVertices);
	tempSubMesh->getGeometryVBO()->getBiTangent().reserve(source->mNumVertices);
	std::vector< std::vector<vertexWeight> >   weightsPerVertex(source->mNumVertices);
/*
	if(source->HasBones()){
	
		for(U16 b = 0; b < source->mNumBones; b++){
			const aiBone* bone = source->mBones[b];
			for( U16 bi = 0; bi < bone->mNumWeights; bi++)
				weightsPerVertex[bone->mWeights[bi].mVertexId].push_back(vertexWeight( b, bone->mWeights[bi].mWeight));
		}
	}
*/	bool processTangents = true;
	if(!source->mTangents){
        processTangents = false;
		PRINT_FN("DVDConverter: SubMesh [ %s ] does not have tangent & biTangent data!", tempSubMesh->getName().c_str());
	}

	for(U32 j = 0; j < source->mNumVertices; j++){
			
		tempSubMesh->getGeometryVBO()->getPosition().push_back(vec3(source->mVertices[j].x,
															        source->mVertices[j].y,
																	source->mVertices[j].z));
		tempSubMesh->getGeometryVBO()->getNormal().push_back(vec3(source->mNormals[j].x,
																  source->mNormals[j].y,
																  source->mNormals[j].z));
		if(processTangents){
			tempSubMesh->getGeometryVBO()->getTangent().push_back(vec3(source->mTangents[j].x,
																	   source->mTangents[j].y,
																	   source->mTangents[j].z));
			tempSubMesh->getGeometryVBO()->getBiTangent().push_back(vec3(source->mBitangents[j].x,
																	     source->mBitangents[j].y,
																		 source->mBitangents[j].z));
		}
/*		vector<U8> boneIndices, boneWeights;
		boneIndices.push_back(0);boneWeights.push_back(0);
		boneIndices.push_back(0);boneWeights.push_back(0);
		boneIndices.push_back(0);boneWeights.push_back(0);
		boneIndices.push_back(0);boneWeights.push_back(0);

		if( source->HasBones())	{
			ai_assert( weightsPerVertex[x].size() <= 4);
			for( U8 a = 0; a < weightsPerVertex[j].size(); a++){
				boneIndices.push_back(weightsPerVertex[j][a]._vertexId);
				boneWeights.push_back((U8) (weightsPerVertex[j][a]._weight * 255.0f));
			}
		}

		tempSubMesh->getGeometryVBO()->getBoneIndices().push_back(boneIndices);
		tempSubMesh->getGeometryVBO()->getBoneWeights().push_back(boneWeights);
*/
	}//endfor

	if(source->mTextureCoords[0] != NULL){
		tempSubMesh->getGeometryVBO()->getTexcoord().reserve(source->mNumVertices);
		for(U32 j = 0; j < source->mNumVertices; j++){
			tempSubMesh->getGeometryVBO()->getTexcoord().push_back(vec2(source->mTextureCoords[0][j].x,
																		source->mTextureCoords[0][j].y));
		}//endfor
	}//endif

	for(U32 k = 0; k < source->mNumFaces; k++)
			for(U32 m = 0; m < source->mFaces[k].mNumIndices; m++)
				tempSubMesh->getIndices().push_back(source->mFaces[k].mIndices[m]);

	return tempSubMesh;
		 
}

/// Load the material for the current SubMesh
Material* DVDConverter::loadSubMeshMaterial(aiMaterial* source, const string& materialName){

	/// See if the material already exists in a cooked state (XML file)
	Material* tempMaterial = XML::loadMaterial(materialName);
	if(tempMaterial) return tempMaterial;
	/// If it's not defined in an XML File, see if it was previously loaded by the Resource Manager
	bool skip = false;
	ResourceDescriptor tempMaterialDescriptor(materialName);
	if(FindResource(materialName)) skip = true;
	/// If we found it in the Resource Manager, return a copy of it
	tempMaterial = CreateResource<Material>(tempMaterialDescriptor);
	if(skip) return tempMaterial;

	ParamHandler& par = ParamHandler::getInstance();

	/// Compare load results with the standard succes value
	aiReturn result = AI_SUCCESS; 
	
	/// default diffuse color
	vec4 tempColorVec4(0.8f, 0.8f, 0.8f,1.0f);

	/// Load diffuse color
	aiColor4D diffuse;
	if(AI_SUCCESS == aiGetMaterialColor(source,AI_MATKEY_COLOR_DIFFUSE, &diffuse)){
		tempColorVec4.r = diffuse.r;
		tempColorVec4.g = diffuse.g;
		tempColorVec4.b = diffuse.b;
		tempColorVec4.a = diffuse.a;
	}else{
		D_PRINT_FN("Material [ %s ] does not have a diffuse color", materialName.c_str());
	}
	tempMaterial->setDiffuse(tempColorVec4);
		
	/// default ambient color
	tempColorVec4.set(0.2f, 0.2f, 0.2f, 1.0f);

	/// Load ambient color
	aiColor4D ambient;
	if(AI_SUCCESS == aiGetMaterialColor(source,AI_MATKEY_COLOR_AMBIENT,&ambient)){
		tempColorVec4.r = ambient.r;
		tempColorVec4.g = ambient.g;
		tempColorVec4.b = ambient.b;
		tempColorVec4.a = ambient.a;
	}else{
		D_PRINT_FN("Material [ %s ] does not have an ambient color", materialName.c_str());
	}
	tempMaterial->setAmbient(tempColorVec4);
		
	/// default specular color
	tempColorVec4.set(1.0f,1.0f,1.0f,1.0f);

	// Load specular color
	aiColor4D specular;
	if(AI_SUCCESS == aiGetMaterialColor(source,AI_MATKEY_COLOR_SPECULAR,&specular)){
		tempColorVec4.r = specular.r;
		tempColorVec4.g = specular.g;
		tempColorVec4.b = specular.b;
		tempColorVec4.a = specular.a;
	}else{
		D_PRINT_FN("Material [ %s ] does not have a specular color", materialName.c_str());
	}
	tempMaterial->setSpecular(tempColorVec4);

	/// default emissive color
	tempColorVec4.set(0.0f, 0.0f, 0.0f, 1.0f);

	/// Load emissive color
	aiColor4D emissive;
	if(AI_SUCCESS == aiGetMaterialColor(source,AI_MATKEY_COLOR_EMISSIVE,&emissive)){
		tempColorVec4.r = emissive.r;
		tempColorVec4.g = emissive.g;
		tempColorVec4.b = emissive.b;
		tempColorVec4.b = emissive.a;
	}
	tempMaterial->setEmissive(tempColorVec4);

	/// default opacity value
	F32 opacity = 1.0f;
	/// Load material opacity value
	aiGetMaterialFloat(source,AI_MATKEY_OPACITY,&opacity);
	tempMaterial->setOpacityValue(opacity);


	/// default shading model
	I32 shadingModel = Material::SHADING_PHONG;
	/// Load shading model
	aiGetMaterialInteger(source,AI_MATKEY_SHADING_MODEL,&shadingModel);
	tempMaterial->setShadingMode(shadingModeInternal(shadingModel));

	/// Default shininess values
	F32 shininess = 15,strength = 1;
	/// Load shininess
	aiGetMaterialFloat(source,AI_MATKEY_SHININESS, &shininess);
	/// Load shininess strength
	aiGetMaterialFloat( source, AI_MATKEY_SHININESS_STRENGTH, &strength);
	tempMaterial->setShininess(shininess * strength);

	/// check if material is two sided
	I32 two_sided = 0;
	aiGetMaterialInteger(source, AI_MATKEY_TWOSIDED, &two_sided);
	tempMaterial->setDoubleSided(two_sided != 0);

	aiString tName; 
	aiTextureMapping mapping; 
	U32 uvInd;
	F32 blend; 
	aiTextureOp op;
	aiTextureMapMode mode[3];

	U8 count = 0;
	/// While we still have diffuse textures
	while(result == AI_SUCCESS){
		/// Load each one
		result = source->GetTexture(aiTextureType_DIFFUSE, count, 
									&tName, 
									&mapping, 
									&uvInd,
									&blend,
									&op, 
									mode);
		if(result != AI_SUCCESS) break;
		/// get full path
		string path = tName.data;
		/// get only image name
		string img_name = path.substr( path.find_last_of( '/' ) + 1 );
		/// try to find a path name
		string pathName = _fileLocation.substr( 0, _fileLocation.rfind("/")+1 );
		/// look in default texture folder
		path = par.getParam<string>("assetsLocation")+"/"+par.getParam<string>("defaultTextureLocation") +"/"+ path;
		/// if we have a name and an extension
		if(!img_name.substr(img_name.find_first_of(".")).empty()){
			/// Is this the base texture or the secondary?
			Material::TextureUsage item;
			if(count == 0) item = Material::TEXTURE_BASE;
			else if(count == 1) item = Material::TEXTURE_SECOND;
			/// Load the texture resource
			ResourceDescriptor texture(img_name);
			texture.setResourceLocation(path);
			texture.setFlag(true);
			Texture2D* textureRes = CreateResource<Texture2D>(texture);
			tempMaterial->setTexture(item,textureRes,aiTextureOpToTextureOperation(op));
			textureRes->setTextureWrap(aiTextureMapModeToInternal(mode[0]),
									   aiTextureMapModeToInternal(mode[1]),
									   aiTextureMapModeToInternal(mode[2])); 
		}//endif

		tName.Clear();
		count++;
		if(count == 2) break; //ToDo: Only 2 texture for now. Fix This! -Ionut;
	}//endwhile

	result = source->GetTexture(aiTextureType_NORMALS, 0, &tName, &mapping, &uvInd, &blend, &op, mode);
	if(result == AI_SUCCESS){
		string path = tName.data;
		string img_name = path.substr( path.find_last_of( '/' ) + 1 );
		path = par.getParam<string>("assetsLocation")+"/"+par.getParam<string>("defaultTextureLocation") +"/"+ path;

		string pathName = _fileLocation.substr( 0, _fileLocation.rfind("/")+1 );
		if(img_name.rfind('.') !=  string::npos){

			ResourceDescriptor texture(img_name);
			texture.setResourceLocation(path);
			texture.setFlag(true);
			Texture2D* textureRes = CreateResource<Texture2D>(texture);
			tempMaterial->setTexture(Material::TEXTURE_NORMALMAP,textureRes,aiTextureOpToTextureOperation(op));
			textureRes->setTextureWrap(aiTextureMapModeToInternal(mode[0]),
									   aiTextureMapModeToInternal(mode[1]),
									   aiTextureMapModeToInternal(mode[2])); 
		}//endif
	}//endif
	result = source->GetTexture(aiTextureType_HEIGHT, 0, &tName, &mapping, &uvInd, &blend, &op, mode);
	if(result == AI_SUCCESS){
		string path = tName.data;
		string img_name = path.substr( path.find_last_of( '/' ) + 1 );
		path = par.getParam<string>("assetsLocation")+"/"+par.getParam<string>("defaultTextureLocation") +"/"+ path;
		string pathName = _fileLocation.substr( 0, _fileLocation.rfind("/")+1 );
		if(img_name.rfind('.') !=  string::npos){

			ResourceDescriptor texture(img_name);
			texture.setResourceLocation(path);
			texture.setFlag(true);
			Texture2D* textureRes = CreateResource<Texture2D>(texture);
			tempMaterial->setTexture(Material::TEXTURE_BUMP,textureRes,aiTextureOpToTextureOperation(op));
			textureRes->setTextureWrap(aiTextureMapModeToInternal(mode[0]),
									   aiTextureMapModeToInternal(mode[1]),
									   aiTextureMapModeToInternal(mode[2])); 
		}//endif
	}//endif
	result = source->GetTexture(aiTextureType_OPACITY, 0, &tName, &mapping, &uvInd, &blend, &op, mode);
	if(result == AI_SUCCESS){
		string path = tName.data;
		string img_name = path.substr( path.find_last_of( '/' ) + 1 );
		string pathName = _fileLocation.substr( 0, _fileLocation.rfind("/")+1 );

		path = par.getParam<string>("assetsLocation")+"/"+par.getParam<string>("defaultTextureLocation") +"/"+ path;
		
		if(img_name.rfind('.') !=  string::npos){
			ResourceDescriptor texture(img_name);
			texture.setResourceLocation(path);
			texture.setFlag(true);
			Texture2D* textureRes = CreateResource<Texture2D>(texture);
			tempMaterial->setTexture(Material::TEXTURE_OPACITY,textureRes,aiTextureOpToTextureOperation(op));
			tempMaterial->setDoubleSided(true);
			textureRes->setTextureWrap(aiTextureMapModeToInternal(mode[0]),
									   aiTextureMapModeToInternal(mode[1]),
									   aiTextureMapModeToInternal(mode[2])); 
		}//endif
	}else{
		I32 flags = 0;
		aiGetMaterialInteger(source,AI_MATKEY_TEXFLAGS_DIFFUSE(0),&flags);

		// try to find out whether the diffuse texture has any
		// non-opaque pixels. If we find a few, use it as opacity texture
		if (tempMaterial->getTexture(Material::TEXTURE_BASE)){
			if(!(flags & aiTextureFlags_IgnoreAlpha) && 
				tempMaterial->getTexture(Material::TEXTURE_BASE)->hasTransparency()){
					Texture2D* textureRes = CreateResource<Texture2D>(ResourceDescriptor(tempMaterial->getTexture(Material::TEXTURE_BASE)->getName()));
					tempMaterial->setTexture(Material::TEXTURE_OPACITY,textureRes);
			}
		}
	}

	result = source->GetTexture(aiTextureType_SPECULAR, 0, &tName, &mapping, &uvInd, &blend, &op, mode);
	if(result == AI_SUCCESS){
		string path = tName.data;
		string img_name = path.substr( path.find_last_of( '/' ) + 1 );
		string pathName = _fileLocation.substr( 0, _fileLocation.rfind("/")+1 );

		path = par.getParam<string>("assetsLocation")+"/"+par.getParam<string>("defaultTextureLocation") +"/"+ path;

		if(img_name.rfind('.') !=  string::npos){
			ResourceDescriptor texture(img_name);
			texture.setResourceLocation(path);
			texture.setFlag(true);
			Texture2D* textureRes = CreateResource<Texture2D>(texture);
			tempMaterial->setTexture(Material::TEXTURE_SPECULAR,textureRes,aiTextureOpToTextureOperation(op));
			textureRes->setTextureWrap(aiTextureMapModeToInternal(mode[0]),
									   aiTextureMapModeToInternal(mode[1]),
									   aiTextureMapModeToInternal(mode[2])); 
		}//endif
	}//endif

	return tempMaterial;
}

Material::TextureOperation DVDConverter::aiTextureOpToTextureOperation(aiTextureOp op){
	switch(op){
		case aiTextureOp_Multiply:
			return Material::TextureOperation_Multiply;
		case aiTextureOp_Add:
			return Material::TextureOperation_Add;
		case aiTextureOp_Subtract:
			return Material::TextureOperation_Subtract;
		case aiTextureOp_Divide:
			return Material::TextureOperation_Divide;
		case aiTextureOp_SmoothAdd:
			return Material::TextureOperation_SmoothAdd;
		case aiTextureOp_SignedAdd:
			return Material::TextureOperation_SignedAdd;
		default:
			return Material::TextureOperation_Replace;
	};
}

Material::ShadingMode DVDConverter::shadingModeInternal(I32 mode){
	switch(mode){
		case aiShadingMode_Flat:
			return Material::SHADING_FLAT;
		case aiShadingMode_Gouraud:
			return Material::SHADING_GOURAUD;
		default:
		case aiShadingMode_Phong:
			return Material::SHADING_PHONG;
		case aiShadingMode_Blinn:
			return Material::SHADING_BLINN;
		case aiShadingMode_Toon:
			return Material::SHADING_TOON;
		case aiShadingMode_OrenNayar:
			return Material::SHADING_OREN_NAYAR;
		case aiShadingMode_Minnaert:
			return Material::SHADING_MINNAERT;
		case aiShadingMode_CookTorrance:
			return Material::SHADING_COOK_TORRANCE;
		case aiShadingMode_NoShading:
			return Material::SHADING_NONE;
		case aiShadingMode_Fresnel:
			return Material::SHADING_FRESNEL;
	};
}

Texture::TextureWrap DVDConverter::aiTextureMapModeToInternal(aiTextureMapMode mode){
	switch(mode){
		case aiTextureMapMode_Wrap:
			return Texture::TextureWrap_Wrap;
		case aiTextureMapMode_Clamp:
			return Texture::TextureWrap_Clamp;
		case aiTextureMapMode_Decal:
			return Texture::TextureWrap_Decal;
		default:
		case aiTextureMapMode_Mirror:
			return Texture::TextureWrap_Repeat;
	};
}