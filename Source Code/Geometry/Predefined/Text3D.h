/*�Copyright 2009-2011 DIVIDE-Studio�*/
/* This file is part of DIVIDE Framework.

   DIVIDE Framework is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   DIVIDE Framework is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with DIVIDE Framework.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _TEXT_3D_H_
#define _TEXT_3D_H_
#include "Geometry/Object3D.h"

class Text3D : public Object3D
{
public:
	Text3D(const std::string& text) :  Object3D(TEXT_3D),
									  _text(text),
									  _font(((void *)0x0000)/*GLUT_STROKE_ROMAN*/)
									  {}
	
	bool load(const std::string &name) {_name = name; _text = name; return true;}

	inline std::string&  getText()    {return _text;}
	inline void*		 getFont()    {return _font;}
	inline F32&			 getWidth()   {return _width;}

	virtual bool computeBoundingBox(SceneGraphNode* node){
		if(node->getBoundingBox().isComputed()) return true;
		vec3 min(-_width*2,0,-_width*0.5f);
		vec3 max(_width*1.5f*_text.length()*10,_width*_text.length()*1.5f,_width*0.5f);
		node->getBoundingBox().set(min,max);
		return SceneNode::computeBoundingBox(node);
	}

private:
	std::string _text;
	void* _font;
	F32   _width;
};


#endif