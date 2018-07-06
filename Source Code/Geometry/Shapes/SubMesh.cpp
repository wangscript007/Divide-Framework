#include "stdafx.h"

#include "Headers/SubMesh.h"
#include "Headers/Mesh.h"

#include "Core/Resources/Headers/ResourceCache.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Dynamics/Entities/Headers/Impostor.h"

namespace Divide {

SubMesh::SubMesh(GFXDevice& context, ResourceCache& parentCache, size_t descriptorHash, const stringImpl& name, ObjectFlag flag)
    : Object3D(context,
               parentCache,
               descriptorHash,
               name,
               ObjectType::SUBMESH,
               to_U32(flag) | to_base(ObjectFlag::OBJECT_FLAG_NO_VB)),
      _visibleToNetwork(true),
      _render(true),
      _ID(0),
      _parentMesh(nullptr)
{
}

SubMesh::~SubMesh()
{
}

void SubMesh::buildDrawCommands(SceneGraphNode& sgn,
                                     const RenderStagePass& renderStagePass,
                                     RenderPackage& pkgInOut) {

    GenericDrawCommand cmd(PrimitiveType::TRIANGLES,
        getGeometryVB()->getPartitionOffset(_geometryPartitionID),
        getGeometryVB()->getPartitionIndexCount(_geometryPartitionID));

    cmd.sourceBuffer(_parentMesh->getGeometryVB());

    GFX::DrawCommand drawCommand;
    drawCommand._drawCommands.push_back(cmd);
    pkgInOut.addDrawCommand(drawCommand);

    Object3D::buildDrawCommands(sgn, renderStagePass, pkgInOut);
}

void SubMesh::setParentMesh(Mesh* const parentMesh) {
    _parentMesh = parentMesh;
    setGeometryVB(_parentMesh->getGeometryVB());
}

};