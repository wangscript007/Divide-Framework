#include "Headers/DXWrapper.h"

#include "GUI/Headers/GUI.h"
#include "Core/Headers/Console.h"
#include "Core/Headers/Application.h"
#include "Utility/Headers/Localization.h"
#include "Geometry/Shapes/Headers/SubMesh.h"
#include "Geometry/Shapes/Headers/Predefined/Box3D.h"
#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Geometry/Shapes/Headers/Predefined/Text3D.h"

namespace Divide {

ErrorCode DX_API::initRenderingAPI(const vec2<U16>& resolution, I32 argc,
                                   char** argv) {
    Console::printfn(Locale::get("START_D3D_API"));
    D3D_ENUM_TABLE::fill();
    // CEGUI::System::create(CEGUI::Direct3D10Renderer::create(
    // /*myD3D10Device*/nullptr ));
    return ErrorCode::DX_INIT_ERROR;
}

void DX_API::closeRenderingAPI() {}

void DX_API::changeResolution(U16 w, U16 h) {}

void DX_API::changeViewport(const vec4<I32>& newViewport) const {}

void DX_API::uploadDrawCommands(
    const vectorImpl<IndirectDrawCommand>& drawCommands) const {}

void DX_API::setWindowPos(U16 w, U16 h) const {}

void DX_API::setCursorPosition(U16 x, U16 y) const {}

void DX_API::beginFrame() {}

void DX_API::endFrame() {}

void DX_API::toggleRasterization(bool state) {}

void DX_API::setLineWidth(F32 width) {}

void DX_API::updateClipPlanes() {}

void DX_API::drawText(const TextLabel& textLabel, const vec2<I32>& position) {}

void DX_API::drawPoints(U32 numPoints) {}

bool DX_API::initShaders() { return true; }

bool DX_API::deInitShaders() { return true; }

void DX_API::threadedLoadCallback() {}

void DX_API::activateStateBlock(const RenderStateBlock& newBlock,
                                RenderStateBlock* const oldBlock) const {}
};