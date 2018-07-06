/*
   Copyright (c) 2014 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
   and associated documentation files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _GUI_EDITOR_AI_INTERFACE_H_
#define _GUI_EDITOR_AI_INTERFACE_H_

#include "GUIEditor.h"
namespace CEGUI{
	class ToggleButton;
};

DEFINE_SINGLETON_EXT1(GUIEditorAIInterface,GUIEditorInterface)

protected:
	friend class GUIEditor;

	GUIEditorAIInterface();
	~GUIEditorAIInterface();
	bool init(CEGUI::Window *parent);
	bool update(const U64 deltaTime);

	bool Handle_CreateNavMesh(const CEGUI::EventArgs &e);
	bool Handle_ToggleDebugDraw(const CEGUI::EventArgs &e);
private:
	bool _createNavMeshQueued;
	CEGUI::ToggleButton* _debugDrawCheckbox;

END_SINGLETON

#endif