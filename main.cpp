
#include <cassert>
#include <map>
#include <string>

#include <osg/Camera>
#include <osg/NodeCallback>
#include <osg/Switch>
#include <osgAnimation/BasicAnimationManager>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgManipulator/ScaleAxisDragger>
#include <osgManipulator/TabBoxDragger>
#include <osgManipulator/TabBoxTrackballDragger>
#include <osgManipulator/TrackballDragger>
#include <osgManipulator/TranslateAxisDragger>
#include <osgViewer/ViewerEventHandlers>

#include <osg/BlendEquation>
#include <osg/BlendFunc>
#include <osg/Camera>
#include <osg/CullFace>
#include <osg/PolygonMode>
#include <osgGA/TrackballManipulator>

#include <osg/RenderInfo>

#include "CollectNormalsVisitor.hpp"
#include "EventManager.hpp"
#include "ImGuiHandler.hpp"
#include "ImGuiWidgets.hpp"
#include "Utils.hpp"
#include "stringutils.hpp"

#include "tinyfiledialogs.h"

// Draw Normals
#include <osg/Geode>
#include <osg/Geometry>
#include <osgUtil/SmoothingVisitor>



struct ProcessEventCallBack : public osg::NodeCallback
{
  void operator()(osg::Node* node, osg::NodeVisitor* nv)
  {
    ose::EventManager::Instance.processEvents();
  }
};

enum class DraggerType
{
  Scale,
  Rotate,
  Translate,
  ScaleTranslate,
};

static osg::ref_ptr<osgManipulator::Dragger>
addDragger(DraggerType type, osg::MatrixTransform* transform, const int key,
           const osg::BoundingSphere& bounds)
{
  osg::ref_ptr<osgManipulator::Dragger> dragger;

  switch (type) {
    case DraggerType::Scale: dragger = new osgManipulator::TabBoxDragger; break;
    case DraggerType::Rotate:
      dragger = new osgManipulator::TrackballDragger;
      break;
    case DraggerType::Translate:
      dragger = new osgManipulator::TranslateAxisDragger;
      break;
    case DraggerType::ScaleTranslate:
      dragger = new osgManipulator::TabBoxTrackballDragger;
      break;
  };

  dragger->addTransformUpdating(transform);

  dragger->setupDefaultGeometry();
  dragger->setHandleEvents(true);
  dragger->setActivationKeyEvent(key);

  float scale = bounds.radius() * 1.6;
  dragger->setMatrix(osg::Matrix::scale(scale, scale, scale) *
                     osg::Matrix::translate(bounds.center()));

  return dragger;
}

class DraggerHandlerCallBack : public osgGA::GUIEventHandler
{
public:
  std::map<int, unsigned int> keyShortCut;

  bool handle(const osgGA::GUIEventAdapter& eventAdapter,
              osgGA::GUIActionAdapter& actionAdapter, osg::Object* object,
              osg::NodeVisitor* nodeVisitor)
  {
    if (eventAdapter.getEventType() == osgGA::GUIEventAdapter::KEYUP) {
      osg::Switch* draggerSwitch = dynamic_cast<osg::Switch*>(object);
      assert(draggerSwitch);

      const bool found = keyShortCut.count(eventAdapter.getKey()) > 0;
      if (found) {
        const unsigned int positionInSwitch =
          keyShortCut.at(eventAdapter.getKey());
        draggerSwitch->setSingleChildOn(positionInSwitch);
        return true;
      }
    }
    return false;
  }
};

struct EditorGui : public ImGuiHandler::GuiCallback
{
  osg::ref_ptr<osg::Node>  scene;
  osg::ref_ptr<osg::Group> meta;
  ose::Selection           selection;

  bool importMenu = false;

  EditorGui()
  {
    ose::EventManager::Instance.enroll("Node::Selected",
                                       [this]() { setDragger(); });
    ose::EventManager::Instance.enroll("Node::Imported",
                                       [this]() { updateDragger(); });
  }

  virtual void operator()()
  {
    // It can also be members
    static bool   show_test_window    = true;
    static bool   show_another_window = false;
    static ImVec4 clear_color         = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    static char   text[255]           = {0};

    {
      static float f = 0.0f;
      ImGui::Text("Hello, world!");
      ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
      ImGui::ColorEdit3("clear color", (float*)&clear_color);
      if (ImGui::Button("Test Window")) show_test_window ^= 1;
      if (ImGui::Button("Another Window")) show_another_window ^= 1;
      ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                  1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
      ImGui::InputText("Hello: ", text, 254);
    }

    // 2. Show another simple window. In most cases you will use an explicit
    // Begin/End pair to name the window.
    if (show_another_window) {
      ImGui::Begin("Another Window", &show_another_window);
      ImGui::Text("Hello from another window!");
      ImGui::End();
    }

    // 3. Show the ImGui test window. Most of the sample code is in
    // ImGui::ShowTestWindow().
    if (show_test_window) {
      ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiCond_FirstUseEver);
      ImGui::ShowTestWindow(&show_test_window);
    }

    // Main Menu
    if (ImGui::BeginMainMenuBar()) {
      if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("Save")) {
          char const* lTheOpenFileName;
          char const* lFilterPatterns[2] = {"*.osgb", "*.osgt"};

          lTheOpenFileName =
            tinyfd_saveFileDialog("Save File", "", 2, lFilterPatterns, nullptr);

          if (lTheOpenFileName) {
            osgDB::writeNodeFile(*scene, lTheOpenFileName);
          }
        }
        ImGui::EndMenu();
      }
      // if (ImGui::BeginMenu("Edit")) {
      //   if (ImGui::MenuItem("Undo", "CTRL+Z")) {
      //   }
      //   if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {
      //   } // Disabled item
      //   ImGui::Separator();
      //   if (ImGui::MenuItem("Cut", "CTRL+X")) {
      //   }
      //   if (ImGui::MenuItem("Copy", "CTRL+C")) {
      //   }
      //   if (ImGui::MenuItem("Paste", "CTRL+V")) {
      //   }
      //   ImGui::EndMenu();
      // }
      ImGui::EndMainMenuBar();
    }

    // 4. Mine :)
    ImGui::Begin("Scene Graph");

    ose::sceneTree(*scene.get(), selection);

    ImGui::Separator();

    // if (selection.isAGroup) {
    //   ImGui::Text("Hello from another window!");

    //   if (ImGui::BeginMenuBar()) {
    //     ImGui::MenuItem("Import", NULL, &importMenu);
    //     ImGui::EndMenu();
    //   }
    // }
    ImGui::End();

    ImGui::Begin("Object Property");
    {
      osg::ref_ptr<osg::Node> ref = selection.getSelection();
      if (ref) {
        ose::widgetObject(*ref.get());
      }
    }
    ImGui::End();

    /* --- Display Pop Up --- */
    if (ose::PopUps.size() > 0) {
      ImGui::OpenPopup("MessagePopUp");
      if (ImGui::BeginPopupModal("MessagePopUp", NULL,
                                 ImGuiWindowFlags_AlwaysAutoResize)) {

        for (const auto& msg : ose::PopUps) {
          ImGui::Text("%s", msg.c_str());
        }

        if (ImGui::Button("OK")) {
          ose::PopUps.clear();
          ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
      }
    }
  }

  void setDragger(void)
  {
    // First delete any draggers
    // TODO: Must be changed if meta contains other nodes type
    meta->removeChildren(0, meta->getNumChildren());

    osg::ref_ptr<osg::Node> ref = selection.getSelection();
    osg::MatrixTransform*   tr = dynamic_cast<osg::MatrixTransform*>(ref.get());
    if (tr) {
      const auto& bounds = ref->getBound();

      meta->addChild(addDragger(DraggerType::Scale, tr, 's', bounds));
      meta->addChild(addDragger(DraggerType::Rotate, tr, 'r', bounds));
      meta->addChild(addDragger(DraggerType::Translate, tr, 'r', bounds));

      /* FIXME: Workaround because the Combined dragger does not allow to select
       * a dragger that are inside antoher one*/

      osg::ref_ptr<DraggerHandlerCallBack> cb = new DraggerHandlerCallBack;
      cb->keyShortCut['s']                    = 0;
      cb->keyShortCut['r']                    = 1;
      cb->keyShortCut['t']                    = 2;
      meta->addEventCallback(cb);
    }
  }

  void updateDragger()
  {
    osg::ref_ptr<osg::Node> ref = selection.getSelection();
    if (ref) {

      for (std::size_t i = 0; i < meta->getNumChildren(); ++i) {
        osgManipulator::Dragger* dragger =
          dynamic_cast<osgManipulator::Dragger*>(meta->getChild(i));
        if (dragger) {
          float scale = ref->getBound().radius() * 1.6;
          dragger->setMatrix(osg::Matrix::scale(scale, scale, scale) *
                             osg::Matrix::translate(ref->getBound().center()));
        }
      }
    }
  }

private:
  osg::ref_ptr<osgManipulator::Dragger> dragger;
};

struct NameVisitor : public osg::NodeVisitor
{
  std::string name;
  osg::Node*  found = nullptr;

  void apply(osg::Node& node) override
  {
    // std::cout << "==" << node.getName() << "\n";
    if (node.getName() == name)
      found = &node;
    else
      traverse(node);
  }
};

static osg::Node*
GetNodeById(osg::Node& root, const std::string& name)
{
  if (root.getName() == name) return &root;
  NameVisitor v;
  v.name = name;

  root.traverse(v);
  return v.found;
}

template <typename T> struct TypeVisitor : public osg::NodeVisitor
{
  T found = nullptr;

  void apply(osg::Node& node) override
  {
    found = dynamic_cast<T>(&node);
    if (found == nullptr) traverse(node);
  }
};

template <typename T>
static T
GetNodeByType(osg::Node& root)
{
  T casted = dynamic_cast<T>(&root);
  if (casted) return casted;
  TypeVisitor<T> v;

  root.traverse(v);
  return v.found;
}

std::string
Stringify(const osg::Vec3& vec)
{
  return ose::toString("[", vec.x(), ", ", vec.y(), ", ", vec.z(), "]");
}

int
main(int argc, char** argv)
{

  osgViewer::Viewer viewer;

  osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;
  osg::ref_ptr<osg::Switch>          meta =
    new osg::Switch; // TODO: Rename as DraggerGroup

  meta->setNewChildDefaultValue(false);

  // meta->getOrCreateStateSet()->setMode(
  // 				       GL_LIGHTING, osg::StateAttribute::OFF |
  // osg::StateAttribute::PROTECTED);

  char const* fileToOpen;
  char const* filterPatterns[3] = {"*.osgt", "*.osgb", "*.osg"}; // TODO: array

  if (argc == 2) {
    fileToOpen = argv[1];
  } else {
    fileToOpen =
      tinyfd_openFileDialog("Open File", "", 3, filterPatterns, NULL, 0);
  }

  osg::ref_ptr<osg::Group> model;
  if (fileToOpen) {
    osg::ref_ptr<osg::Node> current = osgDB::readNodeFile(fileToOpen);
    assert(current); // TODO: True error message;

    // std::cout
    //   << "Num Child:"
    //   << GetNodeById(*current.get(),
    //   "Camera")->asGroup()->getChild(0)->getName()
    //   << "\n";

    // osg::Node* camNode = GetNodeById(*current.get(), "Camera");
    // assert(camNode);
    // osg::Camera* cam = camNode->asGroup()->getChild(0)->asCamera();

    viewer.realize();

#if 0
#if 0
    viewer.getCamera()->setViewMatrixAsLookAt(
      osg::Vec3(-64, 38, 54), osg::Vec3(0, 0, 0), osg::Vec3(0, 0, 1));
    const double zoom   = .08;
    const double width  = 1920.0;
    const double height = 1080.0;
    viewer.getCamera()->setProjectionMatrixAsOrtho(
      -width / 2.0 * zoom, width / 2.0 * zoom, -height / 2.0 * zoom,
      height / 2.0 * zoom, -1.0, 100);
#elseif 0
    osg::Node* camNode = GetNodeById(*current.get(), "Camera");
    assert(camNode);
    osg::Camera* cam = camNode->asGroup()->getChild(0)->asCamera();

    auto viewMatrix       = cam->getViewMatrix();
    auto projectionMatrix = cam->getProjectionMatrix();

    viewer.getCamera()->setViewMatrix(viewMatrix);
    viewer.getCamera()->setProjectionMatrix(projectionMatrix);

    camNode->asGroup()->removeChild(cam);

#endif
#endif
// cam->setProjectionMatrixAsOrtho(0, 1920, 0, 1080, 0.0, 1000);
// cam->setViewMatrixAsLookAt(osg::Vec3(-64, 38, 54), osg::Vec3(0, 0, 0),
// osg::Vec3(0, 1, 0));
// osgDB::writeNodeFile(*current.get(), "example1.osgt");

// std::cout << cam->getProjectionMatrix() << "\n";

// auto m = dynamic_cast<osg::MatrixTransform*>(camNode)->getMatrix() *
// cam->getViewMatrix();
// cam->setViewMatrix(m);
// assert(cam);
// viewer.setCamera(cam);
// camNode->asGroup()->removeChild(cam);

// osg::ref_ptr<osg::Camera> cam;
// cam = new osg::Camera;
// //viewer.setCamera(cam);
// //cam->setProjectionMatrixAsOrtho(0, 1920, 0, 1080, 0.0, 1000);
// //cam->setProjectionMatrixAsPerspective(45, 1920.0 / 1080.0, 0.0, 100.0);
// cam->setReferenceFrame( osg::Camera::ABSOLUTE_RF );
// cam->setClearMask( GL_DEPTH_BUFFER_BIT );
// cam->setViewMatrixAsLookAt(osg::Vec3(-6, 0, 0), osg::Vec3(0, 0, 0),
// osg::Vec3(0, 1, 0));
// osgDB::writeNodeFile(*current.get(), "example1.osgt");

// cam->addChild(current);

#if 1
    model = current->asGroup();
    if (model == nullptr) {
      model = new osg::Group;
      model->addChild(current);
    }
#endif

    ose::CollectNormalsVisitor c;
    model->accept(c);
    root->addChild(c.toNormalsGeomtery(osg::Vec4(1.0f, 0.0f, 0.0f, 1.0f)));

  } else {
    std::cout << "Empty init"
              << "\n";
    model = new osg::MatrixTransform;
  }

  // ose::PopUps.push_back("Welcome");

  osg::ref_ptr<EditorGui> editorGui = new EditorGui;
  editorGui->scene                  = model;
  editorGui->meta                   = meta;

  root->addChild(model);
  root->addChild(meta);

  viewer.realize();

  
/** Experimental **/
#if 0
  osg::Vec3 eye, center, up;
  {
    osg::Node* camNode = GetNodeById(*model.get(), "Camera");
    assert(camNode);
    osg::MatrixTransform* camMat = dynamic_cast<osg::MatrixTransform*>(camNode);
    assert(camMat);
    osg::Matrix mat;
    auto        b = mat.invert(camMat->getMatrix());
    assert(b);
    mat.getLookAt(eye, center, up);
  }
#endif

#if 0
  osgAnimation::BasicAnimationManager* animNode =
    dynamic_cast<osgAnimation::BasicAnimationManager*>(model->getUpdateCallback());
  assert(animNode);
  for (auto &&anim : animNode->getAnimationList()) {
    std::cout << "Anim: " << anim->getName() << "\n";
    animNode->playAnimation(anim);
  }
#endif
/** Experimental **/

#if 1
  root->addUpdateCallback(new ProcessEventCallBack);
  // FPP version of ImGUIL
  osg::ref_ptr<ImGuiHandler> handler = new ImGuiHandler(editorGui);
  handler->setCameraCallbacks(viewer.getCamera());
  viewer.addEventHandler(handler);
  root->addChild(
    addDragger(DraggerType::Scale, root.get(), 's', root->computeBound()));
  root->addChild(
    addDragger(DraggerType::Rotate, root.get(), 'r', root->computeBound()));
#endif
  viewer.setSceneData(root);

  // return viewer.run();

  // osg::notify(osg::WARN) << "Eye: " << Stringify(eye) << "\n"
  //                        << "Center: " << Stringify(center) << "\n"
  //                        << "Up: " << Stringify(up) << "\n";
  osg::ref_ptr<osgGA::TrackballManipulator> m = new osgGA::TrackballManipulator;
  // m->setHomePosition(eye, center, up, false);
  viewer.setCameraManipulator(m);

  // bool first = true;
  while (!viewer.done()) {
    viewer.frame();
    // std::cout << "Frame number: " << viewer.getFrameStamp()->getFrameNumber()
    //           << std::endl;
    // if (first) {
    //   first = false;
    //   // osgDB::writeNodeFile(*towrite.get(), "example.osgt");

    //   osgViewer::ViewerBase::Cameras cams;
    //   viewer.getCameras(cams, false);
    //   // std::cout << "~~~~~~~~~~~~~~~~ Number of cameras:" << cams.size() <<
    //   // "\n";
    // }
  }

  return 0;
}
