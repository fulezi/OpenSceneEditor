
#ifndef SOLEIL__IMGUIWIDGETS_H_
#define SOLEIL__IMGUIWIDGETS_H_

#include <osg/MatrixTransform>
#include <osg/Node>
#include <osg/ref_ptr>

#include <string>
#include <vector>

namespace ose {

  constexpr int NameSizeMax = 1024;

  class Selection
  {
  public:
    Selection();

  public:
    enum class State
    {
      Unselected,
      NewlySelected,
      Selected
    };

  public:
    void select(osg::Node* ptr);
    osg::ref_ptr<osg::Node> getSelection() noexcept;
    bool                    isAGroup(void) const noexcept { return aGroup; }
    State                   getState(void) noexcept;
    bool isItMe(osg::Node* node) const noexcept { return ptr.get() == node; }

  private:
    osg::observer_ptr<osg::Node> ptr;
    bool                         aGroup;
    State                        state;
  };

  void sceneTree(osg::Node& rootNode, Selection& selection);

  void widgetObject(osg::Object& node);

  osg::ref_ptr<osg::MatrixTransform> nodeToBoundingBox(osg::Node& node);

  extern std::vector<std::string> PopUps;

} // ose

#endif /* SOLEIL__IMGUIWIDGETS_H_ */
