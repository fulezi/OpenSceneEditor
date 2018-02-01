
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
#include <osgParticle/ParticleSystemUpdater>
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
#include "GeometryUtils.h"
#include "ImGuiHandler.hpp"
#include "ImGuiWidgets.hpp"
#include "Utils.hpp"
#include "stringutils.hpp"

#include "tinyfiledialogs.h"

// Draw Normals
#include <osg/Geode>
#include <osg/Geometry>
#include <osgUtil/SmoothingVisitor>

// static osg::Node*
// GetNodeById(osg::Node& root, const std::string& name)
// {
//   if (root.getName() == name) return &root;
//   NameVisitor v;
//   v.name = name;

//   root.traverse(v);
//   return v.found;
// }

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

static osg::ref_ptr<osg::Group> scene;
static ose::Selection           selection;

static void
drawUI(void)
{
  // Main Menu
  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("Save")) {
        char const* lTheOpenFileName;
        char const* lFilterPatterns[2] = {"*.osgb", "*.osgt"};

        lTheOpenFileName =
          tinyfd_saveFileDialog("Save File", "", 2, lFilterPatterns, nullptr);

        if (lTheOpenFileName) {
          osg::ref_ptr<osg::Node> ref = selection.getSelection();
          if (not ref) {
            // TODO: Need to be a node?
            ref = scene;
          }
          osgDB::writeNodeFile(*ref, lTheOpenFileName);
        }
      }
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Model")) {
      if (ImGui::MenuItem("Explode")) {
        // osg::ref_ptr<osg::Geometry> geom = nullptr;
        // osg::ref_ptr<osg::Node>     ref  = selection.getSelection();
        // if (ref) {
        //   geom = ref->asGeometry();
        // }
        // if (geom == nullptr) {
        //   ImGui::OpenPopup("CannotExplode");
        //   ImGui::Text("Only working when a osg::Geometry is selected");
        //   if (ImGui::BeginPopup("CannotExplode")) {
        //     ImGui::EndPopup();
        //   }
        // } else {
        // std::cout << "BOOOOOOOOOOOOOOOOOOOOOOOOUUUUNM"
        //           << "\n";
        // scene->addChild(Soleil::ExplodeGeometry(*geom));
        // Soleil::ExplodeGeometry(*geom);
        // }
        Soleil::ExplodeGeometry();
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

  ImGui::Begin("Scene Graph");

  ose::sceneTree(*scene, selection);

  ImGui::Separator();

  ImGui::End();

  ImGui::Begin("Object Property");
  {
    osg::ref_ptr<osg::Node> ref = selection.getSelection();
    if (ref) {
      ose::widgetObject(*ref);
    }
  }
  ImGui::End();

  ImGui::ShowTestWindow(nullptr);
}

int
main(int argc, char** argv)
{

  osgViewer::Viewer viewer;

  osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;

  char const* fileToOpen;
  char const* filterPatterns[4] = {"*.osgt", "*.osgb", "*.osg",
                                   "*.3ds"}; // TODO: array

  if (argc == 2) {
    fileToOpen = argv[1];
  } else {
    fileToOpen =
      tinyfd_openFileDialog("Open File", "", 4, filterPatterns, NULL, 0);
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

#if 1
    model = current->asGroup();
    if (model == nullptr) {
      model = new osg::Group;
      model->addChild(current);
    }
#endif

#if 0
    ose::CollectNormalsVisitor c;
    model->accept(c);
    root->addChild(c.toNormalsGeomtery(osg::Vec4(1.0f, 0.0f, 0.0f, 1.0f)));
#endif

  } else {
    std::cout << "Empty init"
              << "\n";
    model = new osg::MatrixTransform;
  }

  scene = model;
  Soleil::MeshSetLineMode(*scene);
  root->addChild(model);

  viewer.realize();

#if 1
  {

    //----------------------------------------------------
    // Setup of the ImGUI

    osg::Camera* camera = viewer.getCamera();
    assert(camera);
    osg::ref_ptr<ImGUIEventHandler> imguiHandler =
      new ImGUIEventHandler{drawUI};
    viewer.addEventHandler(imguiHandler);

    camera->setPreDrawCallback(new ImGUINewFrame{*imguiHandler});
    camera->setPostDrawCallback(new ImGUIRender{*imguiHandler});

  } // End experimental
#endif

#if 1
  {
    osg::ref_ptr<osg::Node> particle = Soleil::ExplodeGeometry();
    //root->addChild(particle);
    root->addChild(particle);
  }
#elif 0
  {
    gna osgParticle::ParticleSystem* ps1 =
      Soleil::create_simple_particle_system(root);

    osgParticle::ParticleSystemUpdater* psu =
      new osgParticle::ParticleSystemUpdater;
    psu->addParticleSystem(ps1);

    // add the updater node to the scene graph
    root->addChild(psu);
  }
#endif

  viewer.setSceneData(root);

  // osg::ref_ptr<osgGA::TrackballManipulator> m = new
  // osgGA::TrackballManipulator;
  // viewer.setCameraManipulator(m);

  return viewer.run();
}
