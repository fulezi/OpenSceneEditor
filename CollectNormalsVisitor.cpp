
#include "CollectNormalsVisitor.hpp"

#include <cassert>

#include <osg/Geode>
#include <osg/Geometry>

namespace ose {

  CollectNormalsVisitor::CollectNormalsVisitor(const float magnitude)
    : NodeVisitor(osg::NodeVisitor::TraversalMode::TRAVERSE_ACTIVE_CHILDREN)
    , magnitude(magnitude)
  {
  }

  void CollectNormalsVisitor::apply(osg::Geode& geode)
  {
    for (unsigned int it = 0; it < geode.getNumDrawables(); ++it) {
      const osg::Drawable* childDrawable = geode.getDrawable(it);
      assert(childDrawable);

      const osg::Geometry* geometry = childDrawable->asGeometry();
      /* We only support geometry */
      if (geometry) {
        const osg::Vec3Array* normals =
          dynamic_cast<const osg::Vec3Array*>(geometry->getNormalArray());
        assert(normals);
        const osg::Vec3Array* vertices =
          dynamic_cast<const osg::Vec3Array*>(geometry->getVertexArray());
        assert(vertices);

        assert(normals->getBinding() == osg::Array::Binding::BIND_PER_VERTEX &&
               "// TODO: Support other binding mods");

        osg::Matrix world = osg::computeWorldToLocal(this->getNodePath());
        for (unsigned int it = 0; it < vertices->size(); ++it) {
          const osg::Vec3& lineBegin = (*vertices)[it] * world;
          const osg::Vec3& lineEnd   = lineBegin + (*normals)[it] * magnitude;

          normalList.push_back(lineBegin);
          normalList.push_back(lineEnd);
        }
      } else {
        osg::notify() << "Skipping non-geometry Drawable:"
                      << childDrawable->getName() << "\n";
      }
    }
    traverse(geode);
  }

  osg::ref_ptr<osg::Geode> CollectNormalsVisitor::toNormalsGeomtery(
    osg::Vec4 color)
  {
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    vertices->insert(vertices->begin(), normalList.begin(), normalList.end());

    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    colors->push_back(color);

    osg::ref_ptr<osg::Geometry> norms = new osg::Geometry;
    norms->setVertexArray(vertices);
    norms->setColorArray(colors);
    norms->setColorBinding(osg::Geometry::BIND_OVERALL);

    norms->addPrimitiveSet(new osg::DrawArrays(GL_LINES, 0, vertices->size()));

    osg::ref_ptr<osg::Geode> leaf = new osg::Geode;
    // Make sure to deactivate the lighting
    leaf->getOrCreateStateSet()->setMode(
      GL_LIGHTING, osg::StateAttribute::OFF | osg::StateAttribute::PROTECTED);
    leaf->addDrawable(norms);
    return leaf;
  }

} // ose
