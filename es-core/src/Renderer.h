#ifndef _RENDERER_H_
#define _RENDERER_H_

#include <vector>
#include <string>
#include "platform.h"
#include <Eigen/Dense>
#include GLHEADER

class GuiComponent;
class Font;

#define COLOR_INT_TO_GL_RED(x) 			((GLfloat)(((x) & 0xff000000) >> 24) / 255.0f)
#define COLOR_INT_TO_GL_GREEN(x)		((GLfloat)(((x) & 0x00ff0000) >> 16) / 255.0f)
#define COLOR_INT_TO_GL_BLUE(x)			((GLfloat)(((x) & 0x0000ff00) >> 8) / 255.0f)
#define COLOR_INT_TO_GL_ALPHA(x)		((GLfloat)(((x) & 0x000000ff)) / 255.0f)

//The Renderer provides several higher-level functions for drawing (rectangles, text, etc.).
//Renderer_draw_gl.cpp has most of the higher-level functions and wrappers.
//Renderer_init_*.cpp has platform-specific renderer initialziation/deinitialziation code.  (e.g. the Raspberry Pi sets up dispmanx/OpenGL ES)
namespace Renderer
{
	bool init(int w, int h);
	void deinit();

	unsigned int getScreenWidth();
	unsigned int getScreenHeight();

	void buildGLColorArray(GLubyte* ptr, unsigned int color, unsigned int vertCount);

	//graphics commands
	void swapBuffers();

	void pushClipRect(Eigen::Vector2i pos, Eigen::Vector2i dim);
	void popClipRect();

	void setMatrix(const Eigen::Affine3f& transform);

	void drawRect(int x, int y, int w, int h, unsigned int color, GLenum blend_sfactor = GL_SRC_ALPHA, GLenum blend_dfactor = GL_ONE_MINUS_SRC_ALPHA);
	void drawRect(float x, float y, float w, float h, unsigned int color, GLenum blend_sfactor = GL_SRC_ALPHA, GLenum blend_dfactor = GL_ONE_MINUS_SRC_ALPHA);
}

#endif
