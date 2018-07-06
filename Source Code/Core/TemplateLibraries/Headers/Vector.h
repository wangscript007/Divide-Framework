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

#ifndef _VECTOR_H_
#define _VECTOR_H_
#include "config.h"
/// boost vectors have a completely different interface,
/// so we can't actually use them
#if defined(VECTOR_IMP) && VECTOR_IMP == BOOST_IMP
#undef VECTOR_IMP
#define VECTOR_IMP STL_IMP
#endif

#if defined(VECTOR_IMP) && VECTOR_IMP == BOOST_IMP
#include "BoostVector.h"
#elif defined(VECTOR_IMP) && VECTOR_IMP == EASTL_IMP
#include "EASTLVector.h"
#else  // defined(VECTOR_IMP) && VECTOR_IMP == STL_IMP
#include "STLVector.h"
#endif  // defined(VECTOR_IMP)

#endif
