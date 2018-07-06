#include "Headers/XMLEntryData.h"

#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"

namespace Divide {

XMLEntryData::XMLEntryData()
{
    scriptLocation = "XML";
    config = "config.xml";
    startupScene = "DefaultScene";
    scenesLocation = "Scenes";
    assetsLocation = "assets";
    serverAddress = "192.168.0.2";
}

XMLEntryData::~XMLEntryData()
{
}

bool XMLEntryData::fromXML(const char* xmlFile) {
    Console::printfn(Locale::get(_ID("XML_LOAD_SCRIPTS")));

    LOAD_FILE(xmlFile);
    if (FILE_VALID()) {
        GET_PARAM(scriptLocation);
        GET_PARAM(config);
        GET_PARAM(startupScene);
        GET_PARAM(scenesLocation);
        GET_PARAM(assetsLocation);
        GET_PARAM(serverAddress);
        return true;
    }

    return false;
}

bool XMLEntryData::toXML(const char* xmlFile) const {
    PREPARE_FILE_FOR_WRITING(xmlFile);
    PUT_PARAM(scriptLocation);
    PUT_PARAM(config);
    PUT_PARAM(startupScene);
    PUT_PARAM(scenesLocation);
    PUT_PARAM(assetsLocation);
    PUT_PARAM(serverAddress);
    SAVE_FILE(xmlFile);
    return true;
}

}; //namespace Divide
