cmake_minimum_required(VERSION 3.10)
project(PathTree)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(PathTree WIN32 main.cpp resource.rc)

if(MSVC)
    set_target_properties(PathTree PROPERTIES LINK_FLAGS "/ENTRY:mainCRTStartup")
endif()

if(MINGW)
    target_link_options(PathTree PRIVATE "-static-libgcc" "-static-libstdc++" "-static")
endif()