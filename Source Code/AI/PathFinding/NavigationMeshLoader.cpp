#include "Headers/NavigationMeshLoader.h"

#include "Core/Headers/ParamHandler.h"
#include "Managers/Headers/SceneManager.h"
#include "Geometry/Shapes/Headers/Object3D.h"
#include "Environment/Terrain/Headers/Terrain.h"
#include "Environment/Water/Headers/Water.h"

namespace Navigation {

        static vec3<F32> _minVertValue, _maxVertValue;

	NavigationMeshLoader::NavigationMeshLoader(){
	}

	NavigationMeshLoader::~NavigationMeshLoader(){
	}
	       
	char* NavigationMeshLoader::parseRow(char* buf, char* bufEnd, char* row, I32 len){

		bool cont = false;
		bool start = true;
		bool done = false;
		I32 n = 0;

		while (!done && buf < bufEnd)   {
			char c = *buf;
			buf++;
			// multirow
			switch (c)  {
				case '\\':
					cont = true; // multirow
					break;
				case '\n':
					if (start) break;
					done = true;
					break;
				case '\r':
					break;
				case '\t':
				case ' ':
					if (start) break;
				default:
					start = false;
					cont = false;
					row[n++] = c;
					if (n >= len-1)
						done = true;
					break;
			}
		}
		row[n] = '\0';
		return buf;
	}

	I32 NavigationMeshLoader::parseFace(char* row, I32* data, I32 n, I32 vcnt) {
		I32 j = 0;
		while (*row != '\0')  {
			// Skip initial white space
			while (*row != '\0' && (*row == ' ' || *row == '\t'))
				row++;
			char* s = row;
			// Find vertex delimiter and terminated the string there for conversion.
			while (*row != '\0' && *row != ' ' && *row != '\t')  {
				if (*row == '/') *row = '\0';
				row++;
			}
			if (*s == '\0')
				continue;
			I32 vi = atoi(s);
			data[j++] = vi < 0 ? vi+vcnt : vi-1;
			if (j >= n) return j;
		}
		return j;
	}

	NavModelData NavigationMeshLoader::loadMeshFile(const char* filename) {

		NavModelData modelData;

		char* buf = 0;
		FILE* fp = fopen(filename, "rb");
		if (!fp)
			return modelData;
        modelData.setName(filename);
		fseek(fp, 0, SEEK_END);
		I32 bufSize = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		buf = new char[bufSize];
		if (!buf)  {
			fclose(fp);
			return modelData;
		}
		fread(buf, bufSize, 1, fp);
		fclose(fp);

		char* src = buf;
		char* srcEnd = buf + bufSize;
		char row[512];
		I32 face[32];
		F32 x,y,z;
		I32 nv;
		//I32 vcap = 0;
		//I32 tcap = 0;
	    
		while (src < srcEnd)   {
			// Parse one row
			row[0] = '\0';
			src = parseRow(src, srcEnd, row, sizeof(row)/sizeof(char));
			// Skip comments
			if (row[0] == '#') continue;
			if (row[0] == 'v' && row[1] != 'n' && row[1] != 't') {
				// Vertex pos
				sscanf(row+1, "%f %f %f", &x, &y, &z);
				addVertex(&modelData, vec3<F32>(x, y, z));
			}
			if (row[0] == 'f')  {
				// Faces
				nv = parseFace(row+1, face, 32, modelData.vert_ct);
				for (I32 i = 2; i < nv; ++i)
				{
					const I32 a = face[0];
					const I32 b = face[i-1];
					const I32 c = face[i];
					if (a < 0 || a >= (I32)modelData.vert_ct || b < 0 || b >= (I32)modelData.vert_ct || c < 0 || c >= (I32)modelData.vert_ct)
						continue;
					addTriangle(&modelData, vec3<U32>(a, b, c));
				}
			}
		}

		SAFE_DELETE_ARRAY(buf);

		// Calculate normals.
		modelData.normals = new F32[modelData.tri_ct*3];
		for (I32 i = 0; i < (I32)modelData.tri_ct*3; i += 3)   {
			const F32* v0 = &modelData.verts[modelData.tris[i]*3];
			const F32* v1 = &modelData.verts[modelData.tris[i+1]*3];
			const F32* v2 = &modelData.verts[modelData.tris[i+2]*3];
			F32 e0[3], e1[3];
			for (I32 j = 0; j < 3; ++j)  {
				e0[j] = v1[j] - v0[j];
				e1[j] = v2[j] - v0[j];
			}
			F32* n = &modelData.normals[i];
			n[0] = e0[1]*e1[2] - e0[2]*e1[1];
			n[1] = e0[2]*e1[0] - e0[0]*e1[2];
			n[2] = e0[0]*e1[1] - e0[1]*e1[0];
			F32 d = sqrtf(n[0]*n[0] + n[1]*n[1] + n[2]*n[2]);
			if (d > 0)  {
				d = 1.0f/d;
				n[0] *= d;
				n[1] *= d;
				n[2] *= d;
			}
		}
	    
		return modelData;
	}

	NavModelData NavigationMeshLoader::mergeModels(NavModelData a, NavModelData b, bool delOriginals/* = false*/) {

		NavModelData mergedData;
		if(a.verts || b.verts)  {
			if(!a.verts) {
				return b;
			}else if (! b.verts ) {
				return a;
			}
	        
			mergedData.clear();

			I32 totalVertCt = (a.vert_ct + b.vert_ct);
			I32 newCap = 8;

			while(newCap < totalVertCt)
				newCap *= 2;

			mergedData.verts = new F32[newCap*3];
			mergedData.vert_cap = newCap;
			mergedData.vert_ct = totalVertCt;
			memcpy(mergedData.verts,               a.verts, a.vert_ct*3*sizeof(F32));
			memcpy(mergedData.verts + a.vert_ct*3, b.verts, b.vert_ct*3*sizeof(F32));

			I32 totalTriCt = (a.tri_ct + b.tri_ct);
			newCap = 8;

			while(newCap < totalTriCt)
				newCap *= 2;

			mergedData.tris = new I32[newCap*3];
			mergedData.tri_cap = newCap;
			mergedData.tri_ct = totalTriCt;
			I32 aFaceSize = a.tri_ct * 3;
			memcpy(mergedData.tris, a.tris, aFaceSize*sizeof(I32));
	        
			I32 bFaceSize = b.tri_ct * 3;
			I32* bFacePt = mergedData.tris + a.tri_ct * 3;// i like pointing at faces
			memcpy(bFacePt, b.tris, bFaceSize*sizeof(I32));
	        
	        
			for(U32 i = 0; i < (U32)bFaceSize;i++)
				*(bFacePt + i) += a.vert_ct;

			if(mergedData.vert_ct > 0) {
				if(delOriginals)  {
					a.clear();
					b.clear();
				}
			}else
				mergedData.clear();
		}
        mergedData.setName(a.getName()+"+"+b.getName());
		return mergedData;
	}

	bool NavigationMeshLoader::saveModelData(NavModelData data, const char* filename, const char* activeScene/* = NULL*/) {
		if( ! data.getVertCount() || ! data.getTriCount())
			return false;
	    
        std::string path(ParamHandler::getInstance().getParam<std::string>("assetsLocation")+"/navMeshes/");
		std::string filenamestring(filename);
		//CreatePath //IsDirectory

		if(activeScene) {
			path +=  std::string(activeScene);
	        
		}

		path += std::string("/") + filenamestring + std::string(".obj");
		/// Create the file if it doesn't exists
		FILE *fp = NULL;
		fopen_s(&fp,path.c_str(),"w");
		fclose(fp);

		std::ofstream myfile;
		myfile.open(path.c_str());
		if(!myfile.is_open())
			return false;

		F32* vstart = data.verts;
		I32* tstart = data.tris;

		for(U32 i = 0; i < data.getVertCount(); i++)  {
			F32* vp = vstart + i*3;
			myfile << "v " << (*(vp)) << " " << *(vp+1) << " " << (*(vp+2)) << "\n";
		}

		for(U32 i = 0; i < data.getTriCount(); i++)  {
			I32* tp = tstart + i*3;
			myfile << "f " << *(tp) << " " << *(tp+1) << " " << *(tp+2) << "\n";
		}

		myfile.close();
		return true;
	}

	void NavigationMeshLoader::addVertex(NavModelData* modelData, const vec3<F32>& vertex){

		if (modelData->vert_ct+1 > modelData->vert_cap)  {
			modelData->vert_cap = ! modelData->vert_cap ? 8 : modelData->vert_cap*2;
			F32* nv = new F32[modelData->vert_cap*3];
			if (modelData->vert_ct)
				memcpy(nv, modelData->verts, modelData->vert_ct*3*sizeof(F32));
			if( modelData->verts )
				delete [] modelData->verts;
			modelData->verts = nv;
		}

		F32* dst = &modelData->verts[modelData->vert_ct*3];
		*dst++ = vertex.x;
		*dst++ = vertex.y;
		*dst++ = vertex.z;
		modelData->vert_ct++;

        if(vertex.x > _maxVertValue.x)	_maxVertValue.x = vertex.x;
		if(vertex.x < _minVertValue.x)	_minVertValue.x = vertex.x;
		if(vertex.y > _maxVertValue.y)	_maxVertValue.y = vertex.y;
		if(vertex.y < _minVertValue.y)	_minVertValue.y = vertex.y;
		if(vertex.z > _maxVertValue.z)	_maxVertValue.z = vertex.z;
		if(vertex.z < _minVertValue.z)	_minVertValue.z = vertex.z;

        //assert(vertex.x > -5000 && vertex.x < 5000);
        //assert(vertex.y > -5000 && vertex.y < 5000);
        //assert(vertex.z > -5000 && vertex.z < 5000);
	}

	void NavigationMeshLoader::addTriangle(NavModelData* modelData,const vec3<U32>& triangleIndices, SamplePolyAreas areaType){

		if (modelData->tri_ct+1 > modelData->tri_cap)  {
			modelData->tri_cap = !modelData->tri_cap ? 8 : modelData->tri_cap*2;
			I32* nv = new int[modelData->tri_cap*3];
			if (modelData->tri_ct)
				memcpy(nv, modelData->tris, modelData->tri_ct*3*sizeof(int));
			if( modelData->tris )
				SAFE_DELETE_ARRAY(modelData->tris);
			modelData->tris = nv;
		}

		I32* dst = &modelData->tris[modelData->tri_ct*3];
		*dst++ = triangleIndices.x;
		*dst++ = triangleIndices.y;
		*dst++ = triangleIndices.z;
        modelData->getAreaTypes().push_back(areaType); 
		modelData->tri_ct++;
	}

	NavModelData NavigationMeshLoader::parseNode(SceneGraphNode* sgn/* = NULL*/, const std::string& navMeshName/* = ""*/){
        std::string nodeName((sgn->getNode<SceneNode>()->getType() != TYPE_ROOT) ? "node [ " + sgn->getName() + " ]." : " root node.");
        PRINT_FN(Locale::get("NAV_MESH_GENERATION_START"),nodeName.c_str());
        U32 timeStart = GETMSTIME();
		NavModelData data,returnData;
		data.clear(false);
        data.setName(navMeshName.empty() ? nodeName : navMeshName);
		returnData = parse(sgn->getBoundingBox(), data, sgn);
        U32 timeEnd = GETMSTIME();
        U32 timeDiff = timeEnd - timeStart;
        if(returnData.valid){
            PRINT_FN(Locale::get("NAV_MESH_GENERATION_COMPLETE"),getMsToSec(timeDiff));
        }else{
            ERROR_FN(Locale::get("NAV_MESH_GENERATION_INCOMPLETE"),getMsToSec(timeDiff));
        }
        return returnData;
	}

	static const vec3<F32> cubePoints[8] = {

	   vec3<F32>(-1.0f, -1.0f, -1.0f), vec3<F32>(-1.0f, -1.0f,  1.0f), vec3<F32>(-1.0f,  1.0f, -1.0f), vec3<F32>(-1.0f,  1.0f,  1.0f),
	   vec3<F32>( 1.0f, -1.0f, -1.0f), vec3<F32>( 1.0f, -1.0f,  1.0f), vec3<F32>( 1.0f,  1.0f, -1.0f), vec3<F32>( 1.0f,  1.0f,  1.0f)
	};

	static const U32 cubeFaces[6][4] = {

	   { 0, 4, 6, 2 }, { 0, 2, 3, 1 }, { 0, 1, 5, 4 },
	   { 3, 2, 6, 7 }, { 7, 6, 4, 5 }, { 3, 7, 5, 1 }
	};

	NavModelData NavigationMeshLoader::parse(const BoundingBox& box,NavModelData& data, SceneGraphNode* sgn) {
        BoundingBox bb = box;
        //Ignore if specified
        if(sgn->getNavigationContext() == SceneGraphNode::NODE_IGNORE)
            goto next;

		SceneNode* sn = sgn->getNode<SceneNode>();
        SceneNodeType nodeType = sn->getType();
		assert(sn != NULL);
		if(nodeType != TYPE_WATER && nodeType != TYPE_TERRAIN && nodeType != TYPE_OBJECT3D){
               if(nodeType == TYPE_LIGHT ||
                  nodeType == TYPE_ROOT ||
                  nodeType == TYPE_PARTICLE_EMITTER ||
                  nodeType == TYPE_TRIGGER ||
                  nodeType == TYPE_SKY){
                      //handle special case
               }else{
			        PRINT_FN(Locale::get("WARN_NAV_UNSUPPORTED"),sn->getName().c_str());
			        goto next;
               }
		}

		U32 numVert = 1;
        U32 curNumVert = numVert;
		MeshDetailLevel level = DETAIL_LOW;

		switch(nodeType){
			case TYPE_WATER: {
				level = DETAIL_BOUNDINGBOX;
				 }break;
			case TYPE_TERRAIN: {
				level = DETAIL_ABSOLUTE;
				}break;
			case TYPE_OBJECT3D: {
                if(sgn->getUsageContext() == SceneGraphNode::NODE_DYNAMIC){
					level = DETAIL_BOUNDINGBOX;
				}else{
					level = DETAIL_ABSOLUTE;
				}
				}break;
			default:{
				PRINT_FN(Locale::get("WARN_NAV_INCOMPLETE"));
				goto next;
				}break;
		};
        VertexBufferObject* geometry = NULL;
        SamplePolyAreas areType = SAMPLE_AREA_OBSTACLE;

        if(level == DETAIL_ABSOLUTE){
            if(sn->getType() == TYPE_OBJECT3D){
		        geometry = dynamic_cast<Object3D* >(sn)->getGeometryVBO();
            }else if(sn->getType() == TYPE_TERRAIN){
                geometry = dynamic_cast<Terrain* >(sn)->getGeometryVBO();
                areType = SAMPLE_POLYAREA_GROUND;
            }else if(sn->getType() == TYPE_WATER){
                geometry = dynamic_cast<WaterPlane* >(sn)->getQuad()->getGeometryVBO();
                areType = SAMPLE_POLYAREA_WATER;
            }else{
                ERROR_FN(Locale::get("ERROR_NAV_UNKNOWN_NODE_FOR_ABSOLUTE"),sn->getType());
                goto next;
            }

			for (U32 i = 0; i < geometry->getPosition().size(); ++i ){
                addVertex(&data,geometry->getPosition()[i]);
                if(geometry->getPosition()[i].x < -5000 || geometry->getPosition()[i].y < -5000 || geometry->getPosition()[i].z < -5000){
                    PRINT_FN("opa ..");
                }
		    }
 			for (U32 i = 0; i < geometry->getTriangles().size(); ++i){
			    addTriangle(&data,geometry->getTriangles()[i],areType);
			}
		}else if( level == DETAIL_LOW || level == DETAIL_BOUNDINGBOX ){
           
            if(sn->getType() == TYPE_WATER){
                areType = SAMPLE_POLYAREA_WATER;
                sgn = sgn->getChildren()["waterPlane"];
            }
            mat4<F32> transform = sgn->getTransform()->getMatrix();
            //Skip small objects
            if(box.getWidth() < 0.05f || box.getDepth() < 0.05f || box.getHeight() < 0.05f)  goto next;

            vectorImpl<vec3<F32> > verts;
            for(U32 i = 0; i < 8; i++){
               verts.push_back(cubePoints[i]);
            }
        
            const vec3<F32>& boxCenter = bb.getCenter();
            const vec3<F32>& halfSize = bb.getHalfExtent();
     

            for(U32 i = 0; i < 8; i++){
                vec3<F32> half = verts[i] * halfSize;
                vec3<F32> tmp = (transform * half) + boxCenter;
                numVert++;
                addVertex(&data, tmp);
                if(tmp.x < -5000 || tmp.y < -5000 || tmp.z < -5000){
                    PRINT_FN("opa ..");
                }
            }
            for(U32 f = 0; f < 6; f++){
               for(U32 v = 2; v < 4; v++){
                   // Note: We reverse normal on the polygons to prevent things from going inside out
                      
                   addTriangle(&data,  vec3<U32>(curNumVert+cubeFaces[f][0]+1, 
                                                 curNumVert+cubeFaces[f][v]+1,
                                                 curNumVert+cubeFaces[f][v-1]+1), 
                               areType);
               }
            }
        }else if( level == DETAIL_MEDIUM || level == DETAIL_HIGH){
            ERROR_FN(Locale::get("ERROR_RECAST_MEDIUM_DETAIL"));
        }else{
            ERROR_FN(Locale::get("ERROR_RECAST_DETAIL_LEVEL"),level);
    }
    PRINT_FN("Added [ %s ] with values MAX [ %5.2f | %5.2f | %5.2f ] MIN [ %5.2f | %5.2f | %5.2f ]",
             sn->getName().c_str(), _maxVertValue.x,_maxVertValue.y,_maxVertValue.z,
                                    _minVertValue.y,_minVertValue.x,_minVertValue.z);
    //altough labels are bad, skipping here using them is the easiest solution to follow -Ionut
    next:;
	for_each(SceneGraphNode::NodeChildren::value_type& it,sgn->getChildren()){
		parse(it.second->getBoundingBox(), data, it.second);
	}
	return data;
}

};