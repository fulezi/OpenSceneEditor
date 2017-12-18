
#include <cassert>

#include <osg/Group>

#include "Utils.hpp"

namespace ose {

  RemoveNodeVisitor::RemoveNodeVisitor(osg::Node* node)
    : NodeVisitor(TRAVERSE_ALL_CHILDREN)
    , toDelete(node)
    , deleted(0)
  {
  }

  void RemoveNodeVisitor::apply(osg::Node& current)
  {
    osg::Group* group = current.asGroup();
    if (group) {
      unsigned int pos = group->getChildIndex(toDelete);
      if (pos < group->getNumChildren()) {
        bool done = group->removeChildren(pos, 1);
	assert(done && "Node note deleted");

	deleted++;
        return;
      }

      traverse(current);
    }
  }

} // ose
