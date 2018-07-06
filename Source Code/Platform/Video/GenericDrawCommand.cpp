#include "Headers/GenericDrawCommand.h"

#include "Core/Math/Headers/MathHelper.h"
#include "Platform/Video/Headers/RenderAPIEnums.h"

namespace Divide {

IndirectDrawCommand::IndirectDrawCommand()
    : indexCount(0),
      primCount(1),
      firstIndex(0),
      baseVertex(0),
      baseInstance(0)
{
}

void IndirectDrawCommand::set(const IndirectDrawCommand& other) {
    indexCount = other.indexCount;
    primCount = other.primCount;
    firstIndex = other.firstIndex;
    baseVertex = other.baseVertex;
    baseInstance = other.baseInstance;
}

bool IndirectDrawCommand::operator==(const IndirectDrawCommand &other) const {
    return indexCount == other.indexCount &&
           primCount == other.primCount &&
           firstIndex == other.firstIndex &&
           baseVertex == other.baseVertex &&
           baseInstance == other.baseInstance;
}

GenericDrawCommand::GenericDrawCommand()
    : GenericDrawCommand(PrimitiveType::TRIANGLE_STRIP, 0, 0)
{
}

GenericDrawCommand::GenericDrawCommand(PrimitiveType type,
                                       U32 firstIndex,
                                       U32 indexCount,
                                       U32 primCount)
  : _lodIndex(0),
    _drawCount(1),
    _drawToBuffer(0),
    _stateHash(0),
    _type(type),
    _commandOffset(0),
    _shaderProgram(nullptr),
    _sourceBuffer(nullptr),
    _renderOptions(to_const_uint(RenderOptions::RENDER_GEOMETRY))
{
    _cmd.indexCount = indexCount;
    _cmd.firstIndex = firstIndex;
    _cmd.primCount = primCount;

    static_assert(sizeof(IndirectDrawCommand) == 20, "Size of IndirectDrawCommand is incorrect!");
    static_assert(sizeof(GenericDrawCommand) == 64, "Size of GenericDrawCommand is incorrect!");
}

void GenericDrawCommand::set(const GenericDrawCommand& base) {
    _cmd.set(base._cmd);
    _lodIndex = base._lodIndex;
    _drawCount = base._drawCount;
    _drawToBuffer = base._drawToBuffer;
    _renderOptions = base._renderOptions;
    _stateHash = base._stateHash;
    _type = base._type;
    _shaderProgram = base._shaderProgram;
    _sourceBuffer = base._sourceBuffer;
    _commandOffset = base._commandOffset;
}

void GenericDrawCommand::reset() {
    set(GenericDrawCommand());
}

bool GenericDrawCommand::compatible(const GenericDrawCommand& other) const {
    return _lodIndex == other._lodIndex &&
           _drawToBuffer == other._drawToBuffer &&
           _renderOptions == other._renderOptions &&
           _stateHash == other._stateHash && _type == other._type &&
           (_shaderProgram != nullptr) == (other._shaderProgram != nullptr) &&
           (_sourceBuffer != nullptr) == (other._sourceBuffer != nullptr);
}

void GenericDrawCommand::renderMask(U32 mask) {
    if (Config::Build::IS_DEBUG_BUILD) {
        auto validateMask = [mask]() -> U32 {
            U32 validMask = 0;
            for (U32 stateIt = 1; stateIt <= to_const_uint(RenderOptions::COUNT); ++stateIt) {
                U32 bitState = toBit(stateIt);

                if (BitCompare(mask, bitState)) {
                    SetBit(validMask, bitState);
                }
            }
            return validMask;
        };

        U32 parsedMask = validateMask();
        DIVIDE_ASSERT(parsedMask != 0 && parsedMask == mask,
                      "GenericDrawCommand::renderMask error: Invalid state specified!");
        _renderOptions = parsedMask;
    } else {
        _renderOptions = mask;
    }
}

bool GenericDrawCommand::isEnabledOption(RenderOptions option) const {
    return BitCompare(_renderOptions, to_uint(option));
}

void GenericDrawCommand::enableOption(RenderOptions option) {
    SetBit(_renderOptions, to_uint(option));
}

void GenericDrawCommand::disableOption(RenderOptions option) {
    ClearBit(_renderOptions, to_uint(option));
}

void GenericDrawCommand::toggleOption(RenderOptions option) {
    toggleOption(option, !isEnabledOption(option));
}

void GenericDrawCommand::toggleOption(RenderOptions option, const bool state) {
    if (state) {
        enableOption(option);
    } else {
        disableOption(option);
    }
}

}; //namespace Divide