#include "stdafx.h"

#include "Headers/CommandBuffer.h"
#include "Core/Headers/Console.h"
#include "Platform/Video/Buffers/VertexBuffer/Headers/VertexDataInterface.h"
#include <boost/poly_collection/algorithm.hpp>

namespace Divide {

namespace GFX {

DEFINE_POOL(BindPipelineCommand, 4096);
DEFINE_POOL(SendPushConstantsCommand, 4096);
DEFINE_POOL(DrawCommand, 4096);
DEFINE_POOL(SetViewportCommand, 4096);
DEFINE_POOL(BeginRenderPassCommand, 4096);
DEFINE_POOL(EndRenderPassCommand, 4096);
DEFINE_POOL(BeginPixelBufferCommand, 4096);
DEFINE_POOL(EndPixelBufferCommand, 4096);
DEFINE_POOL(BeginRenderSubPassCommand, 4096);
DEFINE_POOL(EndRenderSubPassCommand, 4096);
DEFINE_POOL(BlitRenderTargetCommand, 4096);
DEFINE_POOL(ResetRenderTargetCommand, 4096);
DEFINE_POOL(ComputeMipMapsCommand, 2048);
DEFINE_POOL(SetScissorCommand, 4096);
DEFINE_POOL(SetBlendCommand, 4096);
DEFINE_POOL(SetCameraCommand, 4096);
DEFINE_POOL(SetClipPlanesCommand, 4096);
DEFINE_POOL(BindDescriptorSetsCommand, 4096);
DEFINE_POOL(BeginDebugScopeCommand, 4096);
DEFINE_POOL(EndDebugScopeCommand, 4096);
DEFINE_POOL(DrawTextCommand, 4096);
DEFINE_POOL(DrawIMGUICommand, 4096);
DEFINE_POOL(DispatchComputeCommand, 1024);
DEFINE_POOL(MemoryBarrierCommand, 1024);
DEFINE_POOL(ReadAtomicCounterCommand, 1024);
DEFINE_POOL(ExternalCommand, 4096);

void CommandBuffer::add(const CommandBuffer& other) {
    _commandOrder.reserve(_commandOrder.size() + other._commandOrder.size());
    for (const CommandEntry& cmd : other._commandOrder) {
        other.getCommand<CommandBase>(cmd).addToBuffer(*this);
    }
}

void CommandBuffer::batch() {
    clean();

    std::array<CommandBase*, to_base(GFX::CommandType::COUNT)> prevCommands;
    vectorEASTL<CommandEntry>::iterator it;

    bool tryMerge = true;

    bool partial = false;
    while (tryMerge) {
        prevCommands.fill(nullptr);
        tryMerge = false;
        bool skip = false;
        for (it = std::begin(_commandOrder); it != std::cend(_commandOrder);) {
            skip = false;

            const CommandEntry& cmd = *it;
            CommandBase* crtCommand = getCommandInternal<CommandBase>(cmd);
            CommandBase*& prevCommand = prevCommands[to_U16(cmd.type<GFX::CommandType::_enumerated>())];
            
            assert(prevCommand == nullptr || prevCommand->_type._value == cmd.type<GFX::CommandType::_enumerated>());

            if (prevCommand != nullptr && tryMergeCommands(prevCommand, crtCommand, partial)) {
                it = _commandOrder.erase(it);
                skip = true;
                tryMerge = true;
            } else {
                prevCommands.fill(nullptr);
            }
            

            if (!skip) {
                prevCommand = crtCommand;
                ++it;
            }
        }
    }
    
    // If we don't have any actual work to do, clear everything
    bool hasWork = false;
    for (const CommandEntry& cmd : _commandOrder) {
        if (hasWork) {
            break;
        }

        switch (cmd.type<GFX::CommandType::_enumerated>()) {
            case GFX::CommandType::BEGIN_RENDER_PASS: {
                // We may just wish to clear the RT
                const GFX::BeginRenderPassCommand& crtCmd = getCommand<GFX::BeginRenderPassCommand>(cmd);
                if (crtCmd._descriptor.stateMask() != 0) {
                    hasWork = true;
                    break;
                }
            } break;
            case GFX::CommandType::READ_ATOMIC_COUNTER:
            case GFX::CommandType::DISPATCH_COMPUTE:
            case GFX::CommandType::MEMORY_BARRIER:
            case GFX::CommandType::DRAW_TEXT:
            case GFX::CommandType::DRAW_COMMANDS:
            case GFX::CommandType::DRAW_IMGUI:
            //case GFX::CommandType::BIND_DESCRIPTOR_SETS:
            //case GFX::CommandType::BIND_PIPELINE:
            case GFX::CommandType::BLIT_RT:
            case GFX::CommandType::RESET_RT:
            case GFX::CommandType::SEND_PUSH_CONSTANTS:
            case GFX::CommandType::BEGIN_PIXEL_BUFFER:
            case GFX::CommandType::SET_CAMERA:
            case GFX::CommandType::SET_CLIP_PLANES:
            case GFX::CommandType::SET_SCISSOR:
            case GFX::CommandType::SET_BLEND:
            case GFX::CommandType::SET_VIEWPORT:
            case GFX::CommandType::SWITCH_WINDOW: {
                hasWork = true;
                break;
            }break;
                
        };
    }

    if (!hasWork) {
        _commandOrder.clear();
        return;
    }
}

void CommandBuffer::clean() {
    if (_commandOrder.empty()) {
        return;
    }

    bool skip = false;
    const Pipeline* prevPipeline = nullptr;
    const DescriptorSet* prevDescriptorSet = nullptr;

    vectorEASTL<CommandEntry>::iterator it;
    for (it = std::begin(_commandOrder); it != std::cend(_commandOrder);) {
        skip = false;
        const CommandEntry& cmd = *it;

        switch (cmd.type<GFX::CommandType::_enumerated>()) {
            case CommandType::DRAW_COMMANDS :
            {
                vectorEASTL<GenericDrawCommand>& cmds = getCommandInternal<DrawCommand>(cmd)->_drawCommands;

                auto beginIt = eastl::begin(cmds);
                auto endIt = eastl::end(cmds);
                cmds.erase(eastl::remove_if(beginIt, endIt, [](const GenericDrawCommand& cmd) -> bool { return cmd._drawCount == 0; }), endIt);
                if (cmds.empty()) {
                    it = _commandOrder.erase(it);
                    skip = true;
                }
            } break;
            case CommandType::BIND_PIPELINE : {
                const Pipeline* pipeline = getCommandInternal<BindPipelineCommand>(cmd)->_pipeline;
                // If the current pipeline is identical to the previous one, remove it
                if (prevPipeline != nullptr && *prevPipeline == *pipeline) {
                    it = _commandOrder.erase(it);
                    skip = true;
                }
                prevPipeline = pipeline;
            }break;
            case GFX::CommandType::SEND_PUSH_CONSTANTS: {
                PushConstants& constants = getCommandInternal<SendPushConstantsCommand>(cmd)->_constants;
                if (constants.data().empty()) {
                    it = _commandOrder.erase(it);
                    skip = true;
                }
            }break;
            case GFX::CommandType::BIND_DESCRIPTOR_SETS: {
                const DescriptorSet& set = getCommandInternal<BindDescriptorSetsCommand>(cmd)->_set;
                if (prevDescriptorSet != nullptr && *prevDescriptorSet == set) {
                    it = _commandOrder.erase(it);
                    skip = true;
                }
                if (!skip && set._shaderBuffers.empty() && set._textureData.textures().empty() && set._textureViews.empty()) {
                    it = _commandOrder.erase(it);
                    skip = true;
                } else {
                    prevDescriptorSet = &set;
                }
            }break;
            case GFX::CommandType::DRAW_TEXT: {
                const TextElementBatch& batch = getCommandInternal<DrawTextCommand>(cmd)->_batch;
                bool hasText = !batch._data.empty();
                if (hasText) {
                    hasText = false;
                    for (const TextElement& element : batch()) {
                        hasText = hasText || !element.text().empty();
                    }
                }
                if (!hasText) {
                    it = _commandOrder.erase(it);
                    skip = true;
                }
            }break;
            default: break;
        };


        if (!skip) {
            ++it;
        }
    }

    // Remove redundant pipeline changes
    vec_size size = _commandOrder.size();

    vector<vec_size> redundantEntries;
    redundantEntries.reserve(size);
    for (vec_size i = 1; i < size; ++i) {
        if (_commandOrder[i - 1].type<GFX::CommandType::_enumerated>() == _commandOrder[i].type<GFX::CommandType::_enumerated>() &&
            _commandOrder[i].type<GFX::CommandType::_enumerated>() == CommandType::BIND_PIPELINE) {
            redundantEntries.push_back(i - 1);
        }
    }

    if (!redundantEntries.empty()) {
        _commandOrder = erase_sorted_indices(_commandOrder, redundantEntries);
    }
}

// New use cases that emerge from production work should be checked here.
bool CommandBuffer::validate() const {
    bool valid = true;

    if (Config::ENABLE_GPU_VALIDATION) {
        bool pushedPass = false, pushedSubPass = false, pushedPixelBuffer = false;
        bool hasPipeline = false, needsPipeline = false;
        bool hasDescriptorSets = false, needsDescriptorSets = false;
        U32 pushedDebugScope = 0;

        for (const CommandEntry& cmd : _commandOrder) {
            switch (cmd.type<GFX::CommandType::_enumerated>()) {
                case GFX::CommandType::BEGIN_RENDER_PASS: {
                    if (pushedPass) {
                        return false;
                    }
                    pushedPass = true;
                } break;
                case GFX::CommandType::END_RENDER_PASS: {
                    if (!pushedPass) {
                        return false;
                    }
                    pushedPass = false;
                } break;
                case GFX::CommandType::BEGIN_RENDER_SUB_PASS: {
                    if (pushedSubPass) {
                        return false;
                    }
                    pushedSubPass = true;
                } break;
                case GFX::CommandType::END_RENDER_SUB_PASS: {
                    if (!pushedSubPass) {
                        return false;
                    }
                    pushedSubPass = false;
                } break;
                case GFX::CommandType::BEGIN_DEBUG_SCOPE: {
                    ++pushedDebugScope;
                } break;
                case GFX::CommandType::END_DEBUG_SCOPE: {
                    if (pushedDebugScope == 0) {
                        return false;
                    }
                    --pushedDebugScope;
                } break;
                case GFX::CommandType::BEGIN_PIXEL_BUFFER: {
                    if (pushedPixelBuffer) {
                        return false;
                    }
                    pushedPixelBuffer = true;
                }break;
                case GFX::CommandType::END_PIXEL_BUFFER: {
                    if (!pushedPixelBuffer) {
                        return false;
                    }
                    pushedPixelBuffer = false;
                }break;
                case GFX::CommandType::BIND_PIPELINE: {
                    hasPipeline = true;
                } break;
                case GFX::CommandType::DISPATCH_COMPUTE: 
                case GFX::CommandType::DRAW_TEXT:
                case GFX::CommandType::DRAW_IMGUI:
                case GFX::CommandType::DRAW_COMMANDS: {
                    needsPipeline = true;
                }break;
                case GFX::CommandType::BIND_DESCRIPTOR_SETS: {
                    hasDescriptorSets = true;
                }break;
                case GFX::CommandType::BLIT_RT: {
                    needsDescriptorSets = true;
                }break;
                default: {
                    // no requirements yet
                }break;
            };
        }

        valid = !pushedPass && !pushedSubPass && !pushedPixelBuffer && pushedDebugScope == 0;

        if (needsDescriptorSets) {
            valid = hasDescriptorSets && valid;
        }

        if (needsPipeline && !hasPipeline) {
            valid = false;
        }
    }

    return valid;
}

void CommandBuffer::toString(const GFX::CommandBase& cmd, I32& crtIndent, stringImpl& out) const {
    auto append = [](stringImpl& target, const stringImpl& text, I32 indent) {
        for (I32 i = 0; i < indent; ++i) {
            target.append("    ");
        }
        target.append(text);
    };

    switch (cmd._type) {
        case GFX::CommandType::BEGIN_RENDER_PASS:
        case GFX::CommandType::BEGIN_PIXEL_BUFFER:
        case GFX::CommandType::BEGIN_RENDER_SUB_PASS: 
        case GFX::CommandType::BEGIN_DEBUG_SCOPE : {
            append(out, cmd.toString(to_U16(crtIndent)), crtIndent);
            ++crtIndent;
        }break;
        case GFX::CommandType::END_RENDER_PASS:
        case GFX::CommandType::END_PIXEL_BUFFER:
        case GFX::CommandType::END_RENDER_SUB_PASS:
        case GFX::CommandType::END_DEBUG_SCOPE: {
            --crtIndent;
            append(out, cmd.toString(to_U16(crtIndent)), crtIndent);
        }break;
        default: {
            append(out, cmd.toString(to_U16(crtIndent)), crtIndent);
        }break;
    }
}

stringImpl CommandBuffer::toString() const {
    I32 crtIndent = 0;
    stringImpl out = "\n\n\n\n";
    for (const CommandEntry& cmd : _commandOrder) {
        toString(getCommand<CommandBase>(cmd), crtIndent, out);
        out.append("\n");
    }
    out.append("\n\n\n\n");

    assert(crtIndent == 0);

    return out;
}

}; //namespace GFX
}; //namespace Divide