cmake_minimum_required (VERSION 3.5)

project(OpenSceneEditor)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -W -Wall -Wextra")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(OpenSceneGraph REQUIRED osg osgDB osgUtil osgViewer osgGA osgManipulator osgAnimation )

find_package(OpenGL REQUIRED)

include_directories (SYSTEM
  ${OPENGL_INCLUDE_DIR}
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
  )

