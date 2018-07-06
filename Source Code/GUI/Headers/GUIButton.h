/*
   Copyright (c) 2016 DIVIDE-Studio
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

#ifndef _GUI_BUTTON_H_
#define _GUI_BUTTON_H_

#include "GUIElement.h"
#include "GUIText.h"

namespace CEGUI {
class Window;
class Font;
class EventArgs;
};

namespace Divide {

class GUIButton : public GUIElement {
    typedef DELEGATE_CBK<> ButtonCallback;
    friend class GUI;

   protected:
    GUIButton(ULL ID,
              const stringImpl& text,
              const stringImpl& guiScheme, 
              const vec2<F32>& relativeOffset,
              const vec2<F32>& relativeDimensions,
              const vec3<F32>& color,
              CEGUI::Window* parent,
              ButtonCallback callback);
    ~GUIButton();

    void draw() const;
    void setTooltip(const stringImpl& tooltipText);
    void setFont(const stringImpl& fontName, const stringImpl& fontFileName, U32 size);
    bool joystickButtonPressed(const CEGUI::EventArgs& /*e*/);

   protected:
    stringImpl _text;
    vec3<F32> _color;
    bool _pressed;
    bool _highlight;
    /// A pointer to a function to call if the button is pressed
    ButtonCallback _callbackFunction;
    CEGUI::Window* _btnWindow;
};

};  // namespace Divide

#endif