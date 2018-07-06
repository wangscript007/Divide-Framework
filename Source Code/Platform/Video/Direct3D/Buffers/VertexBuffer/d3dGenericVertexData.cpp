#include "Headers/d3dGenericVertexData.h"

namespace Divide {
IMPLEMENT_ALLOCATOR(d3dGenericVertexData, 0, 0);

d3dGenericVertexData::d3dGenericVertexData(GFXDevice& context, bool persistentMapped, const U32 ringBufferLength)
    : GenericVertexData(context, persistentMapped, ringBufferLength)
{
}

d3dGenericVertexData::~d3dGenericVertexData()
{
}
}; //namespace Divide