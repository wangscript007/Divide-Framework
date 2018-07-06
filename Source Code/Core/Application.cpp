#include "Headers/Kernel.h"
#include "Headers/Application.h"
#include "Core/Headers/ParamHandler.h"

Application::Application() : _kernel(NULL),
							 _mainWindowId(-1)
{
}

Application::~Application(){
	PRINT_FN(Locale::get("STOP_KERNEL"));
	SAFE_DELETE(_kernel);
}

I8 Application::Initialize(const std::string& entryPoint){   
	assert(!entryPoint.empty());
	///Read language table
	ParamHandler::getInstance().setDebugOutput(false);
	///Print a copyright notice in the log file
	Console::getInstance().printCopyrightNotice();
	PRINT_FN(Locale::get("START_APPLICATION"));
	///Create a new kernel
	_kernel = New Kernel();
	assert(_kernel != NULL);
	///and load it via an XML file config
	_mainWindowId = _kernel->Initialize(entryPoint);
	return _mainWindowId;
}

void Application::run(){
	_kernel->beginLogicLoop();
}

void Application::Deinitialize(){
	_kernel->Shutdown();
}