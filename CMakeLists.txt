cmake_minimum_required (VERSION 3.5)

project(OpenSceneEditor)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -W -Wall -Wextra")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


# set(OSG_DIR /home/florian/jeux/OpenSceneGraph/build/INST)
find_package(OpenSceneGraph REQUIRED osg osgDB osgUtil osgViewer osgGA osgManipulator osgAnimation ) # NO_CMAKE_PATH # osgDB osgUtil

find_package(OpenGL REQUIRED)

include_directories (SYSTEM
  ${OPENGL_INCLUDE_DIR}
  # assimp/build/INST/include
  )


include_directories(
  ${OPENSCENEGRAPH_INCLUDE_DIRS}
  imgui/
  )

add_executable(OpenSceneEditor
  main.cpp
  ImGuiHandler.cpp
  imgui/imgui.cpp
  imgui/imgui_draw.cpp
  imgui/imgui_demo.cpp
  tinyfiledialogs.c

  
  ImGuiWidgets.cpp
  Utils.cpp
  EventManager.cpp
  CollectNormalsVisitor.cpp
  )


target_link_libraries(OpenSceneEditor
  ${OPENSCENEGRAPH_LIBRARIES}
  ${OPENGL_LIBRARIES}
  # /home/florian/cppworkspace/OpenSceneEditor/assimp/build/INST/lib/libassimp.a
  # /home/florian/cppworkspace/OpenSceneEditor/assimp/build/INST/lib/libzlibstatic.a
  # /home/florian/cppworkspace/OpenSceneEditor/assimp/build/INST/lib/libIrrXML.a
  )

