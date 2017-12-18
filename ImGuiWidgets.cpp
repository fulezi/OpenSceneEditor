
#include <iostream>
#include <string>

#include <osg/ComputeBoundsVisitor>
#include <osg/Geode>
#include <osg/Group>
#include <osg/MatrixTransform>
#include <osg/NodeVisitor>
#include <osg/PolygonMode>
#include <osg/ShapeDrawable>
#include <osgDB/ReadFile>

#include "EventManager.hpp"
#include "ImGuiWidgets.hpp"
#include "stringutils.hpp"

#include "imgui.h"
#include "tinyfiledialogs.h"

namespace ose {

  /* --- Globals --- */
  std::vector<std::string> PopUps;

  /* --- Selection --- */
  Selection::Selection()
    : ptr(nullptr)
    , aGroup(false)
    , state(State::Unselected)
  {
  }

  void Selection::select(osg::Node* node)
  {
    state  = (node == nullptr) ? State::Unselected : State::NewlySelected;
    aGroup = (node != nullptr && node->asGroup() != nullptr);
    ptr    = node;
  }

  osg::ref_ptr<osg::Node> Selection::getSelection() noexcept
  {
    osg::ref_ptr<osg::Node> ref;
    if (ptr.lock(ref)) return ref;
    return nullptr;
  }

  Selection::State Selection::getState(void) noexcept
  {
    if (state == State::NewlySelected) {
      state = State::Selected;
      return State::NewlySelected;
    }
    return state;
  }

  /* --- Visitors --- */

  /**
   *
   */
  class Visitor : public osg::NodeVisitor
  {
  public:
    Visitor(Selection& selection);

  public:
    void apply(osg::Node& node) override;

  private:
    Selection& selection;
  };

  Visitor::Visitor(Selection& selection)
    : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN)
    , selection(selection)
  {
  }

  void Visitor::apply(osg::Node& node)
  {
    const std::string name = [&node]() {
      std::string name =
        node.libraryName() + std::string("::") + node.className();
      if (not node.getName().empty()) {
        name += ": " + node.getName();
      }
      return name;
    }();

    bool       selected = selection.isItMe(&node);
    const bool nodeOpen = ImGui::TreeNodeEx(
      name.c_str(), selected ? ImGuiTreeNodeFlags_Selected : 0);
    if (ImGui::IsItemClicked()) {
      // Currently one selection
      selection.select(&node);
      EventManager::Instance.emit("Node::Selected");
      selected = true;
    }

    if (selected && selection.isAGroup()) {
      if (ImGui::Button("Add..")) ImGui::OpenPopup("Pouet");

      if (ImGui::BeginPopup("Pouet")) {
        if (ImGui::MenuItem("Import")) {
          osg::ref_ptr<osg::Group> group = node.asGroup();
          assert(group);

          auto import = [group]() {
            char const* lTheOpenFileName;

            lTheOpenFileName =
              tinyfd_openFileDialog("Open File", "", 0, nullptr, nullptr, 0);

            if (lTheOpenFileName) {
              // TODO: Error
              // tinyfd_messageBox("Error", "Open file name is NULL", "ok",
              // "error",
              //                   1);
              auto model = osgDB::readNodeFile(lTheOpenFileName);
              if (model) {
                assert(group);

                group->addChild(model);
                EventManager::Instance.emit("Node::Imported");

              } else {
                PopUps.push_back(toString("Failed to import file: '",
                                          lTheOpenFileName, "'\n\n"));
              }
            }
          };

          EventManager::Instance.delay(import);

        } else if (ImGui::MenuItem("Add Group")) {
          osg::ref_ptr<osg::Group> group = node.asGroup();
          assert(group);
          EventManager::Instance.delay(
            [group]() { group->addChild(new osg::Group); });
        } else if (ImGui::MenuItem("Add MatrixTransform")) {
          osg::ref_ptr<osg::Group> group = node.asGroup();
          assert(group);
          EventManager::Instance.delay(
            [group]() { group->addChild(new osg::MatrixTransform); });
        } else if (ImGui::MenuItem("Add Camera")) {
          osg::ref_ptr<osg::Group> group = node.asGroup();
          assert(group);
          EventManager::Instance.delay([group]() {
            osg::ref_ptr<osg::Camera> camera = new osg::Camera;
            camera->setClearMask(GL_DEPTH_BUFFER_BIT);
            camera->setRenderOrder(osg::Camera::POST_RENDER);
	    
            group->addChild(camera);
          });
        }

        ImGui::EndPopup();
      }
    }

    if (nodeOpen) {
      traverse(node);

      ImGui::TreePop();
    }
  }

  void sceneTree(osg::Node& rootNode, Selection& selection)
  {
    // TODO: Improvement, Use visitor to make an array and update this array?
    Visitor v(selection);
    v.setTraversalMask(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN);

    rootNode.accept(v);
  }

  void widgetObject(osg::Object& obj)
  {
    char objectName[NameSizeMax];

    strncpy(objectName, obj.getName().c_str(), NameSizeMax);

    ImGui::LabelText("Library Name ", "%s", obj.libraryName());
    ImGui::LabelText("Class Name ", "%s", obj.className());
    ImGui::InputText("Name", objectName, NameSizeMax);

    obj.setName(objectName);
  }

  osg::ref_ptr<osg::MatrixTransform> nodeToBoundingBox(osg::Node& node)
  {
    osg::ref_ptr<osg::MatrixTransform> boundingBox = new osg::MatrixTransform;

    osg::ComputeBoundsVisitor cbbv;
    node.accept(cbbv);

    osg::BoundingBox bb = cbbv.getBoundingBox();
    // osg::Matrix      localToWorld =
    //   osg::computeLocalToWorld(node.getParent(0)->getParentalNodePaths()[0]);

    // osg::BoundingBox bb;
    // for (unsigned int i = 0; i < 8; ++i)
    //   bb.expandBy(localBB.corner(i) * localToWorld);

    boundingBox->setMatrix(osg::Matrix::scale(bb.xMax() - bb.xMin(),
                                              bb.yMax() - bb.yMin(),
                                              bb.zMax() - bb.zMin()) *
                           osg::Matrix::translate(bb.center()));

    osg::ref_ptr<osg::Box> box = new osg::Box;

    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    geode->addDrawable(new osg::ShapeDrawable(box));

    boundingBox->addChild(geode);
    boundingBox->getOrCreateStateSet()->setAttributeAndModes(
      new osg::PolygonMode(osg::PolygonMode::FRONT_AND_BACK,
                           osg::PolygonMode::LINE));
    boundingBox->getOrCreateStateSet()->setMode(GL_LIGHTING,
                                                osg::StateAttribute::OFF);

    return boundingBox;
  }

} // ose
