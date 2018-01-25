
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

    const bool selected = selection.isItMe(&node);
    const bool nodeOpen = ImGui::TreeNodeEx(
      name.c_str(),
      (selected ? ImGuiTreeNodeFlags_Selected : 0) |
        (node.asGroup() != nullptr ? ImGuiTreeNodeFlags_DefaultOpen
                                   : ImGuiTreeNodeFlags_Leaf) |
        ImGuiTreeNodeFlags_OpenOnArrow);
    if (ImGui::IsItemClicked()) {
      // Currently one selection
      if (selected) {
        selection.select(nullptr);
      } else {
        selection.select(&node);
      }
      EventManager::Instance.emit("Node::Selected");

      // selected = !selected;
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
