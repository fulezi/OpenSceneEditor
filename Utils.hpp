#ifndef SOLEIL__UTILS_H_
#define SOLEIL__UTILS_H_

#include <osg/Node>
#include <osg/NodeVisitor>

namespace ose {

  /**
   * Delete the first node find in the group. Works recursively.
   */
  class RemoveNodeVisitor : public osg::NodeVisitor
  {
  public:
    RemoveNodeVisitor(osg::Node* node);

    void apply(osg::Node& searchNode) override;

    int getNumDeleted(void) const noexcept { return deleted; }

  private:
    osg::Node* toDelete;
    int        deleted;
  };

} // ose

#endif /* SOLEIL__UTILS_H_ */
