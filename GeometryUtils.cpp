
#include "GeometryUtils.h"

#include <osg/BlendFunc>
#include <osg/MatrixTransform>
#include <osg/Point>
#include <osg/PointSprite>
#include <osg/PolygonMode>
#include <osg/PrimitiveSet>
#include <osg/TemplatePrimitiveFunctor>
#include <osg/Texture2D>
#include <osg/io_utils>
#include <osgDB/ReadFile>
#include <osgParticle/AccelOperator>
#include <osgParticle/ExplosionOperator>
#include <osgParticle/ModularEmitter>
#include <osgParticle/ParticleSystem>
#include <osgParticle/ParticleSystemUpdater>
#include <osgParticle/RadialShooter>
#include <osgParticle/SectorPlacer>

#include <cassert>
#include <iostream>
#include <queue>

namespace Soleil {

  struct FaceFunctor
  {

    osg::ref_ptr<osg::Geometry> current;

    void operator()(const osg::Vec3, bool)
    {
      assert(false && "Not supported yet");
    }

    void operator()(const osg::Vec3 v1, const osg::Vec3 v2, const osg::Vec3 v3,
                    bool /*eatVertexDataAsTemporary*/)
    {
    }

    void operator()(const osg::Vec3, const osg::Vec3, bool)
    {
      assert(false && "Not supported yet");
    }

    void operator()(const osg::Vec3, const osg::Vec3, const osg::Vec3,
                    const osg::Vec3, bool)
    {
      assert(false && "Not supported yet");
    }
  };

  // TODO: Test:

  class TimeUniformCallback : public osg::Uniform::Callback
  {
  public:
    float previousFrame;

    TimeUniformCallback()
      : previousFrame(0.0f)
    {
    }

    void operator()(osg::Uniform* uniform, osg::NodeVisitor* nv)
    {
      const osg::FrameStamp* fs = nv->getFrameStamp();
      if (!fs) return;
      // float angle = osg::inDegrees((float)fs->getFrameNumber());
      // uniform->set(osg::Vec3(20.0f * cosf(angle), 20.0f * sinf(angle),
      // 1.0f));
      if (previousFrame == 0.0f) {
        previousFrame = (float)fs->getFrameNumber();
      }

      float time = (float)fs->getFrameNumber() - previousFrame;
      uniform->set(time);
    }
  };

  osg::ref_ptr<osg::Group> ExplodeGeometry(osg::Geometry& geometry)
  {
#if 0
    const osg::Geometry::PrimitiveSetList primitives =
      geometry.getPrimitiveSetList();
    for (const osg::ref_ptr<osg::PrimitiveSet> primitive : primitives) {
    }

    osg::TemplatePrimitiveFunctor<FaceFunctor> f;
    geometry.accept(f);

    assert(geometry.getPrimitiveSetList().size() == 1 &&
           "Currently only one primitive type is supported");
    assert(geometry.getPrimitiveSetList()[0]->getMode() ==
             osg::PrimitiveSet::Mode::TRIANGLES &&
           "Currently only one TRIANGLES type is supported");

#elif 0

    static const char* vertSource = {
      "// gl2_VertexShader\n"
      "#ifdef GL_ES\n"
      "    precision highp float;\n"
      "#endif\n"
      "uniform float time;\n"
      "varying vec2 texCoord;\n"
      "varying vec4 vertexColor;\n"
      "void main(void)\n"
      "{\n"
      "    mat4 particleTranslation;\n"
      //"    particleTranslation[3] = vec4(gl_Normal * time, 1.0);\n"
      // "    particleTranslation[3] = vec4(vec3(1.0) * time, 1.0);\n"
      // "    gl_Position = gl_ModelViewProjectionMatrix * particleTranslation *
      // "
      // "gl_Vertex;\n"
      "    gl_Position = gl_ModelViewProjectionMatrix * "
      "vec4(vec3(gl_NormalMatrix * gl_Normal * time), 1.0) * "
      "gl_Vertex;\n"
      "    texCoord = gl_MultiTexCoord0.xy;\n"
      "    vertexColor = gl_Color; \n"
      "}\n"
      // "\n"
      // "void main()\n"
      // "{\n"
      // "gl_Position = ftransform();\n"
      // "}\n"
      // "\n"
      // "\n"
      // "\n"
      // "\n"
      // "\n"
      // "\n"
      // "\n"
      "\n"};

    static const char* gl2_FragmentShader = {
      "// gl2_FragmentShader\n"
      "#ifdef GL_ES\n"
      "    precision highp float;\n"
      "#endif\n"
      "uniform sampler2D baseTexture;\n"
      "varying vec2 texCoord;\n"
      "varying vec4 vertexColor;\n"
      "void main(void)\n"
      "{\n"
      "    gl_FragColor = vertexColor * texture2D(baseTexture, texCoord);\n"
      "}\n"};

    osg::StateSet* stateset = geometry.getOrCreateStateSet();

    // osg::Program* p = dynamic_cast<osg::Program*>(
    //   stateset->getAttribute(osg::StateAttribute::PROGRAM));
    // assert(p);

    osg::ref_ptr<osg::Program> program = new osg::Program;
    program->addShader(new osg::Shader(osg::Shader::VERTEX, vertSource));
    program->addShader(
      new osg::Shader(osg::Shader::FRAGMENT, gl2_FragmentShader));
    // program->addShader(p->getShader(1));
    stateset->setAttributeAndModes(program);

    osg::ref_ptr<osg::Uniform> time = new osg::Uniform("time", 0.0f);
    time->setUpdateCallback(new TimeUniformCallback);
    stateset->addUniform(time);

#elif 0
    const osg::Geometry::PrimitiveSetList primitives =
      geometry.getPrimitiveSetList();
    for (const osg::ref_ptr<osg::PrimitiveSet> primitive : primitives) {
      std::cout << "Working on primitive mode: " << primitive->getMode()
                << "\n";

      osg::DrawElements* de = dynamic_cast<osg::DrawElements*>(primitive.get());
      assert(de);

      osg::ref_ptr<osg::Vec3Array> vertices(new osg::Vec3Array);
      osg::ref_ptr<osg::Vec3Array> normals(new osg::Vec3Array);
      osg::ref_ptr<osg::Vec2Array> textures(new osg::Vec2Array);

      std::queue<osg::Vec3f> face;
      for (unsigned int i = 0; i < primitive->getNumPrimitives(); ++i) {
        osg::Vec3Array* vertices =
          dynamic_cast<osg::Vec3Array*>(geometry.getVertexArray());
        assert(vertices);

        std::cout << "iterator " << i << ": primitive->getElement(" << i
                  << ") = " << de->getElement(i) << "->\t"
                  << "Vertex=" << vertices->at(de->getElement(i)) << "\n";
        face.push(vertices->at(de->getElement(i)));

        if (i % 3 &&
            primitive->getMode() == osg::PrimitiveSet::Mode::TRIANGLES) {
          // TODO: Create new object or particle
        }
      }
    }
#elif 1
    static osg::ref_ptr<osgParticle::ParticleSystem> ps;

    if (not ps) {
      ps = new osgParticle::ParticleSystem;

      ps->getDefaultParticleTemplate().setShape(osgParticle::Particle::POINT);

      osg::ref_ptr<osg::BlendFunc> blendFunc = new osg::BlendFunc;
      blendFunc->setFunction(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      osg::ref_ptr<osg::Texture2D> texture = new osg::Texture2D;
      texture->setImage(osgDB::readImageFile("Images/smoke.rgb"));

      osg::StateSet* ss = ps->getOrCreateStateSet();
      ss->setAttributeAndModes(blendFunc.get());
      ss->setTextureAttributeAndModes(0, texture.get());
      ss->setAttribute(new osg::Point(20.0f));
      ss->setTextureAttributeAndModes(0, new osg::PointSprite);
      ss->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
      ss->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
    }

    osg::ref_ptr<osgParticle::RandomRateCounter> rrc =
      new osgParticle::RandomRateCounter;
    rrc->setRateRange(800, 800);

    osg::ref_ptr<osgParticle::ModularEmitter> emitter =
      new osgParticle::ModularEmitter;
    emitter->setParticleSystem(ps);
    emitter->setCounter(rrc);
    emitter->setEndless(false);
    emitter->setLifeTime(.10f);

    emitter->setPlacer(new osgParticle::SectorPlacer);
    emitter->setShooter(new osgParticle::RadialShooter);

    float                      _scale = 50.0f;
    osgParticle::SectorPlacer* placer =
      dynamic_cast<osgParticle::SectorPlacer*>(emitter->getPlacer());
    if (placer) {
      // placer->setCenter(_position);
      placer->setRadiusRange(0.0f * _scale, 0.25f * _scale);
    }

    osgParticle::RadialShooter* shooter =
      dynamic_cast<osgParticle::RadialShooter*>(emitter->getShooter());
    if (shooter) {
      shooter->setThetaRange(0.0f, osg::PI * 2.0f);
      shooter->setInitialSpeedRange(1.0f * _scale, 10.0f * _scale);
    }

#if 0
    osg::ref_ptr<osgParticle::AccelOperator> accel =
      new osgParticle::AccelOperator;
    accel->setToGravity();
     program->addOperator(accel);
#elif 0
    osg::ref_ptr<osgParticle::ExplosionOperator> accel =
      new osgParticle::ExplosionOperator;
    // accel->setEnabled(true);
    // accel->setMagnitude(100.5f);
    // accel->setRadius(306.14f);
    // accel->setEpsilon(100.5f);
    // accel->setSigma(100.5f);
    accel->setCenter(osg::Vec3(10.0f, 10.0f, -100.0f));
    program->addOperator(accel);
#endif
    osg::ref_ptr<osgParticle::ModularProgram> program =
      new osgParticle::ModularProgram;
    program->setParticleSystem(ps);

    static osg::ref_ptr<osg::MatrixTransform> parent;

    if (parent == nullptr) {
      parent = new osg::MatrixTransform;

      // osg::ref_ptr<osg::MatrixTransform> parent2 = new osg::MatrixTransform;

      osg::ref_ptr<osg::Geode> geode = new osg::Geode;
      geode->addDrawable(ps);
      parent->addChild(emitter);
      parent->addChild(program);
      parent->addChild(geode);

      // parent->addChild(parent2);

      osg::ref_ptr<osgParticle::ParticleSystemUpdater> updater =
        new osgParticle::ParticleSystemUpdater;
      updater->addParticleSystem(ps);
      parent->addChild(updater);

      // MeshSetLineMode(geometry);

      return parent;
    }
    return nullptr;

#elif 1
    // Ok folks, this is the first particle system we build; it will be
    // very simple, with no textures and no special effects, just default
    // values except for a couple of attributes.

    // First of all, we create the ParticleSystem object; it will hold
    // our particles and expose the interface for managing them; this object
    // is a Drawable, so we'll have to add it to a Geode later.

    osgParticle::ParticleSystem* ps = new osgParticle::ParticleSystem;

    // As for other Drawable classes, the aspect of graphical elements of
    // ParticleSystem (the particles) depends on the StateAttribute's we
    // give it. The ParticleSystem class has an helper function that let
    // us specify a set of the most common attributes: setDefaultAttributes().
    // This method can accept up to three parameters; the first is a texture
    // name (std::string), which can be empty to disable texturing, the second
    // sets whether particles have to be "emissive" (additive blending) or not;
    // the third parameter enables or disables lighting.

    ps->setDefaultAttributes("", true, false);

    // Now that our particle system is set we have to create an emitter, that is
    // an object (actually a Node descendant) that generate new particles at
    // each frame. The best choice is to use a ModularEmitter, which allow us to
    // achieve a wide variety of emitting styles by composing the emitter using
    // three objects: a "counter", a "placer" and a "shooter". The counter must
    // tell the ModularEmitter how many particles it has to create for the
    // current frame; then, the ModularEmitter creates these particles, and for
    // each new particle it instructs the placer and the shooter to set its
    // position vector and its velocity vector, respectively.
    // By default, a ModularEmitter object initializes itself with a counter of
    // type RandomRateCounter, a placer of type PointPlacer and a shooter of
    // type RadialShooter (see documentation for details). We are going to leave
    // these default objects there, but we'll modify the counter so that it
    // counts faster (more particles are emitted at each frame).

    osgParticle::ModularEmitter* emitter = new osgParticle::ModularEmitter;

    // the first thing you *MUST* do after creating an emitter is to set the
    // destination particle system, otherwise it won't know where to create
    // new particles.

    emitter->setParticleSystem(ps);

    // Ok, get a pointer to the emitter's Counter object. We could also
    // create a new RandomRateCounter object and assign it to the emitter,
    // but since the default counter is already a RandomRateCounter, we
    // just get a pointer to it and change a value.

    osgParticle::RandomRateCounter* rrc =
      static_cast<osgParticle::RandomRateCounter*>(emitter->getCounter());

    // Now set the rate range to a better value. The actual rate at each frame
    // will be chosen randomly within that range.

    rrc->setRateRange(20, 30); // generate 20 to 30 particles per second

    // The emitter is done! Let's add it to the scene graph. The cool thing is
    // that any emitter node will take into account the accumulated
    // local-to-world
    // matrix, so you can attach an emitter to a transform node and see it move.

    osg::ref_ptr<osg::MatrixTransform> parent = new osg::MatrixTransform;

    parent->addChild(emitter);

    // Ok folks, we have almost finished. We don't add any particle modifier
    // here (see ModularProgram and Operator classes), so all we still need is
    // to create a Geode and add the particle system to it, so it can be
    // displayed.

    // add the ParticleSystem to the scene graph
    parent->addChild(ps);

    osgParticle::ParticleSystemUpdater* psu =
      new osgParticle::ParticleSystemUpdater;
    psu->addParticleSystem(ps);
    parent->addChild(psu);

    return parent;
#endif
    // return nullptr;
  }

  void MeshSetLineMode(osg::Node& node)
  {
    // boxes->getOrCreateStateSet()->setMode(GL_LIGHTING,
    //                                       osg::StateAttribute::OFF);
    // boxes->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
    // boxes->getOrCreateStateSet()->setRenderingHint(
    //   osg::StateSet::TRANSPARENT_BIN);
    // boxes->setDataVariance(osg::Object::DataVariance::DYNAMIC);
    node.getOrCreateStateSet()->setAttributeAndModes(new osg::PolygonMode(
      osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::LINE));
  }

  osgParticle::ParticleSystem* create_simple_particle_system(osg::Group* root)
  {

    // Ok folks, this is the first particle system we build; it will be
    // very simple, with no textures and no special effects, just default
    // values except for a couple of attributes.

    // First of all, we create the ParticleSystem object; it will hold
    // our particles and expose the interface for managing them; this object
    // is a Drawable, so we'll have to add it to a Geode later.

    osgParticle::ParticleSystem* ps = new osgParticle::ParticleSystem;

    // As for other Drawable classes, the aspect of graphical elements of
    // ParticleSystem (the particles) depends on the StateAttribute's we
    // give it. The ParticleSystem class has an helper function that let
    // us specify a set of the most common attributes: setDefaultAttributes().
    // This method can accept up to three parameters; the first is a texture
    // name (std::string), which can be empty to disable texturing, the second
    // sets whether particles have to be "emissive" (additive blending) or not;
    // the third parameter enables or disables lighting.

    ps->setDefaultAttributes("", true, false);

    // Now that our particle system is set we have to create an emitter, that is
    // an object (actually a Node descendant) that generate new particles at
    // each frame. The best choice is to use a ModularEmitter, which allow us to
    // achieve a wide variety of emitting styles by composing the emitter using
    // three objects: a "counter", a "placer" and a "shooter". The counter must
    // tell the ModularEmitter how many particles it has to create for the
    // current frame; then, the ModularEmitter creates these particles, and for
    // each new particle it instructs the placer and the shooter to set its
    // position vector and its velocity vector, respectively.
    // By default, a ModularEmitter object initializes itself with a counter of
    // type RandomRateCounter, a placer of type PointPlacer and a shooter of
    // type RadialShooter (see documentation for details). We are going to leave
    // these default objects there, but we'll modify the counter so that it
    // counts faster (more particles are emitted at each frame).

    osgParticle::ModularEmitter* emitter = new osgParticle::ModularEmitter;

    // the first thing you *MUST* do after creating an emitter is to set the
    // destination particle system, otherwise it won't know where to create
    // new particles.

    emitter->setParticleSystem(ps);

    // Ok, get a pointer to the emitter's Counter object. We could also
    // create a new RandomRateCounter object and assign it to the emitter,
    // but since the default counter is already a RandomRateCounter, we
    // just get a pointer to it and change a value.

    osgParticle::RandomRateCounter* rrc =
      static_cast<osgParticle::RandomRateCounter*>(emitter->getCounter());

    // Now set the rate range to a better value. The actual rate at each frame
    // will be chosen randomly within that range.

    rrc->setRateRange(20, 30); // generate 20 to 30 particles per second

    // The emitter is done! Let's add it to the scene graph. The cool thing is
    // that any emitter node will take into account the accumulated
    // local-to-world
    // matrix, so you can attach an emitter to a transform node and see it move.

    root->addChild(emitter);

    // Ok folks, we have almost finished. We don't add any particle modifier
    // here (see ModularProgram and Operator classes), so all we still need is
    // to create a Geode and add the particle system to it, so it can be
    // displayed.

    // add the ParticleSystem to the scene graph
    root->addChild(ps);

    return ps;
  }

  osg::ref_ptr<osg::Node> ExplodeGeometry()
  {
    constexpr float                                  scale = 1.0f;
    static osg::ref_ptr<osgParticle::ParticleSystem> ps;

    if (not ps) {
      ps = new osgParticle::ParticleSystem;

      ps->setDefaultAttributes("Images/smoke.rgb", false, false);

      float radius  = 0.4f * scale;
      float density = 1.2f; // 1.0kg/m^3

      auto& defaultParticleTemplate = ps->getDefaultParticleTemplate();
      defaultParticleTemplate.setLifeTime(1.0 + 0.1 * scale);
      defaultParticleTemplate.setSizeRange(osgParticle::rangef(0.75f, 3.0f));
      defaultParticleTemplate.setAlphaRange(osgParticle::rangef(0.1f, 1.0f));
      defaultParticleTemplate.setColorRange(osgParticle::rangev4(
        osg::Vec4(1.0f, 0.8f, 0.2f, 1.0f), osg::Vec4(1.0f, 0.4f, 0.1f, 0.0f)));
      defaultParticleTemplate.setRadius(radius);
      defaultParticleTemplate.setMass(density * radius * radius * radius *
                                      osg::PI * 4.0f / 3.0f);

      // ps->getDefaultParticleTemplate().setShape(osgParticle::Particle::POINT);

      // osg::ref_ptr<osg::BlendFunc> blendFunc = new osg::BlendFunc;
      // blendFunc->setFunction(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      // osg::ref_ptr<osg::Texture2D> texture = new osg::Texture2D;
      // texture->setImage(osgDB::readImageFile("Images/smoke.rgb"));

      // osg::StateSet* ss = ps->getOrCreateStateSet();
      // ss->setAttributeAndModes(blendFunc.get());
      // ss->setTextureAttributeAndModes(0, texture.get());
      // ss->setAttribute(new osg::Point(20.0f));
      // ss->setTextureAttributeAndModes(0, new osg::PointSprite);
      // ss->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
      // ss->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
    }

    osg::ref_ptr<osgParticle::RandomRateCounter> rrc =
      new osgParticle::RandomRateCounter;
    rrc->setRateRange(800, 1000);

    osg::ref_ptr<osgParticle::ModularEmitter> emitter =
      new osgParticle::ModularEmitter;
    emitter->setParticleSystem(ps);
    emitter->setCounter(rrc);
    emitter->setEndless(false);
    emitter->setLifeTime(.10f);

    emitter->setPlacer(new osgParticle::SectorPlacer);
    emitter->setShooter(new osgParticle::RadialShooter);

    osgParticle::SectorPlacer* placer =
      dynamic_cast<osgParticle::SectorPlacer*>(emitter->getPlacer());
    if (placer) {
      //placer->setCenter(osg::Vec3(0, 0, 60));
      placer->setRadiusRange(0.0f * scale, 0.25f * scale);
    }

    osgParticle::RadialShooter* shooter =
      dynamic_cast<osgParticle::RadialShooter*>(emitter->getShooter());
    if (shooter) {
      shooter->setThetaRange(0.0f, osg::PI * 2.0f);
      shooter->setInitialSpeedRange(1.0f * scale, 10.0f * scale);
    }

#if 0
    osg::ref_ptr<osgParticle::AccelOperator> accel =
      new osgParticle::AccelOperator;
    accel->setToGravity();
     program->addOperator(accel);
#elif 0
    osg::ref_ptr<osgParticle::ExplosionOperator> accel =
      new osgParticle::ExplosionOperator;
    // accel->setEnabled(true);
    // accel->setMagnitude(100.5f);
    // accel->setRadius(306.14f);
    // accel->setEpsilon(100.5f);
    // accel->setSigma(100.5f);
    accel->setCenter(osg::Vec3(10.0f, 10.0f, -100.0f));
    program->addOperator(accel);
#endif
    
    static osg::ref_ptr<osg::MatrixTransform> parent;

    if (parent == nullptr) {
      parent = new osg::MatrixTransform;

      osg::ref_ptr<osgParticle::ModularProgram> program =
        new osgParticle::ModularProgram;
      program->setParticleSystem(ps);

      // osg::ref_ptr<osg::MatrixTransform> parent2 = new osg::MatrixTransform;

      osg::ref_ptr<osg::Geode> geode = new osg::Geode;
      geode->addDrawable(ps);
      parent->addChild(emitter);
      parent->addChild(program);
      parent->addChild(geode);

      // parent->addChild(parent2);

      osg::ref_ptr<osgParticle::ParticleSystemUpdater> updater =
        new osgParticle::ParticleSystemUpdater;
      updater->addParticleSystem(ps);
      parent->addChild(updater);

      // MeshSetLineMode(geometry);

      return parent;
    }

    parent->addChild(emitter);

    return nullptr;
  }

} // Soleil
