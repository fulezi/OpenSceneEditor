#ifndef SOLEIL__IMGUIHANDLER_H_
#define SOLEIL__IMGUIHANDLER_H_

#include <iostream>
#include <osg/Camera>
#include <osgGA/GUIEventHandler>
#include <osgViewer/ViewerEventHandlers>

#include "imgui.h"

class ImGUIEventHandler : public osgGA::GUIEventHandler
{
private:
  double time;
  bool   mousePressed[3];
  float  mouseWheel;
  GLuint fontTexture;
  bool   initialized;

protected:
  void initialize(osg::RenderInfo& renderInfo);
  
public:
  ImGUIEventHandler();

  void newFrame(osg::RenderInfo& renderInfo);

  bool handle(const osgGA::GUIEventAdapter& eventAdapter,
              osgGA::GUIActionAdapter& /*actionAdapter*/,
              osg::Object* /*object*/, osg::NodeVisitor* /*nodeVisitor*/);

};

class ImGUINewFrame : public osg::Camera::DrawCallback
{
  ImGUIEventHandler& imguiHandler;

public:
  ImGUINewFrame(ImGUIEventHandler& imguiHandler)
    : imguiHandler(imguiHandler)
  {
  }

  void operator()(osg::RenderInfo& renderInfo) const override
  {
    imguiHandler.newFrame(renderInfo);
  }
};

class ImGUIRender : public osg::Camera::DrawCallback
{
public:
  void operator()(osg::RenderInfo& /*renderInfo*/) const override;

  
  //test
  osg::ref_ptr<osg::Geode> root;
};

#endif /* SOLEIL__IMGUIHANDLER_H_ */
