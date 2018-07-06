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

#ifndef _GL_RESOURCES_H_
#define _GL_RESOURCES_H_

#include <gl/glew.h>
//#define GL3_PROTOTYPES
//#include <gl/gl3.h>
//#define __gl_h_
#include <gl/freeglut.h> 
#ifdef _DEBUG
 //#define GLCheck(Func) ((Func), GLCheckError(__FILE__, __LINE__))
#define GLCheck(Func) (Func)
#else
 #define GLCheck(Func) (Func)
#endif

#endif