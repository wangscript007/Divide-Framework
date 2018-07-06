#include "Headers/ParticleRoundGenerator.h"

namespace Divide {

void ParticleRoundGenerator::generate(vectorImpl<std::future<void>>& packagedTasks,
                                      const U64 deltaTime,
                                      std::shared_ptr<ParticleData> p,
                                      U32 startIndex,
                                      U32 endIndex) {

    for (U32 i = startIndex; i < endIndex; i++) {
        F32 ang = Random(0.0f, to_float(M_2PI));
        p->_position[i].xyz(_center + vec3<F32>(_radX * std::sin(ang),
                                                _radY * std::cos(ang), 
                                                0.0f));
    }
}
};