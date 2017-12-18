
#include <osg/NodeVisitor>

namespace ose {

  /**
   * Compute a representation of normals from children geometries.
   *
   * Normals are bound to vertices.
   * The length of each normal is controlled with the magniture parameter.
   */
  struct CollectNormalsVisitor : public osg::NodeVisitor
  {
    /** Vector of normal lines: [normal1-begin, normal1-end, ...] */
    std::vector<osg::Vec3> normalList;
    float                  magnitude;

    CollectNormalsVisitor(const float magnitude = 2.0f);

    void apply(osg::Geode& geode) override;
    osg::ref_ptr<osg::Geode> toNormalsGeomtery(
      osg::Vec4 color = osg::Vec4(1.0f, 0.0f, 0.0f, 1.0f));
  };

} // ose
