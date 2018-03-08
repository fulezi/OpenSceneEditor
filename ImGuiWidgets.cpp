
#include <iostream>
#include <string>

#include <osg/ComputeBoundsVisitor>
#include <osg/Geode>
#include <osg/Group>
#include <osg/MatrixTransform>
#include <osg/NodeVisitor>
#include <osg/PolygonMode>
#include <osg/ShapeDrawable>
#include <osgAnimation/BasicAnimationManager>
#include <osgDB/ReadFile>

#include "EventManager.h"
#include "EventNodeSelected.h"
#include "ImGuiWidgets.hpp"
#include "stringutils.hpp"

#include "imgui.h"
#include "tinyfiledialogs.h"

namespace ose {

/* --- Enum to name functions --- */
#include "EnumToNameFunction.cpp"
  /* --- Enum to name functions --- */

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
      Soleil::EventManager::Emit(
        std::make_shared<ose::EventNodeSelected>(&node, this->getNodePath()));

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

  static void widgetGeometry(const osg::Geometry* geometry)
  {
    if (geometry == nullptr) return;

    ImGui::Text("%u Elements", geometry->getVertexArray()->getNumElements());
    ImGui::Text("Nornal binding: %s",
                BindingToName(geometry->getNormalArray()));
    for (const osg::Array* array : geometry->getTexCoordArrayList()) {
      ImGui::Text("Texture binding: %s", BindingToName(array));
    }
    for (const osg::ref_ptr<osg::PrimitiveSet> set :
         geometry->getPrimitiveSetList()) {
      ImGui::Text("%s", PrimitiveTypeToName(set->getType()));
      ImGui::Indent();
      ImGui::Text("%u Primitives of type %s", set->getNumPrimitives(),
                  GlModeToName(set->getMode()));
      ImGui::Unindent();
    }
  }

  static void writeCallback(osg::Callback* cb)
  {
    if (cb == nullptr) return;

    ImGui::Text("Update callback: %s", cb->getCompoundClassName().c_str());
    osgAnimation::BasicAnimationManager* animNode =
      dynamic_cast<osgAnimation::BasicAnimationManager*>(cb);
    if (animNode) {
      for (auto&& anim : animNode->getAnimationList()) {
        if (ImGui::Button(anim->getName().c_str())) {

	  //anim->setPlayMode(osgAnimation::Animation::PlayMode::LOOP);
          animNode->playAnimation(anim);
          // Soleil::EventManager::Emit(EventPlayAnimation(animNode, anim));
        }
      }
    }
  }

  static void widgetNode(osg::Node* node)
  {
    if (node == nullptr) return;

#if 0
    const osg::StateSet* state = node->getStateSet();
    if (state) {
      ImGui::Separator();
      for (const auto& it : state->getAttributeList()) {
      }
    }
#endif

    ImGui::Separator();
    widgetGeometry(node->asGeometry());
    ImGui::Separator();
    writeCallback(node->getUpdateCallback());
  }

  void widgetObject(osg::Object& obj)
  {
    char objectName[NameSizeMax];

    strncpy(objectName, obj.getName().c_str(), NameSizeMax);

    ImGui::LabelText("Library Name ", "%s", obj.libraryName());
    ImGui::LabelText("Class Name ", "%s", obj.className());
    ImGui::InputText("Name", objectName, NameSizeMax);

    widgetNode(obj.asNode());

    obj.setName(
      objectName); // TODO: Only change the name if the text has changed
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
