/*
Copyright (c) 2018 DIVIDE-Studio
Copyright (c) 2009 Ionut Cava

This file is part of DIVIDE Framework.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software
and associated documentation files (the "Software"), to deal in the Software
without restriction,
including without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the Software
is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#pragma once
#ifndef _UNIT_COMPONENT_H_
#define _UNIT_COMPONENT_H_

#include "SGNComponent.h"

namespace Divide {

FWD_DECLARE_MANAGED_CLASS(Unit);
class UnitComponent final : public BaseComponentType<UnitComponent, ComponentType::UNIT> {
public:
    UnitComponent(SceneGraphNode& parentSGN, PlatformContext& context);
    ~UnitComponent();

    // This call will take ownership of the specified pointer!
    bool setUnit(Unit_ptr unit);

    template <typename T = Unit>
    inline eastl::shared_ptr<T> getUnit() const {
        static_assert(std::is_base_of<Unit, T>::value,
            "UnitComponent::getUnit error: Invalid target unit type!");
        return eastl::static_pointer_cast<T>(_unit);
    }

private:
    Unit_ptr _unit;
};

INIT_COMPONENT(UnitComponent);
}; //namespace Divide

#endif //_UNIT_COMPONENT_H_