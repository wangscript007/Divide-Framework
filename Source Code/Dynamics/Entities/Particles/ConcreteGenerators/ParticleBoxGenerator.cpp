#include "Headers/ParticleBoxGenerator.h"

namespace Divide {

void ParticleBoxGenerator::generate(const U64 deltaTime,
                                    std::shared_ptr<ParticleData> p,
                                    U32 startIndex,
                                    U32 endIndex) {
    vec3<F32> posMin(_pos.xyz() - _maxStartPosOffset.xyz());
    vec3<F32> posMax(_pos.xyz() + _maxStartPosOffset.xyz());

    for (U32 i = startIndex; i < endIndex; ++i) {
        p->_position[i].xyz(Random(posMin, posMax));
    }
}
};