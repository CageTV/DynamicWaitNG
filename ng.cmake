# DynamicWaitNG.dll on alandtse's CommonLibSSE-NG (Skyrim 1.5.97 to 1.6.1170 and 1.7.x, SE and AE); included by E:/WorkSpace/ng-build/CMakeLists.txt (see ng_plugin there).
find_path(DWNG_SIMPLEINI_INCLUDE_DIRS "SimpleIni.h")
ng_plugin(TARGET DynamicWaitNG NAME DynamicWaitNG VERSION 1.0.1 AUTHOR "CageTV" ROOT "${CMAKE_CURRENT_LIST_DIR}"
    SOURCES src/main.cpp src/Settings.cpp src/Wait.cpp src/Menu.cpp
    INCLUDES "${CMAKE_CURRENT_LIST_DIR}/src" "${DWNG_SIMPLEINI_INCLUDE_DIRS}"
    PCH "${CMAKE_CURRENT_LIST_DIR}/src/PCH.h")
