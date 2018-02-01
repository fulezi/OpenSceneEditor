
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

  static char const* GlModeToName(GLenum m)
  {
    using osg::PrimitiveSet;

    switch (m) {
      case PrimitiveSet::Mode::POINTS: return "POINTS";
      case PrimitiveSet::Mode::LINES: return "LINES";
      case PrimitiveSet::Mode::LINE_STRIP: return "LINE_STRIP";
      case PrimitiveSet::Mode::LINE_LOOP: return "LINE_LOOP";
      case PrimitiveSet::Mode::TRIANGLES: return "TRIANGLES";
      case PrimitiveSet::Mode::TRIANGLE_STRIP: return "TRIANGLE_STRIP";
      case PrimitiveSet::Mode::TRIANGLE_FAN: return "TRIANGLE_FAN";
      case PrimitiveSet::Mode::QUADS: return "QUADS";
      case PrimitiveSet::Mode::QUAD_STRIP: return "QUAD_STRIP";
      case PrimitiveSet::Mode::POLYGON: return "POLYGON";
      case PrimitiveSet::Mode::LINES_ADJACENCY: return "LINES_ADJACENCY";
      case PrimitiveSet::Mode::LINE_STRIP_ADJACENCY:
        return "LINE_STRIP_ADJACENCY";
      case PrimitiveSet::Mode::TRIANGLES_ADJACENCY:
        return "TRIANGLES_ADJACENCY";
      case PrimitiveSet::Mode::TRIANGLE_STRIP_ADJACENCY:
        return "TRIANGLE_STRIP_ADJACENCY";
      case PrimitiveSet::Mode::PATCHES: return "PATCHES";
    }
    assert(false && "unknown GL mode");
    return "Unknown";
  }

  static char const* PrimitiveTypeToName(osg::PrimitiveSet::Type type)
  {
    using osg::PrimitiveSet;

    switch (type) {
      case PrimitiveSet::Type::PrimitiveType: return "PrimitiveType";
      case PrimitiveSet::Type::DrawArraysPrimitiveType:
        return "DrawArraysPrimitiveType";
      case PrimitiveSet::Type::DrawArrayLengthsPrimitiveType:
        return "DrawArrayLengthsPrimitiveType";
      case PrimitiveSet::Type::DrawElementsUBytePrimitiveType:
        return "DrawElementsUBytePrimitiveType";
      case PrimitiveSet::Type::DrawElementsUShortPrimitiveType:
        return "DrawElementsUShortPrimitiveType";
      case PrimitiveSet::Type::DrawElementsUIntPrimitiveType:
        return "DrawElementsUIntPrimitiveType";
      case PrimitiveSet::Type::MultiDrawArraysPrimitiveType:
        return "MultiDrawArraysPrimitiveType";
      case PrimitiveSet::Type::DrawArraysIndirectPrimitiveType:
        return "DrawArraysIndirectPrimitiveType";
      case PrimitiveSet::Type::DrawElementsUByteIndirectPrimitiveType:
        return "DrawElementsUByteIndirectPrimitiveType";
      case PrimitiveSet::Type::DrawElementsUShortIndirectPrimitiveType:
        return "DrawElementsUShortIndirectPrimitiveType";
      case PrimitiveSet::Type::DrawElementsUIntIndirectPrimitiveType:
        return "DrawElementsUIntIndirectPrimitiveType";
      case PrimitiveSet::Type::MultiDrawArraysIndirectPrimitiveType:
        return "MultiDrawArraysIndirectPrimitiveType";
      case PrimitiveSet::Type::MultiDrawElementsUByteIndirectPrimitiveType:
        return "MultiDrawElementsUByteIndirectPrimitiveType";
      case PrimitiveSet::Type::MultiDrawElementsUShortIndirectPrimitiveType:
        return "MultiDrawElementsUShortIndirectPrimitiveType";
      case PrimitiveSet::Type::MultiDrawElementsUIntIndirectPrimitiveType:
        return "MultiDrawElementsUIntIndirectPrimitiveType";
    }
  }

  static char const* BindingToName(const osg::Array* array)
  {
    if (array == nullptr) return "N/A";

    osg::Array::Binding binding = array->getBinding();
    switch (binding) {
      case osg::Array::BIND_UNDEFINED: return "BIND_UNDEFINED";
      case osg::Array::BIND_OFF: return "BIND_OFF";
      case osg::Array::BIND_OVERALL: return "BIND_OVERALL";
      case osg::Array::BIND_PER_PRIMITIVE_SET: return "BIND_PER_PRIMITIVE_SET";
      case osg::Array::BIND_PER_VERTEX: return "BIND_PER_VERTEX";
    }
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

  static void widgetNode(const osg::Node* node)
  {
    if (node == nullptr) return;

    const osg::StateSet* state = node->getStateSet();
    if (state) {
      ImGui::Separator();
      for (const auto& it : state->getAttributeList()) {
	
      }
    }

    ImGui::Separator();
    widgetGeometry(node->asGeometry());
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
