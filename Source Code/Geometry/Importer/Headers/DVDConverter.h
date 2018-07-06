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

#ifndef _DIVIDE_FORMAT_CONVERTER_H_
#define _DIVIDE_FORMAT_CONVERTER_H_

#include "resource.h"
#include "Geometry/Shapes/Headers/Mesh.h"

struct aiScene;
struct aiMesh;
struct aiMaterial;
DEFINE_SINGLETON( DVDConverter )

public:
    Mesh* load(const std::string& file);
private:
	SubMesh* loadSubMeshGeometry(aiMesh* source, U8 count);
	Material* loadSubMeshMaterial(aiMaterial* source, const std::string& materialName);
private:
	const aiScene* scene;
	U32   _ppsteps;
	std::string _fileLocation;
	std::string _modelName;

END_SINGLETON

#endif