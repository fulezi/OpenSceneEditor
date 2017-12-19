#include "ImGuiHandler.hpp"

#include "imgui/imgui.h"

#include <iostream>

#include <osgUtil/SceneView>
#include <osgUtil/UpdateVisitor>

#include <osg/BlendEquation>
#include <osg/BlendFunc>
#include <osg/Camera>
#include <osg/CullFace>
#include <osg/PolygonMode>

#include <osg/GLExtensions>
#include <osg/Texture1D>
#include <osg/Texture2D>

#include <iterator>

static GLuint g_FontTexture  = 0;
static int    g_ShaderHandle = 0, g_VertHandle = 0, g_FragHandle = 0;
static int    g_AttribLocationTex = 0, g_AttribLocationProjMtx = 0;
static int    g_AttribLocationPosition = 0, g_AttribLocationUV = 0,
           g_AttribLocationColor = 0;
static unsigned int g_VboHandle = 0, g_VaoHandle = 0, g_ElementsHandle = 0;

//////////////////////////////////////////////////////////////////////////////
// Imporant Note: Dear ImGui expects the control Keys indices not to be	    //
// greater thant 511. It actually uses an array of 512 elements. However,   //
// OSG has indices greater than that. So here I do a conversion for special //
// keys between ImGui and OSG.						    //
//////////////////////////////////////////////////////////////////////////////

/**
 * Special keys that are usually greater than 512 in OSGga
 **/
enum ConvertedKey : int
{
  ConvertedKey_Tab = 257,
  ConvertedKey_Left,
  ConvertedKey_Right,
  ConvertedKey_Up,
  ConvertedKey_Down,
  ConvertedKey_PageUp,
  ConvertedKey_PageDown,
  ConvertedKey_Home,
  ConvertedKey_End,
  ConvertedKey_Delete,
  ConvertedKey_BackSpace,
  ConvertedKey_Enter,
  ConvertedKey_Escape,
  // Modifiers
  ConvertedKey_LeftControl,
  ConvertedKey_RightControl,
  ConvertedKey_LeftShift,
  ConvertedKey_RightShift,
  ConvertedKey_LeftAlt,
  ConvertedKey_RightAlt,
  ConvertedKey_LeftSuper,
  ConvertedKey_RightSuper
};

/**
 * Check for a special key and return the converted code (range [257, 511]) if
 * so. Otherwise returns -1
 */
static int
ConvertFromOSGKey(int key)
{
  using KEY = osgGA::GUIEventAdapter::KeySymbol;

  switch (key) {
    default: // Not found
      return -1;
    case KEY::KEY_Tab: return ConvertedKey_Tab;
    case KEY::KEY_Left: return ConvertedKey_Left;
    case KEY::KEY_Right: return ConvertedKey_Right;
    case KEY::KEY_Up: return ConvertedKey_Up;
    case KEY::KEY_Down: return ConvertedKey_Down;
    case KEY::KEY_Page_Up: return ConvertedKey_PageUp;
    case KEY::KEY_Page_Down: return ConvertedKey_PageDown;
    case KEY::KEY_Home: return ConvertedKey_Home;
    case KEY::KEY_End: return ConvertedKey_End;
    case KEY::KEY_Delete: return ConvertedKey_Delete;
    case KEY::KEY_BackSpace: return ConvertedKey_BackSpace;
    case KEY::KEY_Return: return ConvertedKey_Enter;
    case KEY::KEY_Escape: return ConvertedKey_Escape;

    case KEY::KEY_Control_L: return ConvertedKey_LeftControl;
    case KEY::KEY_Control_R: return ConvertedKey_RightControl;
    case KEY::KEY_Shift_L: return ConvertedKey_LeftShift;
    case KEY::KEY_Shift_R: return ConvertedKey_RightShift;
    case KEY::KEY_Alt_L: return ConvertedKey_LeftAlt;
    case KEY::KEY_Alt_R: return ConvertedKey_RightAlt;
    case KEY::KEY_Super_L: return ConvertedKey_LeftSuper;
    case KEY::KEY_Super_R: return ConvertedKey_RightSuper;
  }
  assert(false && "Switch has a default case");
  return -1;
}

ImGUIEventHandler::ImGUIEventHandler()
  : time(0.0)
  , mousePressed{false, false, false}
  , mouseWheel(0.0)
  , fontTexture(0)
  , initialized(false)
{
}

void
ImGUIEventHandler::newFrame(osg::RenderInfo& renderInfo)
{
  ImGuiIO& io = ImGui::GetIO();
  if (initialized == false) {
    initialize(renderInfo);
  }
  osg::Viewport* viewport = renderInfo.getCurrentCamera()->getViewport();
  if (viewport == nullptr) {
    // TODO Get values from viewport
    viewport = renderInfo.getView()->getCamera()->getViewport();
  }
  assert(viewport && "No viewport set");
#if 0
  std::cout << "---------------------------------------"
            << renderInfo.getCurrentCamera()->getName()
            << "---------------------------------------"
            << renderInfo.getCurrentCamera()->getViewport() << "\n";
#endif
  io.DisplaySize = ImVec2(viewport->width(), viewport->height());
  // io.DisplaySize = ImVec2(1920, 1080);

  double aCurrentTime =
    renderInfo.getView()->getFrameStamp()->getSimulationTime();
  io.DeltaTime =
    time > 0.0 ? (float)(aCurrentTime - time) : (float)(1.0f / 60.0f);
  time = aCurrentTime;

  for (int i = 0; i < 3; i++) {
    io.MouseDown[i] = mousePressed[i];
  }

  io.MouseWheel = mouseWheel;
  mouseWheel    = 0.0f;

  ImGui::NewFrame();

  // TODO: Remove:
  bool show_test_window = true;
  ImGui::ShowTestWindow(&show_test_window);
}

bool
ImGUIEventHandler::handle(const osgGA::GUIEventAdapter& eventAdapter,
                          osgGA::GUIActionAdapter& /*actionAdapter*/,
                          osg::Object* /*object*/,
                          osg::NodeVisitor* /*nodeVisitor*/)
{
  const bool wantCapureMouse    = ImGui::GetIO().WantCaptureMouse;
  const bool wantCapureKeyboard = ImGui::GetIO().WantCaptureKeyboard;
  ImGuiIO&   io                 = ImGui::GetIO();

  switch (eventAdapter.getEventType()) {
    case osgGA::GUIEventAdapter::KEYDOWN: {
      const int c           = eventAdapter.getKey();
      const int special_key = ConvertFromOSGKey(c);
      if (special_key > 0) {
        assert(special_key < 512 && "ImGui KeysDown is an array of 512");
        assert(special_key > 256 &&
               "ASCII stop at 127, but we use the range [257, 511]");

        io.KeysDown[special_key] = true;

        io.KeyCtrl = io.KeysDown[ConvertedKey_LeftControl] ||
                     io.KeysDown[ConvertedKey_RightControl];
        io.KeyShift = io.KeysDown[ConvertedKey_LeftShift] ||
                      io.KeysDown[ConvertedKey_RightShift];
        io.KeyAlt = io.KeysDown[ConvertedKey_LeftAlt] ||
                    io.KeysDown[ConvertedKey_RightAlt];
        io.KeySuper = io.KeysDown[ConvertedKey_LeftSuper] ||
                      io.KeysDown[ConvertedKey_RightSuper];
      } else if (c > 0 && c < 0x10000) {
        io.AddInputCharacter((unsigned short)c);
      }
      return wantCapureKeyboard;
    }
    case osgGA::GUIEventAdapter::KEYUP: {
      const int c           = eventAdapter.getKey();
      const int special_key = ConvertFromOSGKey(c);
      if (special_key > 0) {
        assert(special_key < 512 && "ImGui KeysMap is an array of 512");
        assert(special_key > 256 &&
               "ASCII stop at 127, but we use the range [257, 511]");

        io.KeysDown[special_key] = false;

        io.KeyCtrl = io.KeysDown[ConvertedKey_LeftControl] ||
                     io.KeysDown[ConvertedKey_RightControl];
        io.KeyShift = io.KeysDown[ConvertedKey_LeftShift] ||
                      io.KeysDown[ConvertedKey_RightShift];
        io.KeyAlt = io.KeysDown[ConvertedKey_LeftAlt] ||
                    io.KeysDown[ConvertedKey_RightAlt];
        io.KeySuper = io.KeysDown[ConvertedKey_LeftSuper] ||
                      io.KeysDown[ConvertedKey_RightSuper];
      }
      return wantCapureKeyboard;
    }
    case (osgGA::GUIEventAdapter::PUSH): {
      ImGuiIO& io = ImGui::GetIO();
      io.MousePos =
        ImVec2(eventAdapter.getX(), io.DisplaySize.y - eventAdapter.getY());
      mousePressed[0] = true;
      return wantCapureMouse;
    }
    case (osgGA::GUIEventAdapter::DRAG):
    case (osgGA::GUIEventAdapter::MOVE): {
      ImGuiIO& io = ImGui::GetIO();
      io.MousePos =
        ImVec2(eventAdapter.getX(), io.DisplaySize.y - eventAdapter.getY());
      return wantCapureMouse;
    }
    case (osgGA::GUIEventAdapter::RELEASE): {
      mousePressed[0] = false;
      return wantCapureMouse;
    }
    case (osgGA::GUIEventAdapter::SCROLL): {
#if 0
      // TODO: This does not work for me:
      mouseWheel = eventAdapter.getScrollingDeltaY();
#else
      if (eventAdapter.getScrollingMotion() ==
          osgGA::GUIEventAdapter::ScrollingMotion::SCROLL_UP) {
        mouseWheel += 1.0f;
      } else if (eventAdapter.getScrollingMotion() ==
                 osgGA::GUIEventAdapter::ScrollingMotion::SCROLL_DOWN) {
        mouseWheel -= 1.0f;
      }
#endif
      return wantCapureMouse;
    }
      //   default: {
      //   return false;
      // }
  }
  return true; // TODO: return true value
}

static void
checkShader(GLuint shader, osg::GLExtensions* extensions)
{
  GLint isCompiled = 0;
  extensions->glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
  std::string state = (isCompiled == GL_FALSE) ? "FAILED" : "SUCCESS";

  GLint maxLength = 0;
  extensions->glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);
  // The maxLength includes the NULL character
  std::vector<GLchar> errorLog(maxLength);
  extensions->glGetShaderInfoLog(shader, maxLength, &maxLength, &errorLog[0]);

  std::string logs;
  if (maxLength > 1) {
    logs = ":" + std::string(errorLog.begin(), errorLog.end());
  }

  std::cout << "[COMPILATION] Shader compilation " << state << logs << "\n";
}

void
ImGUIEventHandler::initialize(osg::RenderInfo& renderInfo)
{
  const GLchar* vertex_shader =
    "#version 100\n"
    "precision lowp float;\n"
    "uniform mat4 ProjMtx;\n"
    "attribute vec2 Position;\n"
    "attribute vec2 UV;\n"
    "attribute vec4 Color;\n"
    "varying vec2 Frag_UV;\n"
    "varying vec4 Frag_Color;\n"
    "void main()\n"
    "{\n"
    "	Frag_UV = UV;\n"
    "	Frag_Color = Color;\n"
    "	gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
    "}\n";

  const GLchar* fragment_shader =
    "#version 100\n"
    "precision lowp float;\n"
    "uniform sampler2D Texture;\n"
    "varying vec2 Frag_UV;\n"
    "varying vec4 Frag_Color;\n"
    "void main()\n"
    "{\n"
    "	 gl_FragColor = Frag_Color * texture2D(Texture, Frag_UV.st);\n"
    "}\n";

  osg::State&        state      = *renderInfo.getState();
  osg::GLExtensions* extensions = state.get<osg::GLExtensions>();
// Make sure to build with a recent osg

#if 0 // OSG version
  osg::ref_ptr<osg::Shader> vertShader =
    new osg::Shader(osg::Shader::VERTEX, vertex_shader);
  osg::ref_ptr<osg::Shader> fragShader =
    new osg::Shader(osg::Shader::FRAGMENT, fragment_shader);
  osg::ref_ptr<osg::Program> program = new osg::Program;
  program->addShader(vertShader.get());
  program->addShader(fragShader.get());

  program->addBindAttribLocation("Position", 0);
  program->addBindAttribLocation("UV", 1);
  program->addBindAttribLocation("Color", 2);

#else
  g_ShaderHandle = extensions->glCreateProgram();
  g_VertHandle   = extensions->glCreateShader(GL_VERTEX_SHADER);
  g_FragHandle   = extensions->glCreateShader(GL_FRAGMENT_SHADER);
  extensions->glShaderSource(g_VertHandle, 1, &vertex_shader, 0);
  extensions->glShaderSource(g_FragHandle, 1, &fragment_shader, 0);
  extensions->glCompileShader(g_VertHandle);
  checkShader(g_VertHandle, extensions);
  extensions->glCompileShader(g_FragHandle);
  checkShader(g_FragHandle, extensions);
  extensions->glAttachShader(g_ShaderHandle, g_VertHandle);
  extensions->glAttachShader(g_ShaderHandle, g_FragHandle);

  extensions->glBindAttribLocation(g_ShaderHandle, 0, "Position");
  extensions->glBindAttribLocation(g_ShaderHandle, 1, "UV");
  extensions->glBindAttribLocation(g_ShaderHandle, 2, "Color");

  extensions->glLinkProgram(g_ShaderHandle);

  g_AttribLocationTex =
    extensions->glGetUniformLocation(g_ShaderHandle, "Texture");
  g_AttribLocationProjMtx =
    extensions->glGetUniformLocation(g_ShaderHandle, "ProjMtx");
  g_AttribLocationPosition =
    extensions->glGetAttribLocation(g_ShaderHandle, "Position");
  g_AttribLocationUV = extensions->glGetAttribLocation(g_ShaderHandle, "UV");
  g_AttribLocationColor =
    extensions->glGetAttribLocation(g_ShaderHandle, "Color");

#endif

  extensions->glGenBuffers(1, &g_VboHandle);
  extensions->glGenBuffers(1, &g_ElementsHandle);

  extensions->glGenVertexArrays(1, &g_VaoHandle);
  extensions->glBindVertexArray(g_VaoHandle);
  extensions->glBindBuffer(GL_ARRAY_BUFFER, g_VboHandle);
  extensions->glEnableVertexAttribArray(g_AttribLocationPosition);
  extensions->glEnableVertexAttribArray(g_AttribLocationUV);
  extensions->glEnableVertexAttribArray(g_AttribLocationColor);

#define OFFSETOF(TYPE, ELEMENT) ((size_t) & (((TYPE*)0)->ELEMENT))
  extensions->glVertexAttribPointer(g_AttribLocationPosition, 2, GL_FLOAT,
                                    GL_FALSE, sizeof(ImDrawVert),
                                    (GLvoid*)OFFSETOF(ImDrawVert, pos));
  extensions->glVertexAttribPointer(g_AttribLocationUV, 2, GL_FLOAT, GL_FALSE,
                                    sizeof(ImDrawVert),
                                    (GLvoid*)OFFSETOF(ImDrawVert, uv));
  extensions->glVertexAttribPointer(g_AttribLocationColor, 4, GL_UNSIGNED_BYTE,
                                    GL_TRUE, sizeof(ImDrawVert),
                                    (GLvoid*)OFFSETOF(ImDrawVert, col));
#undef OFFSETOF

  // Build texture atlas
  ImGuiIO&       io = ImGui::GetIO();
  unsigned char* pixels;
  int            width, height;
  io.Fonts->GetTexDataAsRGBA32(
    &pixels, &width,
    &height); // Load as RGBA 32-bits (75% of the memory is wasted, but default
              // font is so small) because it is more likely to be compatible
              // with user's existing shaders. If your ImTextureId represent a
              // higher-level concept than just a GL texture id, consider
              // calling GetTexDataAsAlpha8() instead to save on GPU memory.

  // Upload texture to graphics system
  glGenTextures(1, &g_FontTexture);
  glBindTexture(GL_TEXTURE_2D, g_FontTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, pixels);

  // Store our identifier
  io.Fonts->TexID = (void*)(intptr_t)g_FontTexture;

  // Keyboard mapping. ImGui will use those indices to peek into the
  // io.KeyDown[] array.
  io.KeyMap[ImGuiKey_Tab]        = ConvertedKey_Tab;
  io.KeyMap[ImGuiKey_LeftArrow]  = ConvertedKey_Left;
  io.KeyMap[ImGuiKey_RightArrow] = ConvertedKey_Right;
  io.KeyMap[ImGuiKey_UpArrow]    = ConvertedKey_Up;
  io.KeyMap[ImGuiKey_DownArrow]  = ConvertedKey_Down;
  io.KeyMap[ImGuiKey_PageUp]     = ConvertedKey_PageUp;
  io.KeyMap[ImGuiKey_PageDown]   = ConvertedKey_PageDown;
  io.KeyMap[ImGuiKey_Home]       = ConvertedKey_Home;
  io.KeyMap[ImGuiKey_End]        = ConvertedKey_End;
  io.KeyMap[ImGuiKey_Delete]     = ConvertedKey_Delete;
  io.KeyMap[ImGuiKey_Backspace]  = ConvertedKey_BackSpace;
  io.KeyMap[ImGuiKey_Enter]      = ConvertedKey_Enter;
  io.KeyMap[ImGuiKey_Escape]     = ConvertedKey_Escape;
  io.KeyMap[ImGuiKey_A]          = osgGA::GUIEventAdapter::KeySymbol::KEY_A;
  io.KeyMap[ImGuiKey_C]          = osgGA::GUIEventAdapter::KeySymbol::KEY_C;
  io.KeyMap[ImGuiKey_V]          = osgGA::GUIEventAdapter::KeySymbol::KEY_V;
  io.KeyMap[ImGuiKey_X]          = osgGA::GUIEventAdapter::KeySymbol::KEY_X;
  io.KeyMap[ImGuiKey_Y]          = osgGA::GUIEventAdapter::KeySymbol::KEY_Y;
  io.KeyMap[ImGuiKey_Z]          = osgGA::GUIEventAdapter::KeySymbol::KEY_Z;

  initialized = true;
}

void
ImGUIRender::operator()(osg::RenderInfo& renderInfo) const
{
  ImGui::Render();
  ImDrawData* draw_data = ImGui::GetDrawData();

  osg::State&        state      = *renderInfo.getState();
  osg::GLExtensions* extensions = state.get<osg::GLExtensions>();

  ImGuiIO& io        = ImGui::GetIO();
  int      fb_width  = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
  int      fb_height = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
  if (fb_width == 0 || fb_height == 0) return;
  draw_data->ScaleClipRects(io.DisplayFramebufferScale);
  // Setup render state: alpha-blending enabled, no face culling, no depth
  // testing, scissor enabled
  glEnable(GL_BLEND);
  glBlendEquation(GL_FUNC_ADD);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_SCISSOR_TEST);

  // Setup viewport, orthographic projection matrix
  glViewport(0, 0, (GLsizei)fb_width, (GLsizei)fb_height);
  const float ortho_projection[4][4] = {
    {2.0f / io.DisplaySize.x, 0.0f, 0.0f, 0.0f},
    {0.0f, 2.0f / -io.DisplaySize.y, 0.0f, 0.0f},
    {0.0f, 0.0f, -1.0f, 0.0f},
    {-1.0f, 1.0f, 0.0f, 1.0f},
  };
  extensions->glUseProgram(g_ShaderHandle);
  extensions->glUniform1i(g_AttribLocationTex, 0);
  extensions->glUniformMatrix4fv(g_AttribLocationProjMtx, 1, GL_FALSE,
                                 &ortho_projection[0][0]);
  extensions->glBindVertexArray(g_VaoHandle);

  for (int n = 0; n < draw_data->CmdListsCount; n++) {
    const ImDrawList* cmd_list          = draw_data->CmdLists[n];
    const ImDrawIdx*  idx_buffer_offset = 0;

    extensions->glBindBuffer(GL_ARRAY_BUFFER, g_VboHandle);
    extensions->glBufferData(
      GL_ARRAY_BUFFER,
      (GLsizeiptr)cmd_list->VtxBuffer.Size * sizeof(ImDrawVert),
      (const GLvoid*)cmd_list->VtxBuffer.Data, GL_STREAM_DRAW);

    extensions->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_ElementsHandle);
    extensions->glBufferData(
      GL_ELEMENT_ARRAY_BUFFER,
      (GLsizeiptr)cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx),
      (const GLvoid*)cmd_list->IdxBuffer.Data, GL_STREAM_DRAW);

    for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
      const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
      if (pcmd->UserCallback) {
        pcmd->UserCallback(cmd_list, pcmd);
      } else {
        glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->TextureId);
        glScissor((int)pcmd->ClipRect.x, (int)(fb_height - pcmd->ClipRect.w),
                  (int)(pcmd->ClipRect.z - pcmd->ClipRect.x),
                  (int)(pcmd->ClipRect.w - pcmd->ClipRect.y));
        glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount,
                       sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT
                                              : GL_UNSIGNED_INT,
                       idx_buffer_offset);
      }
      idx_buffer_offset += pcmd->ElemCount;
    }
  }
}

// ------------------------------------------------------------------------------
// ------------------------------------------------------------------------------
// ------------------------------------------------------------------------------
// ------------------------------------------------------------------------------
// ------------------------------------------------------------------------------
// ------------------------------------------------------------------------------
// ------------------------------------------------------------------------------
// ------------------------------------------------------------------------------
// ------------------------------------------------------------------------------
// ------------------------------------------------------------------------------
// ------------------------------------------------------------------------------
// ------------------------------------------------------------------------------
// ------------------------------------------------------------------------------
// ------------------------------------------------------------------------------
// ------------------------------------------------------------------------------
// ------------------------------------------------------------------------------
// ------------------------------------------------------------------------------
// ------------------------------------------------------------------------------
// ------------------------------------------------------------------------------
// ------------------------------------------------------------------------------
// ------------------------------------------------------------------------------
// ------------------------------------------------------------------------------
// ---------------- OLD IMPLEMENTATION
// ------------------------------------------------------------------------------
#if 0


void
ImGuiHandler::init()
{

  ImGuiIO& io = ImGui::GetIO();

  // Keyboard mapping. ImGui will use those indices to peek into the
  // io.KeyDown[] array.
  io.KeyMap[ImGuiKey_Tab]        = ConvertedKey_Tab;
  io.KeyMap[ImGuiKey_LeftArrow]  = ConvertedKey_Left;
  io.KeyMap[ImGuiKey_RightArrow] = ConvertedKey_Right;
  io.KeyMap[ImGuiKey_UpArrow]    = ConvertedKey_Up;
  io.KeyMap[ImGuiKey_DownArrow]  = ConvertedKey_Down;
  io.KeyMap[ImGuiKey_PageUp]     = ConvertedKey_PageUp;
  io.KeyMap[ImGuiKey_PageDown]   = ConvertedKey_PageDown;
  io.KeyMap[ImGuiKey_Home]       = ConvertedKey_Home;
  io.KeyMap[ImGuiKey_End]        = ConvertedKey_End;
  io.KeyMap[ImGuiKey_Delete]     = ConvertedKey_Delete;
  io.KeyMap[ImGuiKey_Backspace]  = ConvertedKey_BackSpace;
  io.KeyMap[ImGuiKey_Enter]      = ConvertedKey_Enter;
  io.KeyMap[ImGuiKey_Escape]     = ConvertedKey_Escape;
  io.KeyMap[ImGuiKey_A]          = osgGA::GUIEventAdapter::KeySymbol::KEY_A;
  io.KeyMap[ImGuiKey_C]          = osgGA::GUIEventAdapter::KeySymbol::KEY_C;
  io.KeyMap[ImGuiKey_V]          = osgGA::GUIEventAdapter::KeySymbol::KEY_V;
  io.KeyMap[ImGuiKey_X]          = osgGA::GUIEventAdapter::KeySymbol::KEY_X;
  io.KeyMap[ImGuiKey_Y]          = osgGA::GUIEventAdapter::KeySymbol::KEY_Y;
  io.KeyMap[ImGuiKey_Z]          = osgGA::GUIEventAdapter::KeySymbol::KEY_Z;

  // Build texture atlas
  unsigned char* pixels;
  int            width, height;
  io.Fonts->GetTexDataAsAlpha8(&pixels, &width, &height);

  // Create OpenGL texture
  GLint last_texture;
  glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
  glGenTextures(1, &g_FontTexture);
  glBindTexture(GL_TEXTURE_2D, g_FontTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, width, height, 0, GL_ALPHA,
               GL_UNSIGNED_BYTE, pixels);

  // Store our identifier
  io.Fonts->TexID = (void*)(intptr_t)g_FontTexture;

  // Cleanup (don't clear the input data if you want to append new fonts later)
  io.Fonts->ClearInputData();
  io.Fonts->ClearTexData();
  glBindTexture(GL_TEXTURE_2D, last_texture);

  io.RenderDrawListsFn = ImGui_RenderDrawLists;
}

void
ImGuiHandler::setCameraCallbacks(osg::Camera* theCamera)
{

  ImGuiDrawCallback* aPostDrawCallback = new ImGuiDrawCallback(*this);
  theCamera->setPostDrawCallback(aPostDrawCallback);

  ImGuiNewFrameCallback* aPreDrawCallback = new ImGuiNewFrameCallback(*this);
  theCamera->setPreDrawCallback(aPreDrawCallback);
}

void
ImGuiHandler::newFrame(osg::RenderInfo& theRenderInfo)
{
  if (!g_FontTexture) {
    init();
  }

  ImGuiIO& io = ImGui::GetIO();

  osg::Viewport* aViewport = theRenderInfo.getCurrentCamera()->getViewport();
  io.DisplaySize           = ImVec2(aViewport->width(), aViewport->height());

  double aCurrentTime =
    theRenderInfo.getView()->getFrameStamp()->getSimulationTime();
  io.DeltaTime =
    g_Time > 0.0 ? (float)(aCurrentTime - g_Time) : (float)(1.0f / 60.0f);
  g_Time = aCurrentTime;

  for (int i = 0; i < 3; i++) {
    io.MouseDown[i] = g_MousePressed[i];
  }

  io.MouseWheel = g_MouseWheel;
  g_MouseWheel  = 0.0f;

  ImGui::NewFrame();
}

void
ImGuiHandler::render(osg::RenderInfo& /*theRenderInfo*/)
{

  if (m_callback) {
    (*m_callback)();
  }

  ImGui::Render();
}

bool
ImGuiHandler::handle(const osgGA::GUIEventAdapter& theEventAdapter,
                     osgGA::GUIActionAdapter& /*theActionAdapter*/,
                     osg::Object* /*theObject*/,
                     osg::NodeVisitor* /*theNodeVisitor*/)
{
  const bool wantCapureMouse    = ImGui::GetIO().WantCaptureMouse;
  const bool wantCapureKeyboard = ImGui::GetIO().WantCaptureKeyboard;
  ImGuiIO&   io                 = ImGui::GetIO();

  switch (theEventAdapter.getEventType()) {

    case osgGA::GUIEventAdapter::KEYDOWN: {
      const int c           = theEventAdapter.getKey();
      const int special_key = ConvertFromOSGKey(c);
      if (special_key > 0) {
        assert(special_key < 512 && "ImGui KeysDown is an array of 512");
        assert(special_key > 256 &&
               "ASCII stop at 127, but we use the range [257, 511]");

        io.KeysDown[special_key] = true;

        io.KeyCtrl = io.KeysDown[ConvertedKey_LeftControl] ||
                     io.KeysDown[ConvertedKey_RightControl];
        io.KeyShift = io.KeysDown[ConvertedKey_LeftShift] ||
                      io.KeysDown[ConvertedKey_RightShift];
        io.KeyAlt = io.KeysDown[ConvertedKey_LeftAlt] ||
                    io.KeysDown[ConvertedKey_RightAlt];
        io.KeySuper = io.KeysDown[ConvertedKey_LeftSuper] ||
                      io.KeysDown[ConvertedKey_RightSuper];
      } else if (c > 0 && c < 0x10000) {
        io.AddInputCharacter((unsigned short)c);
      }
      return wantCapureKeyboard;
    }
    case osgGA::GUIEventAdapter::KEYUP: {
      const int c           = theEventAdapter.getKey();
      const int special_key = ConvertFromOSGKey(c);
      if (special_key > 0) {
        assert(special_key < 512 && "ImGui KeysMap is an array of 512");
        assert(special_key > 256 &&
               "ASCII stop at 127, but we use the range [257, 511]");

        io.KeysDown[special_key] = false;

        io.KeyCtrl = io.KeysDown[ConvertedKey_LeftControl] ||
                     io.KeysDown[ConvertedKey_RightControl];
        io.KeyShift = io.KeysDown[ConvertedKey_LeftShift] ||
                      io.KeysDown[ConvertedKey_RightShift];
        io.KeyAlt = io.KeysDown[ConvertedKey_LeftAlt] ||
                    io.KeysDown[ConvertedKey_RightAlt];
        io.KeySuper = io.KeysDown[ConvertedKey_LeftSuper] ||
                      io.KeysDown[ConvertedKey_RightSuper];
      }
      return wantCapureKeyboard;
    }
    case (osgGA::GUIEventAdapter::PUSH): {
      ImGuiIO& io = ImGui::GetIO();
      io.MousePos = ImVec2(theEventAdapter.getX(),
                           io.DisplaySize.y - theEventAdapter.getY());
      g_MousePressed[0] = true;
      return wantCapureMouse;
    }
    case (osgGA::GUIEventAdapter::DRAG):
    case (osgGA::GUIEventAdapter::MOVE): {
      ImGuiIO& io = ImGui::GetIO();
      io.MousePos = ImVec2(theEventAdapter.getX(),
                           io.DisplaySize.y - theEventAdapter.getY());
      return wantCapureMouse;
    }
    case (osgGA::GUIEventAdapter::RELEASE): {
      g_MousePressed[0] = false;
      return wantCapureMouse;
    }
    case (osgGA::GUIEventAdapter::SCROLL): {
      g_MouseWheel = theEventAdapter.getScrollingDeltaY();
      return wantCapureMouse;
    }
    default: {
      return false;
    }
  }

  return false;
}
#endif
