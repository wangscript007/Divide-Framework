################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
/home/ionut/Custom/trunk/Source\ Code/GUI/GUI.cpp \
/home/ionut/Custom/trunk/Source\ Code/GUI/GUIButton.cpp \
/home/ionut/Custom/trunk/Source\ Code/GUI/GUIConsole.cpp \
/home/ionut/Custom/trunk/Source\ Code/GUI/GUIConsoleCommandParser.cpp \
/home/ionut/Custom/trunk/Source\ Code/GUI/GUIElement.cpp \
/home/ionut/Custom/trunk/Source\ Code/GUI/GUIMessageBox.cpp \
/home/ionut/Custom/trunk/Source\ Code/GUI/GUISplash.cpp \
/home/ionut/Custom/trunk/Source\ Code/GUI/GUIText.cpp \
/home/ionut/Custom/trunk/Source\ Code/GUI/GuiFlash.cpp 

OBJS += \
./GUI/GUI.o \
./GUI/GUIButton.o \
./GUI/GUIConsole.o \
./GUI/GUIConsoleCommandParser.o \
./GUI/GUIElement.o \
./GUI/GUIMessageBox.o \
./GUI/GUISplash.o \
./GUI/GUIText.o \
./GUI/GuiFlash.o 

CPP_DEPS += \
./GUI/GUI.d \
./GUI/GUIButton.d \
./GUI/GUIConsole.d \
./GUI/GUIConsoleCommandParser.d \
./GUI/GUIElement.d \
./GUI/GUIMessageBox.d \
./GUI/GUISplash.d \
./GUI/GUIText.d \
./GUI/GuiFlash.d 


# Each subdirectory must supply rules for building sources it contributes
GUI/GUI.o: /home/ionut/Custom/trunk/Source\ Code/GUI/GUI.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/EASTL" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/Threadpool" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/SimpleINI" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/ReCast/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DebugUtils/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/Detour/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DetourCrowd/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DetourTileCache/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/RecastContrib/Include" -I/home/ionut/Custom/Libraries/boost -I/home/ionut/Custom/Libraries/cegui/include -I/home/ionut/Custom/Libraries/physx -I/home/ionut/Custom/Libraries/physx/Include -I/home/ionut/Custom/Libraries/physx/Samples/PxToolkit/include -I/home/ionut/Custom/Libraries/physx/Source/foundation/include -I/home/ionut/Custom/Libraries/OIS/include -I/home/ionut/Custom/Libraries/glbinding/include -I/home/ionut/Custom/Libraries/assimp/include -I/home/ionut/Custom/Libraries/cegui-deps/include -I/usr/include/SDL2 -I/usr/include/AL -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -fopenmp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

GUI/GUIButton.o: /home/ionut/Custom/trunk/Source\ Code/GUI/GUIButton.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/EASTL" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/Threadpool" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/SimpleINI" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/ReCast/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DebugUtils/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/Detour/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DetourCrowd/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DetourTileCache/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/RecastContrib/Include" -I/home/ionut/Custom/Libraries/boost -I/home/ionut/Custom/Libraries/cegui/include -I/home/ionut/Custom/Libraries/physx -I/home/ionut/Custom/Libraries/physx/Include -I/home/ionut/Custom/Libraries/physx/Samples/PxToolkit/include -I/home/ionut/Custom/Libraries/physx/Source/foundation/include -I/home/ionut/Custom/Libraries/OIS/include -I/home/ionut/Custom/Libraries/glbinding/include -I/home/ionut/Custom/Libraries/assimp/include -I/home/ionut/Custom/Libraries/cegui-deps/include -I/usr/include/SDL2 -I/usr/include/AL -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -fopenmp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

GUI/GUIConsole.o: /home/ionut/Custom/trunk/Source\ Code/GUI/GUIConsole.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/EASTL" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/Threadpool" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/SimpleINI" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/ReCast/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DebugUtils/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/Detour/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DetourCrowd/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DetourTileCache/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/RecastContrib/Include" -I/home/ionut/Custom/Libraries/boost -I/home/ionut/Custom/Libraries/cegui/include -I/home/ionut/Custom/Libraries/physx -I/home/ionut/Custom/Libraries/physx/Include -I/home/ionut/Custom/Libraries/physx/Samples/PxToolkit/include -I/home/ionut/Custom/Libraries/physx/Source/foundation/include -I/home/ionut/Custom/Libraries/OIS/include -I/home/ionut/Custom/Libraries/glbinding/include -I/home/ionut/Custom/Libraries/assimp/include -I/home/ionut/Custom/Libraries/cegui-deps/include -I/usr/include/SDL2 -I/usr/include/AL -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -fopenmp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

GUI/GUIConsoleCommandParser.o: /home/ionut/Custom/trunk/Source\ Code/GUI/GUIConsoleCommandParser.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/EASTL" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/Threadpool" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/SimpleINI" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/ReCast/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DebugUtils/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/Detour/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DetourCrowd/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DetourTileCache/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/RecastContrib/Include" -I/home/ionut/Custom/Libraries/boost -I/home/ionut/Custom/Libraries/cegui/include -I/home/ionut/Custom/Libraries/physx -I/home/ionut/Custom/Libraries/physx/Include -I/home/ionut/Custom/Libraries/physx/Samples/PxToolkit/include -I/home/ionut/Custom/Libraries/physx/Source/foundation/include -I/home/ionut/Custom/Libraries/OIS/include -I/home/ionut/Custom/Libraries/glbinding/include -I/home/ionut/Custom/Libraries/assimp/include -I/home/ionut/Custom/Libraries/cegui-deps/include -I/usr/include/SDL2 -I/usr/include/AL -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -fopenmp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

GUI/GUIElement.o: /home/ionut/Custom/trunk/Source\ Code/GUI/GUIElement.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/EASTL" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/Threadpool" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/SimpleINI" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/ReCast/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DebugUtils/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/Detour/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DetourCrowd/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DetourTileCache/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/RecastContrib/Include" -I/home/ionut/Custom/Libraries/boost -I/home/ionut/Custom/Libraries/cegui/include -I/home/ionut/Custom/Libraries/physx -I/home/ionut/Custom/Libraries/physx/Include -I/home/ionut/Custom/Libraries/physx/Samples/PxToolkit/include -I/home/ionut/Custom/Libraries/physx/Source/foundation/include -I/home/ionut/Custom/Libraries/OIS/include -I/home/ionut/Custom/Libraries/glbinding/include -I/home/ionut/Custom/Libraries/assimp/include -I/home/ionut/Custom/Libraries/cegui-deps/include -I/usr/include/SDL2 -I/usr/include/AL -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -fopenmp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

GUI/GUIMessageBox.o: /home/ionut/Custom/trunk/Source\ Code/GUI/GUIMessageBox.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/EASTL" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/Threadpool" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/SimpleINI" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/ReCast/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DebugUtils/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/Detour/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DetourCrowd/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DetourTileCache/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/RecastContrib/Include" -I/home/ionut/Custom/Libraries/boost -I/home/ionut/Custom/Libraries/cegui/include -I/home/ionut/Custom/Libraries/physx -I/home/ionut/Custom/Libraries/physx/Include -I/home/ionut/Custom/Libraries/physx/Samples/PxToolkit/include -I/home/ionut/Custom/Libraries/physx/Source/foundation/include -I/home/ionut/Custom/Libraries/OIS/include -I/home/ionut/Custom/Libraries/glbinding/include -I/home/ionut/Custom/Libraries/assimp/include -I/home/ionut/Custom/Libraries/cegui-deps/include -I/usr/include/SDL2 -I/usr/include/AL -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -fopenmp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

GUI/GUISplash.o: /home/ionut/Custom/trunk/Source\ Code/GUI/GUISplash.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/EASTL" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/Threadpool" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/SimpleINI" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/ReCast/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DebugUtils/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/Detour/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DetourCrowd/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DetourTileCache/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/RecastContrib/Include" -I/home/ionut/Custom/Libraries/boost -I/home/ionut/Custom/Libraries/cegui/include -I/home/ionut/Custom/Libraries/physx -I/home/ionut/Custom/Libraries/physx/Include -I/home/ionut/Custom/Libraries/physx/Samples/PxToolkit/include -I/home/ionut/Custom/Libraries/physx/Source/foundation/include -I/home/ionut/Custom/Libraries/OIS/include -I/home/ionut/Custom/Libraries/glbinding/include -I/home/ionut/Custom/Libraries/assimp/include -I/home/ionut/Custom/Libraries/cegui-deps/include -I/usr/include/SDL2 -I/usr/include/AL -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -fopenmp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

GUI/GUIText.o: /home/ionut/Custom/trunk/Source\ Code/GUI/GUIText.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/EASTL" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/Threadpool" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/SimpleINI" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/ReCast/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DebugUtils/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/Detour/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DetourCrowd/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DetourTileCache/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/RecastContrib/Include" -I/home/ionut/Custom/Libraries/boost -I/home/ionut/Custom/Libraries/cegui/include -I/home/ionut/Custom/Libraries/physx -I/home/ionut/Custom/Libraries/physx/Include -I/home/ionut/Custom/Libraries/physx/Samples/PxToolkit/include -I/home/ionut/Custom/Libraries/physx/Source/foundation/include -I/home/ionut/Custom/Libraries/OIS/include -I/home/ionut/Custom/Libraries/glbinding/include -I/home/ionut/Custom/Libraries/assimp/include -I/home/ionut/Custom/Libraries/cegui-deps/include -I/usr/include/SDL2 -I/usr/include/AL -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -fopenmp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

GUI/GuiFlash.o: /home/ionut/Custom/trunk/Source\ Code/GUI/GuiFlash.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D_DEBUG -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/EASTL" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/Threadpool" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/SimpleINI" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/ReCast/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DebugUtils/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/Detour/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DetourCrowd/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/DetourTileCache/Include" -I"/home/ionut/Custom/trunk/EclipseData/Divide-Game/../../Source Code/Libs/ReCast/RecastContrib/Include" -I/home/ionut/Custom/Libraries/boost -I/home/ionut/Custom/Libraries/cegui/include -I/home/ionut/Custom/Libraries/physx -I/home/ionut/Custom/Libraries/physx/Include -I/home/ionut/Custom/Libraries/physx/Samples/PxToolkit/include -I/home/ionut/Custom/Libraries/physx/Source/foundation/include -I/home/ionut/Custom/Libraries/OIS/include -I/home/ionut/Custom/Libraries/glbinding/include -I/home/ionut/Custom/Libraries/assimp/include -I/home/ionut/Custom/Libraries/cegui-deps/include -I/usr/include/SDL2 -I/usr/include/AL -O0 -g3 -Wall -c -fmessage-length=0 -std=c++1y -fopenmp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

