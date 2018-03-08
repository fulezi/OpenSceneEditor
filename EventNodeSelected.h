

#ifndef SOLEIL__EVENTNODESELECTED_H_
#define SOLEIL__EVENTNODESELECTED_H_

#include "EventManager.h"

#include <osg/Node>

namespace ose {

  class EventNodeSelected : public Soleil::Event
  {
  public:
    constexpr static unsigned long Type(void) { return 2134; }
    // TODO: a constexpr hash

  public:
    EventNodeSelected(osg::ref_ptr<osg::Node> node,
                      osg::NodePath   nodePath)
      : Event(Type())
      , node(node)
      , nodePath(nodePath)
    {
    }

  public:
    osg::ref_ptr<osg::Node> node;
    osg::NodePath   nodePath;
  };

} // ose

#endif /* SOLEIL__EVENTNODESELECTED_H_ */
