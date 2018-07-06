#include "Hardware/Video/OpenGL/Headers/glResources.h"
#include "core.h"
#include "Headers/glVertexArrayObject.h"
#include "Hardware/Video/Headers/GFXDevice.h"

/// Default destructor
glVertexArrayObject::glVertexArrayObject(PrimitiveType type) : VertexBufferObject(type) {
    _bound = false;
	_useVA = false;
	_created = false;
	_animationData = false;
	_refreshQueued = false;
	_VAOid = 0;
    _DepthVAOid = 0;
}

/// Delete buffer
void glVertexArrayObject::Destroy(){
	if(_VBOid > 0){
		GLCheck(glDeleteBuffers(1, &_VBOid));
	}
    if(_DepthVBOid > 0){
        GLCheck(glDeleteBuffers(1, &_DepthVBOid));
    }
	if(_IBOid > 0){
		GLCheck(glDeleteBuffers(1, &_IBOid));
	}
	if(_VAOid > 0){
		GLCheck(glDeleteVertexArrays(1, &_VAOid));
	}
	if(_DepthVAOid > 0){
        GLCheck(glDeleteBuffers(1, &_DepthVAOid));
    }
}

/// Create a dynamic or static VBO
bool glVertexArrayObject::Create(bool staticDraw){	
	_staticDraw = staticDraw;
	return true;
}

bool glVertexArrayObject::queueRefresh() {
	if(_created) return Refresh();
	else _refreshQueued = true;
	return true;	
}

/// Update internal data based on construction method (dynamic/static)
bool glVertexArrayObject::Refresh(){
	///Skip for vertex arrays
	if(_useVA) {
		_refreshQueued = false;
		return true;
	}
	//If we do not use a shader, than most states are texCoord based, so a VAO is useless
	bool skipVAO = (_currentShader == NULL);
	//Dynamic LOD elements (such as terrain) need dynamic indices
	bool skipIndiceBuffer = _hardwareIndices.empty() ? true : false;
    //We can manually override indice usage (again, used by the Terrain rendering system)
    if(!_useHWIndices) skipIndiceBuffer = true;
    //We can't have a VBO without vertex positions
	assert(!_dataPosition.empty());
    //We assume everything is static draw
	GLuint usage = GL_STATIC_DRAW;
    //If we wan't a dynamic buffer, than we are doing something outdated, such as software skinning, or software water rendering
	if(!_staticDraw){
        //OpenGLES support isn't added, but this check doesn't break anything, so I'll just leave it here -Ionut
		(GFX_DEVICE.getApi() == OpenGLES) ?  usage = GL_STREAM_DRAW :  usage = GL_DYNAMIC_DRAW;
	}
    if(_dataTriangles.empty() && !_hardwareIndices.empty()){
        computeTriangleList();
        assert(_dataTriangles.size() > 0);
    }
    //Get the size of each data container
	ptrdiff_t nSizePosition    = _dataPosition.size()*sizeof(vec3<GLfloat>);
	ptrdiff_t nSizeNormal      = _dataNormal.size()*sizeof(vec3<GLfloat>);
	ptrdiff_t nSizeTexcoord    = _dataTexcoord.size()*sizeof(vec2<GLfloat>);
	ptrdiff_t nSizeTangent     = _dataTangent.size()*sizeof(vec3<GLfloat>);
	ptrdiff_t nSizeBiTangent   = _dataBiTangent.size()*sizeof(vec3<GLfloat>);
	ptrdiff_t nSizeBoneWeights = _boneWeights.size()*sizeof(vec4<GLfloat>);
	ptrdiff_t nSizeBoneIndices = _boneIndices.size()*sizeof(vec4<GLushort>);
	//Check if we have any animation data.
	_animationData = (!_boneWeights.empty() && !_boneIndices.empty());

    //get the 8-bit size factor of the VBO (nr. of bytes on Win32)
    ptrdiff_t vboSize = nSizePosition + nSizeNormal + nSizeTexcoord + nSizeTangent + nSizeBiTangent + nSizeBoneWeights + nSizeBoneIndices;
    //Depth optimization is not needed for small meshes. Use config.h to tweak this
    U32 triangleCount = _dataTriangles.size();
    D_PRINT_FN(Locale::get("DISPLAY_VBO_METRICS"), vboSize, DEPTH_VBO_MIN_BYTES,_dataTriangles.size(),DEPTH_VBO_MIN_TRIANGLES);
    if(triangleCount < DEPTH_VBO_MIN_TRIANGLES || vboSize < DEPTH_VBO_MIN_BYTES){
        _optimizeForDepth = false;
    }
    if(_forceOptimizeForDepth) _optimizeForDepth = true;

    //If we do not have a depth VAO or a depth VBO but we need one, create them
    if(_optimizeForDepth && (!_DepthVAOid && !_DepthVBOid)){
        // Create a VAO for depth rendering only if needed
        if(!skipVAO) GLCheck(glGenVertexArrays(1, &_DepthVAOid));
        // Create a VBO for 4depth rendering only;
        GLCheck(glGenBuffers(1, &_DepthVBOid));
        if((!_DepthVAOid && !skipVAO) || !_DepthVBOid){
            ERROR_FN(Locale::get("ERROR_VBO_DEPTH_INIT"));
            // fallback to regular VAO rendering in depth-only mode
            _optimizeForDepth = false;
            _forceOptimizeForDepth = false;
        }
    }
	
    //Get Position offset
	_VBOoffsetPosition  = 0;
	ptrdiff_t previousOffset = _VBOoffsetPosition;
	ptrdiff_t previousCount = nSizePosition;
    //Also, get position offset for the DEPTH-only VBO.
    //No need to check if we want depth-only VBO's as it's a very small variable
    ptrdiff_t previousOffsetDEPTH = previousOffset;
    ptrdiff_t previousCountDEPTH = previousCount;
    //If we have any normal data ...
	if(nSizeNormal > 0){
        //... get it's offset
		_VBOoffsetNormal = previousOffset + previousCount;
		previousOffset = _VBOoffsetNormal;
		previousCount = nSizeNormal;
        //Also update our DEPTH-only offset
        if(_optimizeForDepth){
            //So far, depth and non-depth only offsets are equal
            previousOffsetDEPTH = previousOffset;
            previousCountDEPTH = previousCount;
        }
	}
    //We ignore all furture offsets for depth-only, until we reach the animation data
	if(nSizeTexcoord > 0){
        //Get tex coord's offsets
		_VBOoffsetTexcoord	= previousOffset + previousCount;
		previousOffset = _VBOoffsetTexcoord;
		previousCount = nSizeTexcoord;
	}

	if(nSizeTangent > 0){
        //tangent's offsets
		_VBOoffsetTangent	= previousOffset + previousCount;
		previousOffset = _VBOoffsetTangent;
		previousCount = nSizeTangent;
	}

	if(nSizeBiTangent > 0){
        //bitangent's offsets
		_VBOoffsetBiTangent	  = previousOffset + previousCount;
		previousOffset = _VBOoffsetBiTangent;
		previousCount = nSizeBiTangent;
	}

    // Bone indices and weights are always packed togheter to prevent invalid animations
	if(_animationData){
	    //Get non depth-only offsets
		_VBOoffsetBoneWeights = previousOffset + previousCount;
		previousOffset = _VBOoffsetBoneWeights;
		previousCount   = nSizeBoneWeights;
		_VBOoffsetBoneIndices = previousOffset + previousCount;
        if(_optimizeForDepth){
            //And update depth-only updates if needed
            _VBOoffsetBoneWeightsDEPTH = previousOffsetDEPTH + previousCountDEPTH;
            previousOffsetDEPTH = _VBOoffsetBoneWeightsDEPTH;
            previousCountDEPTH = nSizeBoneWeights;
            _VBOoffsetBoneIndicesDEPTH = previousOffsetDEPTH + previousCountDEPTH;
        }
	}

	//Show an error and assert if we do not have a valid buffer for storage
	if(_VBOid == 0) PRINT_FN(Locale::get("ERROR_VBO_INIT"));
	assert(_VBOid != 0); 
    //Bind the regular VBO
    GL_API::setActiveVBO(_VBOid);
    //Create our regular VBO with all of the data packed in it
	GLCheck(glBufferData(GL_ARRAY_BUFFER, nSizePosition+
									      nSizeNormal+
										  nSizeTexcoord+
										  nSizeTangent+
										  nSizeBiTangent+
										  nSizeBoneWeights+
										  nSizeBoneIndices,
										  0, usage));
    //Add position data
	if(_positionDirty){
	    GLCheck(glBufferSubData(GL_ARRAY_BUFFER, _VBOoffsetPosition, nSizePosition,(const GLvoid*)(&_dataPosition.front().x)));
		// Clear all update flags now, if we do not use a depth-only VBO, else clear it there
		if(!_optimizeForDepth) _positionDirty  = false;
	}
    //Add normal data
	if(_dataNormal.size() && _normalDirty){
        //Ignore for now
		/*for(U32 i = 0; i < _dataNormal.size(); i++){
			g_normalsSmall.push_back(UP1010102(_dataNormal[i].x,_dataNormal[i].z,_dataNormal[i].y,1.0f));
		}*/
		GLCheck(glBufferSubData(GL_ARRAY_BUFFER, _VBOoffsetNormal, nSizeNormal,(const GLvoid*)(&_dataNormal.front().x)));
        // Same rule applies for clearing the normal flag
		if(!_optimizeForDepth) _normalDirty    = false;
	}
    //Add tex coord data
	if(_dataTexcoord.size() && _texcoordDirty){
		GLCheck(glBufferSubData(GL_ARRAY_BUFFER, _VBOoffsetTexcoord, nSizeTexcoord, (const GLvoid*)(&_dataTexcoord.front().s)));
        //We do not use tex coord data in our depth only vbo, so clear it here
		_texcoordDirty  = false;
	}
    //Add tangent data
	if(_dataTangent.size() && _tangentDirty){
		GLCheck(glBufferSubData(GL_ARRAY_BUFFER, _VBOoffsetTangent, nSizeTangent,(const GLvoid*)(&_dataTangent.front().x)));
        //We do not use tangent data in our depth only vbo, so clear it here
		_tangentDirty   = false;
	}
    //Add bitangent data
	if(_dataBiTangent.size() && _bitangentDirty){
		GLCheck(glBufferSubData(GL_ARRAY_BUFFER, _VBOoffsetBiTangent,nSizeBiTangent,(const GLvoid*)(&_dataBiTangent.front().x)));
        //We do not use bitangent data in our depth only vbo, so clear it here
		_bitangentDirty = false;
	}
    //Add animation data
	if(_animationData) {
		if(_indicesDirty){
			GLCheck(glBufferSubData(GL_ARRAY_BUFFER, _VBOoffsetBoneIndices,nSizeBoneIndices,(const GLvoid*)(&_boneIndices.front().x)));
            //Clear the indice flag after we updated the depth-only vbo as well
			if(!_optimizeForDepth) _indicesDirty   = false;
		}

		if(_weightsDirty){
			GLCheck(glBufferSubData(GL_ARRAY_BUFFER, _VBOoffsetBoneWeights,nSizeBoneWeights,(const GLvoid*)(&_boneWeights.front().x)));
            //Clear the weights flag after we updated the depth-only vbo as well
			if(!_optimizeForDepth) _weightsDirty   = false;
		}

	}
    //Time for our depth-only VBO creation
    if(_optimizeForDepth){
        //Show an error and assert if we do not have a valid buffer to stare data in
	    if(_DepthVBOid == 0) PRINT_FN(Locale::get("ERROR_VBO_DEPTH_INIT"));
	    assert(_DepthVBOid != 0); 
        //Bind our depth-only buffer
        GL_API::setActiveVBO(_DepthVBOid);
        //Create our depth-only VBO with all of the data packed in it
        GLCheck(glBufferData(GL_ARRAY_BUFFER, nSizePosition+
									          nSizeNormal+
										      nSizeBoneWeights+
										      nSizeBoneIndices,
										      0, usage));
        //Add position data
	    if(_positionDirty){
	        GLCheck(glBufferSubData(GL_ARRAY_BUFFER, _VBOoffsetPosition, nSizePosition,	(const GLvoid*)(&_dataPosition.front().x)));
		    // Clear the position update flag now
		    _positionDirty  = false;
	    }
        //Add normal data
	    if(_dataNormal.size() && _normalDirty){
            //Ignore for now
		    /*for(U32 i = 0; i < _dataNormal.size(); i++){
			    g_normalsSmall.push_back(UP1010102(_dataNormal[i].x,_dataNormal[i].z,_dataNormal[i].y,1.0f));
		    }*/
		    GLCheck(glBufferSubData(GL_ARRAY_BUFFER, _VBOoffsetNormal, nSizeNormal,(const GLvoid*)(&_dataNormal.front().x)));
            // Clear the normal update flag now
		    _normalDirty    = false;
	    }
        //Add animation data
        if(_animationData) {
		    if(_indicesDirty){
			    GLCheck(glBufferSubData(GL_ARRAY_BUFFER, _VBOoffsetBoneIndicesDEPTH, nSizeBoneIndices,(const GLvoid*)(&_boneIndices.front().x)));
		    	_indicesDirty   = false;
		    }

		    if(_weightsDirty){
			    GLCheck(glBufferSubData(GL_ARRAY_BUFFER, _VBOoffsetBoneWeightsDEPTH, nSizeBoneWeights, (const GLvoid*)(&_boneWeights.front().x)));
			    _weightsDirty   = false;
		    }
    	}
    }
    //Unbind the current VBO (either normal or depth-only)
    GL_API::setActiveVBO(0);
    //We need to make double sure, we are not trying to create a regular VBO during a depth pass or else everything will crash and burn
    bool oldDepthState = _depthPass;
    _depthPass = false;
    //If we need a regular VAO (we have a shader to render the current geometry)
	if(!skipVAO){
        // Generate a "VertexArrayObject"
	    if(_VAOid == 0) GLCheck(glGenVertexArrays(1, &_VAOid));
		//Show an error and assert if the VAO creation failed
		if(_VAOid == 0) PRINT_FN(Locale::get("ERROR_VAO_INIT"));
		assert(_VAOid != 0);
        //Bind the current VAO to save our attribs
        GL_API::setActiveVAO(_VAOid);
	}	
    //Next, let's bind our regular VBO (so the current VAO, if any, will know who owns the attributes)
    GL_API::setActiveVBO(_VBOid);
    //If we use IBO's, then we need to create it first. It will also bind to the current VAO if it exists (skipVAO == false)
	if(!skipIndiceBuffer){
        //Show an error and assert if the IBO creation failed
        if(_IBOid == 0) PRINT_FN(Locale::get("ERROR_IBO_INIT"));
        assert(_IBOid != 0);
        //Bind and create our IBO
        GL_API::setActiveIBO(_IBOid);
		GLCheck(glBufferData(GL_ELEMENT_ARRAY_BUFFER, _hardwareIndices.size() * sizeof(GLushort),
													  (const GLvoid*)(&_hardwareIndices.front()), 
													  usage));
	}
	/// If we do not have a shader assigned, use old style VBO else, use attrib pointers
    if(!skipVAO){
        Enable_Shader_VBO();
         //If we have a VAO, unbinding it will unbind the IBO as well
        GL_API::setActiveVAO(0);
    }else{
    	// VAO's keep track of IBO's, but if we are not using one, we need to manually disable it
		if(!skipIndiceBuffer){
            GL_API::setActiveIBO(0);
		}
	}
     
    // Try and create our DEPTH-only VAO/VBO
    if(_optimizeForDepth){
        // Make sure we bind the correct attribs
        _depthPass = true;
        if(!skipVAO){
		    //Show an error and assert if the depth VAO isn't created properly
		    if(_DepthVAOid == 0) PRINT_FN(Locale::get("ERROR_VAO_DEPTH_INIT"));
		    assert(_DepthVAOid != 0);
            //Bind the depth-only VAO if we are using one
            GL_API::setActiveVAO(_DepthVAOid);
        }	
        //Bind the depth only VBO we constructed earlier
        GL_API::setActiveVBO(_DepthVBOid);
        //Bind the IBO to the depth vao. IBO should have been created during the regular VAO construction
	    if(!skipIndiceBuffer){
            //an assert doesn't hurt
            assert(_IBOid != 0);
            GL_API::setActiveIBO(_IBOid);
        }
	    // If we do not have a shader assigned, use old style VBO else, use attrib pointers
        if(!skipVAO){
            Enable_Shader_VBO();
            GL_API::setActiveVAO(0);
	    // VAO's keep track of IBO's, but if we are not using one, we need to manually disable it
        }else{
		    if(!skipIndiceBuffer){
                GL_API::setActiveIBO(0);
		    }
	    }  
    }
    //Restore depth state to it's previous value
    _depthPass = oldDepthState;
	//Reset vertex buffer object state
    GL_API::setActiveVBO(0);
	checkStatus();
	
	_refreshQueued = false;
	return true;
}


/// This method creates the initial VBO
/// The only diferrence between Create and Refresh is the generation of a new buffer
bool glVertexArrayObject::CreateInternal() {
    if(_dataPosition.empty()){
		ERROR_FN(Locale::get("ERROR_VBO_POSITION"));
		return false;
	}
	// Enforce all update flags. Kinda useless, but it doesn't hurt
	_positionDirty  = true;
	_normalDirty    = true;
	_texcoordDirty  = true;
	_tangentDirty   = true;
	_bitangentDirty = true;
	_indicesDirty   = true;
	_weightsDirty   = true;
	// Generate an "IndexBufferObject" if needed
    if(_useHWIndices){
	    GLCheck(glGenBuffers(1, &_IBOid));
    }
    // Generate a new VBO buffer
	GLCheck(glGenBuffers(1, &_VBOid));
 
	// Validate buffer creation
	if(!_VBOid || (!_IBOid && _useHWIndices)) {
		ERROR_FN(Locale::get("ERROR_VBO_INIT"));
		_useVA = true;
	}else {
		/// calling refresh updates all stored information and sends it to the GPU
		_created = queueRefresh();
	}
	_created = true;
	return _created;
}

/// Inform the VBO what shader we use to render the object so that the data is uploaded correctly
void glVertexArrayObject::setShaderProgram(ShaderProgram* const shaderProgram) {
	if(shaderProgram->getName().compare("NULL_SHADER") == 0){
		_currentShader = NULL;
		queueRefresh();
	}else{
		_currentShader = shaderProgram;
	}
}

///This draw method does not need an Index Buffer!
void glVertexArrayObject::Draw(GFXDataFormat f, U32 count, const void* first_element){
    if(!_bound) Enable();
    GLCheck(glDrawElements(glPrimitiveTypeTable[_type], count, glDataFormat[f],first_element ));
}

///This draw method needs an Index Buffer!
void glVertexArrayObject::Draw(U8 LODindex){
	
	CLAMP<U8>(LODindex,0,_indiceLimits.size() - 1);
	
	assert(_indiceLimits[LODindex].y > _indiceLimits[LODindex].x);
	GLsizei count = _hardwareIndices.size();
	assert(_indiceLimits[LODindex].y < count);

    Enable();
    

    GLCheck(glDrawRangeElements(glPrimitiveTypeTable[_type], 
		                        _indiceLimits[LODindex].x,
								_indiceLimits[LODindex].y,
								count,
								GL_UNSIGNED_SHORT,
								BUFFER_OFFSET(0)));
     Disable();
 }

/// If we do not have a VAO object, we use vertex arrays as fail safes
void glVertexArrayObject::Enable(){
	if(!_created) CreateInternal(); ///< Make sure we have valid data
	if(_refreshQueued) Refresh(); ///< Make sure we do not have a refresh request queued up
    _bound = true;
	if(_useVA) { 
		Enable_VA(); 
	}else{
		if(_currentShader == NULL){
			if(!_hardwareIndices.empty()){
                GL_API::setActiveIBO(_IBOid);
			}
            //if(_depthPass  && _optimizeForDepth){
            //    GL_API::setActiveVBO(_DepthVBOid);
            //}else{
                GL_API::setActiveVBO(_VBOid);
            //}
			Enable_VBO();
			Enable_VBO_TexPointers();		
		}else{
            //if(_depthPass && _optimizeForDepth){
              //  GL_API::setActiveVAO(_DepthVAOid);
            //}else{
                GL_API::setActiveVAO(_VAOid);
            //}
		}
	}
}

/// If we do not have a VAO object, we use vertex arrays as fail safes
void glVertexArrayObject::Disable(){
	if(_useVA){
		Disable_VA();
	}else{
		if(_currentShader == NULL){
			Disable_VBO();
			Disable_VBO_TexPointers();	
			if(!_hardwareIndices.empty()){
                GL_API::setActiveIBO(0);
			}
            GL_API::setActiveVBO(0);
		}else{
            GL_API::setActiveVAO(0);
		}
	}
    _bound = false;
}

/// If we do not have a VBO object, we use vertex arrays as fail safes
void glVertexArrayObject::Enable_VA() {

	GLCheck(glEnableClientState(GL_VERTEX_ARRAY));
	GLCheck(glVertexPointer(3, GL_FLOAT, sizeof(vec3<GLfloat>), &(_dataPosition.front().x)));

	if(!_dataNormal.empty()) {
		GLCheck(glEnableClientState(GL_NORMAL_ARRAY));
		GLCheck(glNormalPointer(GL_FLOAT, sizeof(vec3<GLfloat>), &(_dataNormal.front().x)));
	}

	if(!_dataTexcoord.empty()) {
        GL_API::setClientActiveTextureUnit(0);
		GLCheck(glEnableClientState(GL_TEXTURE_COORD_ARRAY));
		GLCheck(glTexCoordPointer(2, GL_FLOAT, sizeof(vec2<GLfloat>), &(_dataTexcoord.front().s)));
	}

	if(!_dataTangent.empty()) {
        GL_API::setClientActiveTextureUnit(1);
		GLCheck(glEnableClientState(GL_TEXTURE_COORD_ARRAY));
		GLCheck(glTexCoordPointer(3, GL_FLOAT, sizeof(vec3<GLfloat>), &(_dataTangent.front().x)));
	}

	if(!_dataBiTangent.empty()) {
        GL_API::setClientActiveTextureUnit(2);
		GLCheck(glEnableClientState(GL_TEXTURE_COORD_ARRAY));
		GLCheck(glTexCoordPointer(3, GL_FLOAT, sizeof(vec3<GLfloat>), &(_dataBiTangent.front().x)));
	}

	if(_animationData){
		GL_API::setClientActiveTextureUnit(3);
		GLCheck(glEnableClientState(GL_TEXTURE_COORD_ARRAY));
		GLCheck(glTexCoordPointer(4, GL_UNSIGNED_BYTE,sizeof(vec4<GLubyte>), &(_boneIndices.front().x)));
        GL_API::setClientActiveTextureUnit(4);
		GLCheck(glEnableClientState(GL_TEXTURE_COORD_ARRAY));
		GLCheck(glTexCoordPointer(4, GL_FLOAT, sizeof(vec4<GLfloat>), &(_boneWeights.front().x)));
	}
}

/// If we do not have a VBO object, we use vertex arrays as fail safes
void glVertexArrayObject::Disable_VA() {

	if(_animationData){
		GL_API::setClientActiveTextureUnit(4);
		GLCheck(glDisableClientState(GL_TEXTURE_COORD_ARRAY));
		GL_API::setClientActiveTextureUnit(3);
		GLCheck(glDisableClientState(GL_TEXTURE_COORD_ARRAY));
	}

	if(!_dataBiTangent.empty()){
		GL_API::setClientActiveTextureUnit(2);
		GLCheck(glDisableClientState(GL_TEXTURE_COORD_ARRAY));
	}

	if(!_dataTangent.empty()) {
		GL_API::setClientActiveTextureUnit(1);
		GLCheck(glDisableClientState(GL_TEXTURE_COORD_ARRAY));
 	}

	if(!_dataTexcoord.empty()) {
		GL_API::setClientActiveTextureUnit(0);
		GLCheck(glDisableClientState(GL_TEXTURE_COORD_ARRAY));
	}
	
	if(!_dataNormal.empty()) {
		GLCheck(glDisableClientState(GL_NORMAL_ARRAY));
	}

	GLCheck(glDisableClientState(GL_VERTEX_ARRAY));
}


void glVertexArrayObject::Enable_VBO() {
	GLCheck(glEnableClientState(GL_VERTEX_ARRAY));
	GLCheck(glVertexPointer(3, GL_FLOAT, sizeof(vec3<GLfloat>), (const GLvoid*)_VBOoffsetPosition));

	if(!_dataNormal.empty()) {
		GLCheck(glEnableClientState(GL_NORMAL_ARRAY));
		GLCheck(glNormalPointer(GL_FLOAT, sizeof(vec3<GLfloat>), (const GLvoid*)_VBOoffsetNormal));
	}
}

void glVertexArrayObject::Disable_VBO(){
    if(!_dataNormal.empty()) {
		GLCheck(glDisableClientState(GL_NORMAL_ARRAY));
	}

	GLCheck(glDisableClientState(GL_VERTEX_ARRAY));
}

//Bind vertex attributes (only vertex, normal and animation data is used for the depth-only vbo)
void glVertexArrayObject::Enable_Shader_VBO(){
    //bool useDepth = !(!_optimizeForDepth || (!_depthPass && _optimizeForDepth));

	GLCheck(glEnableVertexAttribArray(Divide::GL::VERTEX_POSITION_LOCATION));
	GLCheck(glVertexAttribPointer(Divide::GL::VERTEX_POSITION_LOCATION, 
		                          3,
								  GL_FLOAT,
								  GL_FALSE,
								  sizeof(vec3<GLfloat>),
								  (const GLvoid*)_VBOoffsetPosition));
	if(!_dataNormal.empty()) {
		GLCheck(glEnableVertexAttribArray(Divide::GL::VERTEX_NORMAL_LOCATION));
		GLCheck(glVertexAttribPointer(Divide::GL::VERTEX_NORMAL_LOCATION, 
									  3,
									  GL_FLOAT,
									  GL_FALSE,
									  sizeof(vec3<GLfloat>),
									  (const GLvoid*)_VBOoffsetNormal));
	}
    // If we have a depth-only VBO and this is not a depth pass or we did not optimize for depth rendering,
    // add all the attribs
   // if(!useDepth){
	    if(!_dataTexcoord.empty()) {
		    GLCheck(glEnableVertexAttribArray(Divide::GL::VERTEX_TEXCOORD_LOCATION));
		    GLCheck(glVertexAttribPointer(Divide::GL::VERTEX_TEXCOORD_LOCATION,
			    						  2, 
				    					  GL_FLOAT, 
					    				  GL_FALSE, 
						    			  sizeof(vec2<GLfloat>), 
							    		  (const GLvoid*)_VBOoffsetTexcoord));
	    }

    	if(!_dataTangent.empty()){
	    	GLCheck(glEnableVertexAttribArray(Divide::GL::VERTEX_TANGENT_LOCATION));
		    GLCheck(glVertexAttribPointer(Divide::GL::VERTEX_TANGENT_LOCATION,
			    						  3,
				    					  GL_FLOAT,
					    				  GL_FALSE,
						    			  sizeof(vec3<GLfloat>),
							    		  (const GLvoid*)_VBOoffsetTangent));
	    }

	    if(!_dataBiTangent.empty()){
		    GLCheck(glEnableVertexAttribArray(Divide::GL::VERTEX_BITANGENT_LOCATION));
		    GLCheck(glVertexAttribPointer(Divide::GL::VERTEX_BITANGENT_LOCATION,
			    			              3,
				    					  GL_FLOAT, 
					    				  GL_FALSE,
						    			  sizeof(vec3<GLfloat>),
							    		  (const GLvoid*)_VBOoffsetBiTangent));
	    }
    //}
	if(_animationData){
		/// Bone weights
		GLCheck(glVertexAttribPointer(Divide::GL::VERTEX_BONE_WEIGHT_LOCATION,
			                          4,
									  GL_FLOAT,
									  GL_FALSE,
									  sizeof(vec4<GLfloat>),
                                      //useDepth ? (const GLvoid*)_VBOoffsetBoneWeightsDEPTH : (const GLvoid*)_VBOoffsetBoneWeights));
									  (const GLvoid*)_VBOoffsetBoneWeights));
	
		/// Bone indices
		GLCheck(glVertexAttribIPointer(Divide::GL::VERTEX_BONE_INDICE_LOCATION,
			                           4,
									   GL_UNSIGNED_BYTE,
									   sizeof(vec4<GLubyte>),
                                       //useDepth ? (const GLvoid*)_VBOoffsetBoneIndicesDEPTH : (const GLvoid*)_VBOoffsetBoneIndices));
									  (const GLvoid*)_VBOoffsetBoneIndices));

		GLCheck(glEnableVertexAttribArray(Divide::GL::VERTEX_BONE_WEIGHT_LOCATION));
		GLCheck(glEnableVertexAttribArray(Divide::GL::VERTEX_BONE_INDICE_LOCATION));
	}
}

//Attribute states should be encapsulated by the VAO
void glVertexArrayObject::Disable_Shader_VBO(){}

void glVertexArrayObject::Enable_VBO_TexPointers(){
   // bool useDepth = !(!_optimizeForDepth || (!_depthPass && _optimizeForDepth));

    //if(!useDepth){
	    if(!_dataTexcoord.empty()) {
            GL_API::setClientActiveTextureUnit(0);
		    GLCheck(glEnableClientState(GL_TEXTURE_COORD_ARRAY));
		    GLCheck(glTexCoordPointer(2, GL_FLOAT, sizeof(vec2<GLfloat>), (const GLvoid*)_VBOoffsetTexcoord));
	    }

	    if(!_dataTangent.empty()) {
            GL_API::setClientActiveTextureUnit(1);
		    GLCheck(glEnableClientState(GL_TEXTURE_COORD_ARRAY));
		    GLCheck(glTexCoordPointer(3, GL_FLOAT, sizeof(vec3<GLfloat>), (const GLvoid*)_VBOoffsetTangent));
	    }

	    if(!_dataBiTangent.empty()){
            GL_API::setClientActiveTextureUnit(2);
		    GLCheck(glEnableClientState(GL_TEXTURE_COORD_ARRAY));
		    GLCheck(glTexCoordPointer(3, GL_FLOAT, sizeof(vec3<GLfloat>), (const GLvoid*)_VBOoffsetBiTangent));
	    }
    //}

	if(_animationData){
		GL_API::setClientActiveTextureUnit(3);
		GLCheck(glEnableClientState(GL_TEXTURE_COORD_ARRAY));
        GLCheck(glTexCoordPointer(4, GL_UNSIGNED_BYTE, sizeof(vec4<GLubyte>),/*useDepth ? (const GLvoid*)_VBOoffsetBoneIndicesDEPTH :  */(const GLvoid*)_VBOoffsetBoneIndices));
        GL_API::setClientActiveTextureUnit(4);
		GLCheck(glEnableClientState(GL_TEXTURE_COORD_ARRAY));
        GLCheck(glTexCoordPointer(4, GL_FLOAT, sizeof(vec4<GLfloat>),/*useDepth ? (const GLvoid*)_VBOoffsetBoneWeightsDEPTH : */(const GLvoid*)_VBOoffsetBoneWeights));
	}
}

void glVertexArrayObject::Disable_VBO_TexPointers(){
	if(_animationData){
		GL_API::setClientActiveTextureUnit(4);
		GLCheck(glDisableClientState(GL_TEXTURE_COORD_ARRAY));
		GL_API::setClientActiveTextureUnit(3);
		GLCheck(glDisableClientState(GL_TEXTURE_COORD_ARRAY));
	}
/*
    bool useDepth = !(!_optimizeForDepth || (!_depthPass && _optimizeForDepth));

    if(useDepth){
        GL_API::setClientActiveTextureUnit(0);
        return;
    }
*/
	if(!_dataBiTangent.empty()){
        GL_API::setClientActiveTextureUnit(2);
		GLCheck(glDisableClientState(GL_TEXTURE_COORD_ARRAY));
	}
	if(!_dataTangent.empty()) {
        GL_API::setClientActiveTextureUnit(1);
		GLCheck(glDisableClientState(GL_TEXTURE_COORD_ARRAY));
	}
	if(!_dataTexcoord.empty()){
        GL_API::setClientActiveTextureUnit(0);
		GLCheck(glDisableClientState(GL_TEXTURE_COORD_ARRAY));
	}
}

void glVertexArrayObject::checkStatus(){
}


bool glVertexArrayObject::computeTriangleList(){
    if(_type == TRIANGLE_STRIP){
        _dataTriangles.reserve(_hardwareIndices.size() - 2);
        for(U32 i = 0; i < _hardwareIndices.size() - 2; i++){
            if (!(_hardwareIndices[i] % 2)){
                _dataTriangles.push_back(vec3<U32>(_hardwareIndices[i + 0],_hardwareIndices[i + 1],_hardwareIndices[i + 2]));
            }else{
                _dataTriangles.push_back(vec3<U32>(_hardwareIndices[i + 2],_hardwareIndices[i + 1],_hardwareIndices[i + 0]));
            }
        }
    }else if (_type == TRIANGLES){
         _dataTriangles.reserve(_hardwareIndices.size() / 3);
         for(U32 i = 0; i < _hardwareIndices.size() / 3; i++){
             _dataTriangles.push_back(vec3<U32>(_hardwareIndices[i*3 + 0],_hardwareIndices[i*3 + 1],_hardwareIndices[i*3 + 2]));
         }
    }else if (_type == QUADS){
        _dataTriangles.reserve(_hardwareIndices.size() / 2);
        for(U32 i = 0; i < (_hardwareIndices.size() / 2) - 2; i++){
             if (!(_hardwareIndices[i] % 2)){
                _dataTriangles.push_back(vec3<U32>(_hardwareIndices[i*2 + 0],_hardwareIndices[i*2 + 1],_hardwareIndices[i*2 + 3]));
             }else{
                _dataTriangles.push_back(vec3<U32>(_hardwareIndices[i*2 + 3],_hardwareIndices[i*2 + 1],_hardwareIndices[i*2 + 0]));
             }
        }
    }

    return true;
}