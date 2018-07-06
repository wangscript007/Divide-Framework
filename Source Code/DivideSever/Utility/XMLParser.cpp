#include "Headers/XMLParser.h"
#include "Headers/Patch.h"

namespace XML
{
	using boost::property_tree::ptree;
	ptree pt;

	void loadScene(const std::string& sceneName)
	{
		pt.clear();
		cout << "XML: Loading scene [ " << sceneName << " ] " << endl;
		read_xml("Scenes/" + sceneName + ".xml", pt);
		loadGeometry("Scenes/" + sceneName + "/" + pt.get("assets","assets.xml"));
	}

	void loadGeometry(const std::string &file)
	{
		pt.clear();
		cout << "XML: Loading Geometry: [ " << file << " ] " << endl;
		read_xml(file,pt);
		ptree::iterator it;
		for (it = pt.get_child("geometry").begin(); it != pt.get_child("geometry").end(); ++it )
		{
			string name = it->second.data();
			string format = it->first.data();
			if(format.find("<xmlcomment>") != string::npos) continue;
			FileData model;
			model.ItemName = name;
			model.ModelName  = "Assets/" + pt.get<string>(name + ".model");
			model.position.x = pt.get<F32>(name + ".position.<xmlattr>.x");
			model.position.y = pt.get<F32>(name + ".position.<xmlattr>.y");
			model.position.z = pt.get<F32>(name + ".position.<xmlattr>.z");
			model.orientation.x = pt.get<F32>(name + ".orientation.<xmlattr>.x");
			model.orientation.y = pt.get<F32>(name + ".orientation.<xmlattr>.y");
			model.orientation.z = pt.get<F32>(name + ".orientation.<xmlattr>.z");
			model.scale.x    = pt.get<F32>(name + ".scale.<xmlattr>.x");
			model.scale.y    = pt.get<F32>(name + ".scale.<xmlattr>.y");
			model.scale.z    = pt.get<F32>(name + ".scale.<xmlattr>.z");
			model.type = MESH;
			model.version = pt.get<F32>(name + ".version");
			Patch::getInstance().addGeometry(model);
		}
		for (it = pt.get_child("vegetation").begin(); it != pt.get_child("vegetation").end(); ++it )
		{
			string name = it->second.data();
			string format = it->first.data();
			if(format.find("<xmlcomment>") != string::npos) continue;
			FileData model;
			model.ItemName = name;
			model.ModelName  = "Assets/" + pt.get<string>(name + ".model");
			model.position.x = pt.get<F32>(name + ".position.<xmlattr>.x");
			model.position.y = pt.get<F32>(name + ".position.<xmlattr>.y");
			model.position.z = pt.get<F32>(name + ".position.<xmlattr>.z");
			model.orientation.x = pt.get<F32>(name + ".orientation.<xmlattr>.x");
			model.orientation.y = pt.get<F32>(name + ".orientation.<xmlattr>.y");
			model.orientation.z = pt.get<F32>(name + ".orientation.<xmlattr>.z");
			model.scale.x    = pt.get<F32>(name + ".scale.<xmlattr>.x");
			model.scale.y    = pt.get<F32>(name + ".scale.<xmlattr>.y");
			model.scale.z    = pt.get<F32>(name + ".scale.<xmlattr>.z");
			model.type = VEGETATION;
			model.version = pt.get<F32>(name + ".version");
			Patch::getInstance().addGeometry(model);
		}

		if(boost::optional<ptree &> primitives = pt.get_child_optional("primitives"))
		for (it = pt.get_child("primitives").begin(); it != pt.get_child("primitives").end(); ++it )
		{
			string name = it->second.data();
			string format = it->first.data();
			if(format.find("<xmlcomment>") != string::npos) continue;

			FileData model;
			model.ItemName = name;
			model.ModelName = pt.get<string>(name + ".model");
			model.position.x = pt.get<F32>(name + ".position.<xmlattr>.x");
			model.position.y = pt.get<F32>(name + ".position.<xmlattr>.y");
			model.position.z = pt.get<F32>(name + ".position.<xmlattr>.z");
			model.orientation.x = pt.get<F32>(name + ".orientation.<xmlattr>.x");
			model.orientation.y = pt.get<F32>(name + ".orientation.<xmlattr>.y");
			model.orientation.z = pt.get<F32>(name + ".orientation.<xmlattr>.z");
			model.scale.x    = pt.get<F32>(name + ".scale.<xmlattr>.x");
			model.scale.y    = pt.get<F32>(name + ".scale.<xmlattr>.y");
			model.scale.z    = pt.get<F32>(name + ".scale.<xmlattr>.z");
			/*Primitives don't use materials yet so we can define colors*/
			model.color.r    = pt.get<F32>(name + ".color.<xmlattr>.r");
			model.color.g    = pt.get<F32>(name + ".color.<xmlattr>.g");
			model.color.b    = pt.get<F32>(name + ".color.<xmlattr>.b");
			/*The data variable stores a float variable (not void*) that can represent anything you want*/
			/*For Text3D, it's the line width and for Box3D it's the edge length*/
			if(model.ModelName.compare("Text3D") == 0)
			{
				model.data = pt.get<F32>(name + ".lineWidth");
				model.data2 = pt.get<string>(name + ".text");
			}
			else if(model.ModelName.compare("Box3D") == 0)
				model.data = pt.get<F32>(name + ".size");

			else if(model.ModelName.compare("Sphere3D") == 0)
				model.data = pt.get<F32>(name + ".radius");
			else
				model.data = 0;

			model.type = PRIMITIVE;
			model.version = pt.get<F32>(name + ".version");
			Patch::getInstance().addGeometry(model);
		}
	}
}