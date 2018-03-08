static char const*
GlModeToName(GLenum m)
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
    case PrimitiveSet::Mode::TRIANGLES_ADJACENCY: return "TRIANGLES_ADJACENCY";
    case PrimitiveSet::Mode::TRIANGLE_STRIP_ADJACENCY:
      return "TRIANGLE_STRIP_ADJACENCY";
    case PrimitiveSet::Mode::PATCHES: return "PATCHES";
  }
  assert(false && "unknown GL mode");
  return "Unknown";
}

static char const*
PrimitiveTypeToName(osg::PrimitiveSet::Type type)
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

static char const*
BindingToName(const osg::Array* array)
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
