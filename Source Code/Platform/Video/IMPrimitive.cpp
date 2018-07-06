#include "Headers/IMPrimitive.h"
#include "Platform/Video/Textures/Headers/Texture.h"

namespace Divide {

IMPrimitive::IMPrimitive()
    : GUIDWrapper(),
      _inUse(false),
      _hasLines(false),
      _canZombify(true),
      _forceWireframe(false),
      _paused(false),
      _zombieCounter(0),
      _lineWidth(1.0f),
      _texture(nullptr),
      _drawShader(nullptr),
      _stateHash(0) {}

IMPrimitive::~IMPrimitive() { clear(); }
};