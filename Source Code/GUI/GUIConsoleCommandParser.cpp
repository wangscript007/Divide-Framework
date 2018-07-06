#include "Headers/GUIConsoleCommandParser.h"

#include "Core/Headers/ParamHandler.h"
#include "Managers/Headers/AIManager.h"
#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/ShaderManager.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "AI/PathFinding/NavMeshes/Headers/NavMesh.h" ///< For NavMesh creation

#include "Geometry/Shapes/Headers/Mesh.h"
#include "Geometry/Shapes/Headers/Predefined/Box3D.h"
#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"

GUIConsoleCommandParser::GUIConsoleCommandParser() : _sound(NULL)
{
    _commandMap.insert(std::make_pair("say",DELEGATE_BIND(&GUIConsoleCommandParser::handleSayCommand,this,_1)));
    _commandMap.insert(std::make_pair("quit",DELEGATE_BIND(&GUIConsoleCommandParser::handleQuitCommand,this,_1)));
    _commandMap.insert(std::make_pair("help",DELEGATE_BIND(&GUIConsoleCommandParser::handleHelpCommand,this,_1)));
    _commandMap.insert(std::make_pair("editparam",DELEGATE_BIND(&GUIConsoleCommandParser::handleEditParamCommand,this,_1)));
    _commandMap.insert(std::make_pair("playsound",DELEGATE_BIND(&GUIConsoleCommandParser::handlePlaySoundCommand,this,_1)));
    _commandMap.insert(std::make_pair("createnavmesh",DELEGATE_BIND(&GUIConsoleCommandParser::handleNavMeshCommand,this,_1)));
    _commandMap.insert(std::make_pair("recompileshader",DELEGATE_BIND(&GUIConsoleCommandParser::handleShaderRecompileCommand,this,_1)));
    _commandMap.insert(std::make_pair("setfov",DELEGATE_BIND(&GUIConsoleCommandParser::handleFOVCommand,this,_1)));
    _commandMap.insert(std::make_pair("invalidcommand",DELEGATE_BIND(&GUIConsoleCommandParser::handleInvalidCommand,this,_1)));
    _commandMap.insert(std::make_pair("addobject", DELEGATE_BIND(&GUIConsoleCommandParser::handleAddObject, this, _1)));

    _commandHelp.insert(std::make_pair("say",Locale::get("CONSOLE_SAY_COMMAND_HELP")));
    _commandHelp.insert(std::make_pair("quit",Locale::get("CONSOLE_QUIT_COMMAND_HELP")));
    _commandHelp.insert(std::make_pair("help",Locale::get("CONSOLE_HELP_COMMAND_HELP")));
    _commandHelp.insert(std::make_pair("editparam",Locale::get("CONSOLE_EDITPARAM_COMMAND_HELP")));
    _commandHelp.insert(std::make_pair("playsound",Locale::get("CONSOLE_PLAYSOUND_COMMAND_HELP")));
    _commandHelp.insert(std::make_pair("createnavmesh",Locale::get("CONSOLE_NAVMESH_COMMAND_HELP")));
    _commandHelp.insert(std::make_pair("recompileshader",Locale::get("CONSOLE_SHADER_RECOMPILE_COMMAND_HELP")));
    _commandHelp.insert(std::make_pair("setfov",Locale::get("CONSOLE_CHANGE_FOV_COMMAND_HELP")));
    _commandHelp.insert(std::make_pair("addObject",Locale::get("CONSOLE_ADD_OBJECT_COMMAND_HELP")));
    _commandHelp.insert(std::make_pair("invalidhelp",Locale::get("CONSOLE_INVALID_HELP_ARGUMENT")));
}

GUIConsoleCommandParser::~GUIConsoleCommandParser()
{
    if(_sound != NULL){
        RemoveResource(_sound);
    }
}

bool GUIConsoleCommandParser::processCommand(const std::string& commandString){
    // Be sure we have a string longer than 0
    if (commandString.length() >= 1) {
        // Check if the first letter is a 'command' operator
        if (commandString.at(0) == '/') 	{
            std::string::size_type commandEnd = commandString.find(" ", 1);
            std::string command = commandString.substr(1, commandEnd - 1);
            std::string commandArgs = commandString.substr(commandEnd + 1, commandString.length() - (commandEnd + 1));
            if(commandString.compare(commandArgs) == 0) commandArgs.clear();
            //convert command to lower case
            for(std::string::size_type i=0; i < command.length(); i++){
                command[i] = tolower(command[i]);
            }
            if(_commandMap.find(command) != _commandMap.end()){
                //we have a valid command
                _commandMap[command](commandArgs);
            }else{
                //invalid command
                _commandMap["invalidcommand"](command);
            }
        } else	{
            PRINT_FN("%s",commandString.c_str()); // no commands, just output what was typed
        }
    }
    return true;
}

void GUIConsoleCommandParser::handleSayCommand(const std::string& args){
    PRINT_FN(Locale::get("CONSOLE_SAY_NAME_TAG"),args.c_str());
}

void GUIConsoleCommandParser::handleQuitCommand(const std::string& args){
    if(!args.empty()){
        //quit command can take an extra argument. A reason, for example
        PRINT_FN(Locale::get("CONSOLE_QUIT_COMMAND_ARGUMENT"), args.c_str());
    }
    Application::getInstance().RequestShutdown();
}

void GUIConsoleCommandParser::handleHelpCommand(const std::string& args){
    if(args.empty()){
        PRINT_FN(Locale::get("HELP_CONSOLE_COMMAND"));
        for_each(CommandMap::value_type& it,_commandMap){
            if(it.first.find("invalid") == std::string::npos){
                PRINT_FN("/%s - %s",it.first.c_str(),_commandHelp[it.first]);
            }
        }
    }else{
        if(_commandHelp.find(args) != _commandHelp.end()){
            PRINT_FN("%s", _commandHelp[args]);
        }else{
            PRINT_FN("%s",_commandHelp["invalidhelp"]);
        }
    }
}

void GUIConsoleCommandParser::handleEditParamCommand(const std::string& args){
    if(ParamHandler::getInstance().isParam(args)){
        PRINT_FN(Locale::get("CONSOLE_EDITPARAM_FOUND"), args.c_str(), "N/A", "N/A", "N/A");
    }else{
        PRINT_FN(Locale::get("CONSOLE_EDITPARAM_NOT_FOUND"), args.c_str());
    }
}

void GUIConsoleCommandParser::handlePlaySoundCommand(const std::string& args){
    std::string filename = ParamHandler::getInstance().getParam<std::string>("assetsLocation")+"/" + args;
    std::ifstream soundfile(filename.c_str());
    if (soundfile) {
        //Check extensions (not really, musicwav.abc would still be valid, but still ...)
        if(filename.substr(filename.length()-4,filename.length()).compare(".wav") != 0 &&
           filename.substr(filename.length()-4,filename.length()).compare(".mp3") != 0 &&
           filename.substr(filename.length()-4,filename.length()).compare(".ogg") != 0){
               ERROR_FN(Locale::get("CONSOLE_PLAY_SOUND_INVALID_FORMAT"));
               return;
        }
        if(_sound != NULL) RemoveResource(_sound);

        //The file is valid, so create a descriptor for it
        ResourceDescriptor sound("consoleFilePlayback");
        sound.setResourceLocation(filename);
        sound.setFlag(false);
        _sound = CreateResource<AudioDescriptor>(sound);
        if(filename.find("music") != std::string::npos){
            //play music
            SFXDevice::getInstance().playMusic(_sound);
        }else{
            //play sound but stop music first if it's playing
            SFXDevice::getInstance().stopMusic();
            SFXDevice::getInstance().playSound(_sound);
        }
    }else{
        ERROR_FN(Locale::get("CONSOLE_PLAY_SOUND_INVALID_FILE"),filename.c_str());
    }
}

void GUIConsoleCommandParser::handleNavMeshCommand(const std::string& args){
    SceneGraphNode* sgn = NULL;
    if(!args.empty()){
        sgn = GET_ACTIVE_SCENEGRAPH()->findNode("args");
        if(!sgn){
            ERROR_FN(Locale::get("CONSOLE_NAVMESH_NO_NODE"),args.c_str());
            return;
        }
    }
    Navigation::NavigationMesh* temp = New Navigation::NavigationMesh();
    temp->setFileName(GET_ACTIVE_SCENE()->getName());
    if(!temp->load(sgn)){
        temp->build(sgn,false);
    }
    temp->save();
    AIManager::getInstance().addNavMesh(temp);
}

void GUIConsoleCommandParser::handleShaderRecompileCommand(const std::string& args){
    ShaderManager::getInstance().recompileShaderProgram(args);
}

void GUIConsoleCommandParser::handleFOVCommand(const std::string& args){
    if(!Util::isNumber(args)){
        ERROR_FN(Locale::get("CONSOLE_INVALID_NUMBER"));
        return;
    }
    I32 FoV = (atoi(args.c_str()));
    CLAMP<I32>(FoV,40,140);
    GFX_DEVICE.setHorizontalFoV(FoV);
}

void GUIConsoleCommandParser::handleAddObject(const std::string& args){
    std::istringstream ss(args);
    std::string args1, args2;
    std::getline(ss, args1, ',');
    std::getline(ss, args2, ',');

    float scale = 1.0f;
    if(!Util::isNumber(args2)){
        ERROR_FN(Locale::get("CONSOLE_INVALID_NUMBER"));
    }else{
        scale = (F32)atof(args2.c_str());
    }
    std::string assetLocation = ParamHandler::getInstance().getParam<std::string>("assetsLocation")+"/";

    FileData model;
    model.ItemName = args1 + "_console" + args2;
    model.ModelName  = ((args1.compare("Box3D") == 0 || args1.compare("Sphere3D") == 0) ? "" : assetLocation) + args1;
    model.position = GET_ACTIVE_SCENE()->state().getRenderState().getCamera().getEye();
    model.data = 1.0f;
    model.scale    = vec3<F32>(scale);
    model.orientation = GET_ACTIVE_SCENE()->state().getRenderState().getCamera().getEuler();
    model.type = (args1.compare("Box3D") == 0 || args1.compare("Sphere3D") == 0) ? PRIMITIVE : GEOMETRY;
    model.version = 1.0f;
    model.staticUsage = false;
    model.navigationUsage = true;
    model.useHighDetailNavMesh = true;
    model.physicsUsage = true;
    model.physicsPushable = true;
    GET_ACTIVE_SCENE()->addModel(model);
}

void GUIConsoleCommandParser::handleInvalidCommand(const std::string& args){
    ERROR_FN(Locale::get("CONSOLE_INVALID_COMMAND"),args.c_str());
}