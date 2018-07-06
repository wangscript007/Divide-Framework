#include "Headers/Shader.h"
#include "Headers/ShaderProgram.h"
#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"

namespace Divide {

Shader::Shader(const stringImpl& name, const ShaderType& type,
               const bool optimise)
    : _shader(std::numeric_limits<U32>::max()),
      _name(name),
      _type(type),
      _optimise(optimise) {
    _compiled = false;
}

Shader::~Shader() {
    Console::d_printfn(Locale::get("SHADER_DELETE"), getName().c_str());
    // never delete a shader if it's still in use by a program
    assert(_parentShaderPrograms.empty());
}

/// Register the given shader program with this shader
void Shader::addParentProgram(ShaderProgram* const shaderProgram) {
    // simple, handle-base check
    U32 parentShaderID = shaderProgram->getID();
    vectorImpl<ShaderProgram*>::iterator it;
    it = std::find_if(std::begin(_parentShaderPrograms),
                      std::end(_parentShaderPrograms),
                      [&parentShaderID](ShaderProgram const* sp)
                          -> bool { return sp->getID() == parentShaderID; });
    // assert if we register the same shaderProgram with the same shader
    // multiple times
    assert(it == std::end(_parentShaderPrograms));
    // actually register the shader
    _parentShaderPrograms.push_back(shaderProgram);
}

/// Unregister the given shader program from this shader
void Shader::removeParentProgram(ShaderProgram* const shaderProgram) {
    // program matching works like in 'addParentProgram'
    U32 parentShaderID = shaderProgram->getID();

    vectorImpl<ShaderProgram*>::iterator it;
    it = std::find_if(std::begin(_parentShaderPrograms),
                      std::end(_parentShaderPrograms),
                      [&parentShaderID](ShaderProgram const* sp)
                          -> bool { return sp->getID() == parentShaderID; });
    // assert if the specified shader program wasn't registered
    assert(it != std::end(_parentShaderPrograms));
    // actually unregister the shader
    _parentShaderPrograms.erase(it);
}
};