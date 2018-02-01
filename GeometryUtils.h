
#include <osg/Geometry>
#include <osg/Group>

#include <osgParticle/ParticleSystem>

namespace Soleil {

  osg::ref_ptr<osg::Group> ExplodeGeometry(osg::Geometry& geom);
  osg::ref_ptr<osg::Node> ExplodeGeometry();

  void MeshSetLineMode(osg::Node& node);

  osgParticle::ParticleSystem *create_simple_particle_system(osg::Group *root);

  
}  // Soleil
