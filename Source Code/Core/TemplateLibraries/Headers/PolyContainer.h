/*
Copyright (c) 2018 DIVIDE-Studio
Copyright (c) 2009 Ionut Cava

This file is part of DIVIDE Framework.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software
and associated documentation files (the "Software"), to deal in the Software
without restriction,
including without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
IN CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#pragma once
#ifndef _POLY_CONTAINER_H_
#define _POLY_CONTAINER_H_

namespace Divide {

#pragma pack(push, 1)
struct PolyContainerEntry
{
    U8 _typeIndex = 0;
    I24 _elementIndex = 0;
};
#pragma pack(pop)

template<typename T>
using deleted_unique_ptr = std::unique_ptr<T, std::function<void(T*)>>;

template<typename T, U8 N>
struct PolyContainer {
    typedef vectorEASTLFast<deleted_unique_ptr<T>> EntryList;

    template<typename U>
    inline typename std::enable_if<std::is_base_of<T, U>::value, PolyContainerEntry>::type
        insert(U8 index, deleted_unique_ptr<T>&& cmd) {
        assert(index < N);

        EntryList& collection = _collection[index];
        collection.push_back(std::move(cmd));

        return PolyContainerEntry{ index, to_I32(collection.size() - 1) };
    }

    inline EntryList& get(U8 index) {
        return  _collection[index];
    }

    inline const EntryList& get(U8 index) const {
        return  _collection[index];
    }

    inline T& get(U8 index, I24 entry) {
        assert(index < N);

        const EntryList& collection = _collection[index];
        assert(entry < collection.size());

        return *collection[to_I32(entry)];
    }

    inline T* getPtr(U8 index, I24 entry) const {
        assert(index < N);

        const EntryList& collection = _collection[index];
        if (entry < collection.size()) {
            return collection[to_I32(entry)].get();
        }
        
        return nullptr;
    }

    inline const T& get(U8 index, I24 entry) const {
        assert(index < N);

        const EntryList& collection = _collection[index];
        assert(entry < collection.size());

        return *collection[to_I32(entry)];
    }

    inline T& get(const PolyContainerEntry& entry) {
        return get(entry._typeIndex, entry._elementIndex);
    }

    inline const T& get(const PolyContainerEntry& entry) const {
        return get(entry._typeIndex, entry._elementIndex);
    }

    inline bool exists(const PolyContainerEntry& entry) const {
        return exists(entry._typeIndex, entry._elementIndex);
    }

    inline bool exists(U8 index, I24 entry) const {
        return index < N && entry < _collection[index].size();
    }

    inline vec_size_eastl size(U8 index) const {
        assert(index < N);

        return _collection[index].size();
    }

    inline void reserve(size_t size) {
        for (auto& col : _collection) {
            col.reserve(size);
        }
    }

    inline void reserve(U8 index, size_t reserveSize) {
        assert(index < N);

        _collection[index].reserve(reserveSize);
    }

    inline void clear(bool clearMemory = false) {
        if (clearMemory) {
            for (auto& col : _collection) {
                col.clear();
            }
        } else {
            for (auto& col : _collection) {
                col.resize(0);
            }
        }
    }

    inline void nuke() {
        for (auto& col : _collection) {
            auto size = col.size();
            col.reset_lose_memory();
            col.reserve(size);
        }
    }

    inline void clear(U8 index, bool clearMemory = false) {
        assert(index < N);

        if (clearMemory) {
            _collection[index].clear();
        } else {
            _collection[index].resize(0);
        }
    }

    inline bool empty() const {
        for (auto col : _collection) {
            if (!col.empty()) {
                return false;
            }
        }

        return true;
    }

    std::array<EntryList, N> _collection;
};

}; //namespace Divide
#endif //_POLY_CONTAINER_H_