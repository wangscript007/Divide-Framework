#include "stdafx.h"

#include "Headers/glHardwareQueryPool.h"

namespace Divide {

glHardwareQueryPool::glHardwareQueryPool()
    : _index(0)
{
}

glHardwareQueryPool::~glHardwareQueryPool()
{
    destroy();
}

void glHardwareQueryPool::init(U32 size) {
    destroy();
    U32 j = std::max(size, 1u);
    for (U32 i = 0; i < j; ++i) {
        _queryPool.emplace_back(MemoryManager_NEW glHardwareQueryRing(1, i));
    }
}

void glHardwareQueryPool::destroy() {
    MemoryManager::DELETE_VECTOR(_queryPool);
}

glHardwareQueryRing& glHardwareQueryPool::allocate() {
    glHardwareQueryRing* temp = _queryPool[++_index];
    temp->initQueries();
    return *temp;
}

void glHardwareQueryPool::deallocate(glHardwareQueryRing& query) {
    for (U32 i = 0; i < _index; ++i) {
        if (_queryPool[i]->id() == query.id()) {
            std::swap(_queryPool[i], _queryPool[_index - 1]);
            --_index;
            return;
        }
    }

    DIVIDE_UNEXPECTED_CALL("Target deallocation query not part of the pool!");
}

}; //namespace Divide