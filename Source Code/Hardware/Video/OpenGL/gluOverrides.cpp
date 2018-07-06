#include "Headers/glResources.h"
#ifndef GLM_MESSAGES
#undef  GLM_MESSAGES
#endif
#include <gtc/matrix_inverse.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include "Hardware/Video/Headers/RenderAPIEnums.h"
#include "Hardware/Video/OpenGL/Headers/GLWrapper.h"

namespace Divide {
    namespace GL {
        ///Used for anaglyph rendering
        struct CameraFrustum {
            GLdouble leftfrustum;
            GLdouble rightfrustum;
            GLdouble bottomfrustum;
            GLdouble topfrustum;
            GLfloat modeltranslation;
        } _leftCam, _rightCam;

        /*-----------Object Management----*/
        GLuint _invalidObjectID = std::numeric_limits<U32>::max();

        /*-----------Context Management----*/
        bool _applicationClosing = false;
        bool _contextAvailable = false;
        bool _useDebugOutputCallback = false;
        GLFWwindow* _mainWindow     = NULL;
        GLFWwindow* _loaderWindow   = NULL;

        /*----------- GLU overrides ------*/
        ///Matrix management
        MATRIX_MODE   _currentMatrixMode;
        matrixStack   _viewMatrix;
        matrixStack   _projectionMatrix;
        matrixStack   _textureMatrix;
        viewportStack _viewport;
        glm::mat4     _identityMatrix;
        vector3Stack  _currentViewDirection;

        GLfloat  _anaglyphIOD = -0.01f;

        void _initStacks(){
            _currentViewDirection.push(vec3<GLfloat>());
            _projectionMatrix.push(_identityMatrix);
            _viewMatrix.push(_identityMatrix);
            _textureMatrix.push(_identityMatrix);
            _viewport.push(vec4<U32>(0,0,0,0));
        }

        void _unproject(const vec3<GLfloat>& windowCoords, vec3<GLfloat>& objCoords) {
            glm::vec4 viewport(_viewport.top().x, _viewport.top().y, _viewport.top().z, _viewport.top().w);
            glm::vec3 wincoord(windowCoords.x, windowCoords.y, windowCoords.z);
            glm::vec3 objcoord = glm::unProject(wincoord, _viewMatrix.top(), _projectionMatrix.top(), viewport);
            objCoords.set(objcoord.x, objcoord.y, objcoord.z);
        }

        void _lookAt(const GLfloat* viewMatrix, const vec3<GLfloat>& viewDirection){
           _matrixMode(VIEW_MATRIX);
           _currentViewDirection.top() = viewDirection;
           _viewMatrix.top() = glm::make_mat4(viewMatrix);
           GL_API::getInstance().updateViewMatrix();
        }

        void _ortho(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar){
            _matrixMode(PROJECTION_MATRIX);
            _projectionMatrix.top() = glm::ortho(left,right,bottom,top,zNear,zFar);
            GL_API::getInstance().updateProjectionMatrix();
        }

        void _perspective(GLfloat fovy, GLfloat aspect, GLfloat zNear, GLfloat zFar){
            _matrixMode(PROJECTION_MATRIX);
            _projectionMatrix.top() = glm::perspective(fovy,aspect,zNear,zFar);
            GL_API::getInstance().updateProjectionMatrix();
        }

        void _anaglyph(GLfloat IOD, GLdouble zNear, GLdouble zFar, GLfloat aspect, GLfloat fovy, bool rightFrustum){
            if(!FLOAT_COMPARE(_anaglyphIOD,IOD)){
                static const GLdouble DTR = 0.0174532925;
                static const GLdouble screenZ = 10.0;

                //sets top of frustum based on fovy and near clipping plane
                GLdouble top = zNear*tan(DTR*fovy/2);
                GLdouble right = aspect*top;
                //sets right of frustum based on aspect ratio
                GLdouble frustumshift = (IOD/2)*zNear/screenZ;

                _leftCam.topfrustum = top;
                _leftCam.bottomfrustum = -top;
                _leftCam.leftfrustum = -right + frustumshift;
                _leftCam.rightfrustum = right + frustumshift;
                _leftCam.modeltranslation = IOD/2;

                _rightCam.topfrustum = top;
                _rightCam.bottomfrustum = -top;
                _rightCam.leftfrustum = -right - frustumshift;
                _rightCam.rightfrustum = right - frustumshift;
                _rightCam.modeltranslation = -IOD/2;

                _anaglyphIOD = IOD;
            }

            _matrixMode(PROJECTION_MATRIX);

            CameraFrustum& tempCamera = rightFrustum ? _rightCam : _leftCam;

            _projectionMatrix.top() = glm::frustum(tempCamera.leftfrustum,
                                                   tempCamera.rightfrustum,
                                                   tempCamera.bottomfrustum,
                                                   tempCamera.topfrustum,
                                                   zNear,
                                                   zFar);
            //translate to cancel parallax
            glm::translate(_projectionMatrix.top(), glm::vec3(tempCamera.modeltranslation, 0.0, 0.0));

            GL_API::getInstance().updateProjectionMatrix();
  
            _matrixMode(VIEW_MATRIX);
        }

        void _pushMatrix(){
            if(_currentMatrixMode == PROJECTION_MATRIX) {
                _projectionMatrix.push(_projectionMatrix.top());
            }else if(_currentMatrixMode == VIEW_MATRIX){
                _viewMatrix.push(_viewMatrix.top());
                _currentViewDirection.push(_currentViewDirection.top());
            }else if(_currentMatrixMode == TEXTURE_MATRIX) {
                _textureMatrix.push(_textureMatrix.top());
            }else {
                assert(_currentMatrixMode == -1);
            }
        }

        void _popMatrix(){
            if(_currentMatrixMode == PROJECTION_MATRIX){
                _projectionMatrix.pop();
                GL_API::getInstance().updateProjectionMatrix();
            }else if(_currentMatrixMode == VIEW_MATRIX){
                _viewMatrix.pop();
                _currentViewDirection.pop();
                GL_API::getInstance().updateViewMatrix();
            }else if(_currentMatrixMode == TEXTURE_MATRIX){
                _textureMatrix.pop();
            }else{
                assert(_currentMatrixMode == -1);
            }
        }

        void _loadIdentity(){
            if(_currentMatrixMode == PROJECTION_MATRIX){
                _projectionMatrix.top() = _identityMatrix;
                GL_API::getInstance().updateProjectionMatrix();
            }else if(_currentMatrixMode == VIEW_MATRIX){
                _viewMatrix.top() = _identityMatrix;
                _currentViewDirection.top() = vec3<GLfloat>();
                GL_API::getInstance().updateViewMatrix();
            }else if(_currentMatrixMode == TEXTURE_MATRIX){
                _textureMatrix.top() = _identityMatrix;
            }else{
                assert(_currentMatrixMode == -1);
            }
        }
    }//namespace GL
}// namespace Divide