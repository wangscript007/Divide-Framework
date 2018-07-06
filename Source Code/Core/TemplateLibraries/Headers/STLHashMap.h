/*
Copyright (c) 2015 DIVIDE-Studio
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

#ifndef _STL_HASH_MAP_H_
#define _STL_HASH_MAP_H_
#include <unordered_map>

namespace hashAlg = std;

template <typename K, typename V, typename HashFun = HashType<K> >
using hashMapImpl = std::unordered_map<K, V, HashFun>;

namespace std {

#if defined(STRING_IMP) && STRING_IMP == EASTL_IMP
    template <>
    struct hash<eastl::string> {
        size_t operator()(const eastl::string& x) const {
            return std::hash<std::string>()(x.c_str());
        }
    };
#endif

template <typename K, typename V, typename HashFun = HashType<K> >
using hashPairReturn =
    std::pair<typename hashMapImpl<K, V, HashFun>::iterator, bool>;

template <typename K, typename V, typename HashFun = HashType<K> >
inline hashPairReturn<K, V, HashFun> emplace(hashMapImpl<K, V, HashFun>& map,
    K key, const V& val) {
    return map.emplace(key, val);
}

template <typename K, typename V, typename HashFun = HashType<K> >
inline hashPairReturn<K, V, HashFun> insert(
    hashMapImpl<K, V, HashFun>& map,
    const typename hashMapImpl<K, V, HashFun>::value_type& valuePair) {
    return map.insert(valuePair);
}

}; //namespace std

#endif //_STL_HASH_MAP_H_