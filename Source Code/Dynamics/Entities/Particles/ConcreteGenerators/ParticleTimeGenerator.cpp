#include "Headers/ParticleTimeGenerator.h"

namespace Divide {

void ParticleTimeGenerator::generate(const U64 deltaTime, ParticleData *p,
                                     U32 startIndex, U32 endIndex) {
    DIVIDE_ASSERT(
        IS_VALID_CONTAINER_RANGE(to_uint(p->_position.size()),
                                 startIndex, endIndex),
        "ParticleTimeGenerator::generate error: Invalid Range!");

    for (U32 i = startIndex; i < endIndex; ++i) {
        F32 time = Random(_minTime, _maxTime);
        p->_misc[i].x = time;
        p->_misc[i].y = 0.0f;
        p->_misc[i].z = time > EPSILON_F32 ? 1.0f / p->_misc[i].x : 0.0f;
    }
}
};