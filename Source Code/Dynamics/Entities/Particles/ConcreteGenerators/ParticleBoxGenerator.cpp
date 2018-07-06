#include "Headers/ParticleBoxGenerator.h"

namespace Divide {

void ParticleBoxGenerator::generate(const U64 deltaTime, ParticleData *p,
                                    U32 startIndex, U32 endIndex) {
    DIVIDE_ASSERT(
        IS_VALID_CONTAINER_RANGE(static_cast<U32>(p->_position.size()),
                                 startIndex, endIndex),
        "ParticleBoxGenerator::generate error: Invalid Range!");

    vec4<F32> posMin(_pos.x - _maxStartPosOffset.x,
                     _pos.y - _maxStartPosOffset.y,
                     _pos.z - _maxStartPosOffset.z, 1.0);

    vec4<F32> posMax(_pos.x + _maxStartPosOffset.x,
                     _pos.y + _maxStartPosOffset.y,
                     _pos.z + _maxStartPosOffset.z, 1.0);

    for (U32 i = startIndex; i < endIndex; ++i) {
        p->_position[i].set(random(posMin, posMax));
    }
}
};