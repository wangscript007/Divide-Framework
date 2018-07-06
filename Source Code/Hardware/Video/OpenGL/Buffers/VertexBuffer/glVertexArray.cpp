#include "Headers/glVertexArray.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Managers/Headers/ShaderManager.h"

#define VOID_PTR_CAST(X) (const GLvoid*)(X)

/// Default destructor
glVertexArray::glVertexArray(const PrimitiveType& type) : VertexBuffer(type),
                                                          _refreshQueued(false),
                                                          _animationData(false),
                                                          _formatInternal(GL_NONE),
                                                          _typeInternal(GL_NONE)
{
    //We assume everything is static draw
    _usage = GL_STATIC_DRAW;
    memset(_prevSize, -1, VertexAttribute_PLACEHOLDER * sizeof(GLsizei));
    _prevSizeIndices = -1;
    _VAOid = _VBid = _IBid = 0;

    _multiCount.reserve(MAX_DRAW_COMMANDS);
    _multiIndices.reserve(MAX_DRAW_COMMANDS);
}

glVertexArray::~glVertexArray()
{
    Destroy();
}

/// Delete buffer
void glVertexArray::Destroy(){
    if(_IBid > 0)  glDeleteBuffers(1, &_IBid);
    if(_VAOid > 0) glDeleteVertexArrays(1, &_VAOid);
    if(_VBid > 0)  glDeleteBuffers(1, &_VBid);
    _VAOid = _VBid = _IBid = 0;
}

/// Create a dynamic or static VB
bool glVertexArray::Create(bool staticDraw){
    //If we want a dynamic buffer, then we are doing something outdated, such as software skinning, or software water rendering
    if(!staticDraw){
        GLenum usage = GL_DYNAMIC_DRAW;
        //OpenGLES support isn't added, but this check doesn't break anything, so I'll just leave it here -Ionut
        if(GFX_DEVICE.getApi() == OpenGLES) usage = GL_STREAM_DRAW;
        if(usage != _usage){
            _usage = usage;
            _refreshQueued = true;
        }
    }

    return true;
}

/// Update internal data based on construction method (dynamic/static)
bool glVertexArray::Refresh(){
    //Dynamic LOD elements (such as terrain) need dynamic indices
    //We can manually override indice usage (again, used by the Terrain rendering system)
    DIVIDE_ASSERT(!_hardwareIndicesL.empty() || !_hardwareIndicesS.empty(), "glVertexArray error: Invalid indice data on Refresh()!");

    GLsizei nSize[VertexAttribute_PLACEHOLDER];
    //Get the size of each data container
    nSize[ATTRIB_POSITION]    = (GLsizei)(_dataPosition.size()*sizeof(vec3<GLfloat>));
    nSize[ATTRIB_COLOR]       = (GLsizei)(_dataColor.size()*sizeof(vec3<GLubyte>));
    nSize[ATTRIB_NORMAL]      = (GLsizei)(_dataNormal.size()*sizeof(vec3<GLfloat>));
    nSize[ATTRIB_TEXCOORD]    = (GLsizei)(_dataTexcoord.size()*sizeof(vec2<GLfloat>));
    nSize[ATTRIB_TANGENT]     = (GLsizei)(_dataTangent.size()*sizeof(vec3<GLfloat>));
    nSize[ATTRIB_BITANGENT]   = (GLsizei)(_dataBiTangent.size()*sizeof(vec3<GLfloat>));
    nSize[ATTRIB_BONE_WEIGHT] = (GLsizei)(_boneWeights.size()*sizeof(vec4<GLfloat>));
    nSize[ATTRIB_BONE_INDICE] = (GLsizei)(_boneIndices.size()*sizeof(vec4<GLubyte>));
    GLsizei nSizeIndices      = (GLsizei)(_largeIndices ? _hardwareIndicesL.size() * sizeof(GLuint) : _hardwareIndicesS.size() * sizeof(GLushort));
    // if any of the VBO's components changed size, we need to recreate the entire buffer.
    // could be optimized, I know, but ... 
    bool sizeChanged = false;
    
    for (U8 i = 0; i < VertexAttribute_PLACEHOLDER; ++i){
        if (nSize[i] != _prevSize[i]){
            sizeChanged = true;
            _prevSize[i] = nSize[i];
        }
    }

    bool indicesChanged = (nSizeIndices != _prevSizeIndices);

    _prevSizeIndices = nSizeIndices;
   
    _refreshQueued = sizeChanged || indicesChanged;

    if (!_refreshQueued)
        for (U8 i = 0; i < VertexAttribute_PLACEHOLDER; ++i)
            if (_attribDirty[i]){
                _refreshQueued = true;
                break;
            }
  
    if (!_refreshQueued)
        return true;

    //Bind the current VAO to save our attribs
    GL_API::setActiveVAO(_VAOid);
    //Bind the the vertex buffer and index buffer
    GL_API::setActiveBuffer(GL_ARRAY_BUFFER, _VBid);
    GL_API::setActiveBuffer(GL_ELEMENT_ARRAY_BUFFER, _IBid);

    //Check if we have any animation data.
    _animationData = (!_boneWeights.empty() && !_boneIndices.empty());

    if (sizeChanged || indicesChanged) computeTriangleList();

    GLsizei triangleCount = (GLsizei)(_computeTriangles ? _dataTriangles.size() : -1);

    if (sizeChanged){
        //get the 8-bit size factor of the VB
        GLsizei vbSize = 0;
        for (U8 i = 0; i < VertexAttribute_PLACEHOLDER; ++i)
            vbSize += nSize[i];

        D_PRINT_FN(Locale::get("DISPLAY_VB_METRICS"), vbSize, 4 * 1024 * 1024, triangleCount, 1000);
        //Create our regular VB with all of the data packed in it
        glBufferData(GL_ARRAY_BUFFER, vbSize, NULL, _usage);
        memset(_attribDirty, true, VertexAttribute_PLACEHOLDER * sizeof(bool));
    }

    const GLvoid* offsets[VertexAttribute_PLACEHOLDER];
    offsets[ATTRIB_POSITION]    = _dataPosition.empty()  ? 0 : VOID_PTR_CAST(&_dataPosition.front().x);
    offsets[ATTRIB_COLOR]       = _dataColor.empty()     ? 0 : VOID_PTR_CAST(&_dataColor.front().r);
    offsets[ATTRIB_NORMAL]      = _dataNormal.empty()    ? 0 : VOID_PTR_CAST(&_dataNormal.front().x);
    offsets[ATTRIB_TEXCOORD]    = _dataTexcoord.empty()  ? 0 : VOID_PTR_CAST(&_dataTexcoord.front().s);
    offsets[ATTRIB_TANGENT]     = _dataTangent.empty()   ? 0 : VOID_PTR_CAST(&_dataTangent.front().x);
    offsets[ATTRIB_BITANGENT]   = _dataBiTangent.empty() ? 0 : VOID_PTR_CAST(&_dataBiTangent.front().x);
    offsets[ATTRIB_BONE_WEIGHT] = _boneWeights.empty()   ? 0 : VOID_PTR_CAST(&_boneWeights.front().x);
    offsets[ATTRIB_BONE_INDICE] = _boneIndices.empty()   ? 0 : VOID_PTR_CAST(&_boneIndices.front().x);

    //Get Position offset
    I32 attrib = ATTRIB_POSITION;
    _VBoffset[attrib] = 0;
    GLsizei previousOffset = _VBoffset[attrib];
    GLsizei previousCount = nSize[attrib];
    
    for (; attrib < VertexAttribute_PLACEHOLDER; ++attrib){
        if (nSize[attrib] > 0) {
            if (attrib > ATTRIB_POSITION){
                _VBoffset[attrib] = previousOffset + previousCount;
                previousOffset = _VBoffset[attrib];
                previousCount = nSize[attrib];
            }
            if(_attribDirty[attrib]){
                glBufferSubData(GL_ARRAY_BUFFER, _VBoffset[attrib], nSize[attrib], offsets[attrib]);
                _attribDirty[attrib] = false;
            }
        }
    }
   
    if (indicesChanged){
        //Update our IB
        if(_largeIndices) glBufferData(GL_ELEMENT_ARRAY_BUFFER, _hardwareIndicesL.size() * sizeof(GLuint),   VOID_PTR_CAST(&_hardwareIndicesL.front()), GL_STATIC_DRAW);
        else              glBufferData(GL_ELEMENT_ARRAY_BUFFER, _hardwareIndicesS.size() * sizeof(GLushort), VOID_PTR_CAST(&_hardwareIndicesS.front()), GL_STATIC_DRAW);
    }

    
    Upload_VB_Attributes();
    
    checkStatus();

    _refreshQueued = false;
    return true;
}

/// This method creates the initial VB
/// The only difference between Create and Refresh is the generation of a new buffer
bool glVertexArray::CreateInternal() {
    DIVIDE_ASSERT(!_created, "glVertexArray error: Attempted to double create a VB");

    if(_dataPosition.empty()){
        ERROR_FN(Locale::get("ERROR_VB_POSITION"));
        _created = false;
        return _created;
    }
    _formatInternal = glDataFormat[_format];
    _typeInternal = glPrimitiveTypeTable[_type];
    // Enforce all update flags. Kinda useless, but it doesn't hurt
    memset(_attribDirty, true, VertexAttribute_PLACEHOLDER * sizeof(bool));
    // Generate a "Vertex Array Object"
    glGenVertexArrays(1, &_VAOid);
    // Generate a new Vertex Buffer Object
    glGenBuffers(1, &_VBid);
    // Generate an "Index Buffer Object"
    glGenBuffers(1, &_IBid);
    // Validate buffer creation
    DIVIDE_ASSERT(_VAOid != 0, Locale::get("ERROR_VAO_INIT"));
    DIVIDE_ASSERT(_VBid != 0, Locale::get("ERROR_VB_INIT"));
    //Show an error and assert if the IB creation failed
    DIVIDE_ASSERT(_IBid != 0, Locale::get("ERROR_IB_INIT"));

    // calling refresh updates all stored information and sends it to the GPU
    _created = queueRefresh();
    return _created;
}

void glVertexArray::Draw(const DeferredDrawCommand& command, bool skipBind) {
    U32 instanceCount = command._cmd.instanceCount;

    if(instanceCount == 0)
        return;

    if(!skipBind)
        if(!SetActive())
            return;
    
    GLsizei count = command._cmd.count == 0 ? (GLsizei)(_largeIndices ? _hardwareIndicesL.size() : _hardwareIndicesS.size()) : command._cmd.count;
    const GLvoid* bufferOffset = BUFFER_OFFSET(command._cmd.firstIndex * (_largeIndices ? sizeof(GLuint) : sizeof(GLushort)));
    if(instanceCount == 1) {
        const vec2<U32>& limits = _indiceLimits[command._lodIndex][command._unsignedData];
        if(limits.y > 0){
            DIVIDE_ASSERT(limits.y > limits.x, "glVertexArray error: indices count does not match indice limits for current LoD!");
            glDrawRangeElements(_typeInternal, limits.x, limits.y, count, _formatInternal, bufferOffset);
        }else{
            glDrawElements(_typeInternal, count, _formatInternal, bufferOffset);
        }
    }else{
        glDrawElementsInstanced(_typeInternal, count, _formatInternal, bufferOffset, instanceCount);
    }

    GL_API::registerDrawCall();
}

void glVertexArray::Draw(const vectorImpl<DeferredDrawCommand>& commands, bool skipBind) {
    if(commands.empty()) 
        return;    

    //SetActive returns immediately if it's already been called
    if(!skipBind)
        if(!SetActive())
            return;

    if(!Config::BATCH_DRAW_COMMANDS || commands[0]._cmd.instanceCount > 1){
        for(const DeferredDrawCommand& cmd : commands)
            Draw(cmd, true);
        return;
    }

    DIVIDE_ASSERT(commands.size() < MAX_DRAW_COMMANDS, "glVertexArray error: too many draw commands received!");

    _multiCount.resize(0);
    _multiIndices.resize(0);
    GLsizei size = (_largeIndices ? sizeof(GLuint) : sizeof(GLushort));
    for (const DeferredDrawCommand& cmd : commands){
        _multiCount.push_back(cmd._cmd.count);
        _multiIndices.push_back(BUFFER_OFFSET(cmd._cmd.firstIndex * size));
    }
    glMultiDrawElements(_typeInternal, &_multiCount.front(), _formatInternal, &_multiIndices.front(), (GLsizei)commands.size());
    GL_API::registerDrawCall();
}

/// If we do not have a VAO, we use vertex arrays as fail safes
bool glVertexArray::SetActive(){
    //If we do not use a shader, than most states are texCoord based, so a VAO is useless
    DIVIDE_ASSERT(_currentShader != nullptr && _currentShader->getId() != 0, Locale::get("ERROR_VAO_SHADER"));

    //if the shader is not ready, do not draw anything
    if(!_currentShader->isBound() || GFXDevice::getActiveProgram()->getId() != _currentShader->getId())
        return false;

    // Make sure we have valid data
    if (!_created) CreateInternal();

    // Make sure we do not have a refresh request queued up
    if(_refreshQueued)
        if(!Refresh())
            return false;

    //Choose optimal VAO/VB combo
    if (GL_API::setActiveVAO(_VAOid))
        GL_API::togglePrimitiveRestart(_primitiveRestartEnabled, !_largeIndices);
    
    //Send our transformation matrices (projection, world, view, inv model view, etc)
    currentShader()->uploadNodeMatrices();

    return true;
}

//Bind vertex attributes 
void glVertexArray::Upload_VB_Attributes(){
    glEnableVertexAttribArray(Divide::VERTEX_POSITION_LOCATION);
    glVertexAttribPointer(Divide::VERTEX_POSITION_LOCATION, 3,
                                  GL_FLOAT, GL_FALSE, sizeof(vec3<GLfloat>),
                                  BUFFER_OFFSET(_VBoffset[ATTRIB_POSITION]));

    if (!_dataColor.empty()) {
        glEnableVertexAttribArray(Divide::VERTEX_COLOR_LOCATION);
        glVertexAttribPointer(Divide::VERTEX_COLOR_LOCATION, 3, 
                                      GL_UNSIGNED_BYTE, GL_FALSE, sizeof(vec3<GLubyte>),
                                      BUFFER_OFFSET(_VBoffset[ATTRIB_COLOR]));
    }else{
        glDisableVertexAttribArray(Divide::VERTEX_COLOR_LOCATION);
    }

    if (!_dataNormal.empty()) {
        glEnableVertexAttribArray(Divide::VERTEX_NORMAL_LOCATION);
        glVertexAttribPointer(Divide::VERTEX_NORMAL_LOCATION, 3,
                                      GL_FLOAT, GL_FALSE, sizeof(vec3<GLfloat>),
                                      BUFFER_OFFSET(_VBoffset[ATTRIB_NORMAL]));
    }else{
        glDisableVertexAttribArray(Divide::VERTEX_NORMAL_LOCATION);
    }

    if(!_dataTexcoord.empty()) {
        glEnableVertexAttribArray(Divide::VERTEX_TEXCOORD_LOCATION);
        glVertexAttribPointer(Divide::VERTEX_TEXCOORD_LOCATION, 2,
                                      GL_FLOAT, GL_FALSE, sizeof(vec2<GLfloat>),
                                      BUFFER_OFFSET(_VBoffset[ATTRIB_TEXCOORD]));
    }else{
        glDisableVertexAttribArray(Divide::VERTEX_TEXCOORD_LOCATION);
    }

    if (!_dataTangent.empty()){
        glEnableVertexAttribArray(Divide::VERTEX_TANGENT_LOCATION);
        glVertexAttribPointer(Divide::VERTEX_TANGENT_LOCATION, 3,
                                      GL_FLOAT, GL_FALSE, sizeof(vec3<GLfloat>),
                                      BUFFER_OFFSET(_VBoffset[ATTRIB_TANGENT]));
    }else{
        glDisableVertexAttribArray(Divide::VERTEX_TANGENT_LOCATION);
    }

    if(!_dataBiTangent.empty()){
        glEnableVertexAttribArray(Divide::VERTEX_BITANGENT_LOCATION);
        glVertexAttribPointer(Divide::VERTEX_BITANGENT_LOCATION, 3,
                                      GL_FLOAT, GL_FALSE, sizeof(vec3<GLfloat>),
                                      BUFFER_OFFSET(_VBoffset[ATTRIB_BITANGENT]));
    }else{
        glDisableVertexAttribArray(Divide::VERTEX_BITANGENT_LOCATION);
    }

    if(_animationData){
        //Bone weights
        glEnableVertexAttribArray(Divide::VERTEX_BONE_WEIGHT_LOCATION);
        glVertexAttribPointer(Divide::VERTEX_BONE_WEIGHT_LOCATION, 4,
                                      GL_FLOAT, GL_FALSE, sizeof(vec4<GLfloat>),
                                      BUFFER_OFFSET(_VBoffset[ATTRIB_BONE_WEIGHT]));

        // Bone indices
        glEnableVertexAttribArray(Divide::VERTEX_BONE_INDICE_LOCATION);
        glVertexAttribIPointer(Divide::VERTEX_BONE_INDICE_LOCATION, 4,
                                       GL_UNSIGNED_BYTE, sizeof(vec4<GLubyte>),
                                       BUFFER_OFFSET(_VBoffset[ATTRIB_BONE_INDICE]));
    }else{
        glDisableVertexAttribArray(Divide::VERTEX_BONE_WEIGHT_LOCATION);
        glDisableVertexAttribArray(Divide::VERTEX_BONE_INDICE_LOCATION);
    }
}

//Create a list of triangles from the vertices + indices lists based on primitive type
bool glVertexArray::computeTriangleList(){
    //We can't have a VB without vertex positions
    DIVIDE_ASSERT(!_dataPosition.empty(), "glVertexArray error: computeTriangleList called with no position data available!");

    if(!_computeTriangles) return true;
    if(!_dataTriangles.empty()) return true;
    if(_hardwareIndicesL.empty() && _hardwareIndicesS.empty()) return false;

    size_t indiceCount = _largeIndices ? _hardwareIndicesL.size() : _hardwareIndicesS.size();

    if(_type == TRIANGLE_STRIP){
        vec3<U32> curTriangle;
        _dataTriangles.reserve(indiceCount * 0.5);
        if(_largeIndices){
            for(U32 i = 2; i < indiceCount; i++){
                curTriangle.set(_hardwareIndicesL[i - 2], _hardwareIndicesL[i - 1], _hardwareIndicesL[i]);
                //Check for correct winding
                if (i % 2 != 0) std::swap(curTriangle.y, curTriangle.z);

                _dataTriangles.push_back(curTriangle);
            }
        }else{
            for(U32 i = 2; i < indiceCount; i++){
                curTriangle.set(_hardwareIndicesS[i - 2], _hardwareIndicesS[i - 1], _hardwareIndicesS[i]);
                //Check for correct winding
                if (i % 2 != 0) std::swap(curTriangle.y, curTriangle.z);

                _dataTriangles.push_back(curTriangle);
            }
        }
    }else if (_type == TRIANGLES){
        indiceCount /= 3;
        _dataTriangles.reserve(indiceCount);
        if(_largeIndices){
            for(U32 i = 0; i < indiceCount; i++){
                 _dataTriangles.push_back(vec3<U32>(_hardwareIndicesL[i*3 + 0],_hardwareIndicesL[i*3 + 1],_hardwareIndicesL[i*3 + 2]));
            }
         }else{
             for(U32 i = 0; i < indiceCount; i++){
                 _dataTriangles.push_back(vec3<U32>(_hardwareIndicesS[i*3 + 0],_hardwareIndicesS[i*3 + 1],_hardwareIndicesS[i*3 + 2]));
            }
        }
    }/*else if (_type == QUADS){
        indiceCount *= 0.5;
        _dataTriangles.reserve(indiceCount);
        if(_largeIndices){
            for(U32 i = 0; i < (indiceCount) - 2; i++){
                if (!(_hardwareIndicesL[i] % 2)){
                    _dataTriangles.push_back(vec3<U32>(_hardwareIndicesL[i*2 + 0],_hardwareIndicesL[i*2 + 1],_hardwareIndicesL[i*2 + 3]));
                }else{
                    _dataTriangles.push_back(vec3<U32>(_hardwareIndicesL[i*2 + 3],_hardwareIndicesL[i*2 + 1],_hardwareIndicesL[i*2 + 0]));
                }
            }
        }else{
                for(U32 i = 0; i < (indiceCount) - 2; i++){
                if (!(_hardwareIndicesS[i] % 2)){
                    _dataTriangles.push_back(vec3<U32>(_hardwareIndicesS[i*2 + 0],_hardwareIndicesS[i*2 + 1],_hardwareIndicesS[i*2 + 3]));
                }else{
                    _dataTriangles.push_back(vec3<U32>(_hardwareIndicesS[i*2 + 3],_hardwareIndicesS[i*2 + 1],_hardwareIndicesS[i*2 + 0]));
                }
            }
        }
    }*/

    //Check for degenerate triangles
    _dataTriangles.erase(std::partition(_dataTriangles.begin(), _dataTriangles.end(),
                         [](const vec3<U32>& triangle) -> bool {
                            return (triangle.x != triangle.y && triangle.x != triangle.z && triangle.y != triangle.z);
                        }), _dataTriangles.end());

    DIVIDE_ASSERT(!_dataTriangles.empty(), "glVertexArray error: computeTriangleList() failed to generate any triangles!");
    return true;
}
//Various post-create checks
void glVertexArray::checkStatus(){
}