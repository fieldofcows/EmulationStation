#include "platform.h"
#include "Renderer.h"
#include GLHEADER
#include <iostream>
#include "resources/Font.h"
#include "resources/ShaderVector.h"
#include <boost/filesystem.hpp>
#include "Log.h"
#include <stack>
#include "Util.h"

namespace Renderer {
	std::stack<Eigen::Vector4i> clipStack;

	void setColor4bArray(GLubyte* array, unsigned int color)
	{
		array[0] = (color & 0xff000000) >> 24;
		array[1] = (color & 0x00ff0000) >> 16;
		array[2] = (color & 0x0000ff00) >> 8;
		array[3] = (color & 0x000000ff);
	}

	void buildGLColorArray(GLubyte* ptr, unsigned int color, unsigned int vertCount)
	{
		unsigned int colorGl;
		setColor4bArray((GLubyte*)&colorGl, color);
		for(unsigned int i = 0; i < vertCount; i++)
		{
			((GLuint*)ptr)[i] = colorGl;
		}
	}

	void pushClipRect(Eigen::Vector2i pos, Eigen::Vector2i dim)
	{
		Eigen::Vector4i box(pos.x(), pos.y(), dim.x(), dim.y());
		if(box[2] == 0)
			box[2] = Renderer::getScreenWidth() - box.x();
		if(box[3] == 0)
			box[3] = Renderer::getScreenHeight() - box.y();

		//glScissor starts at the bottom left of the window
		//so (0, 0, 1, 1) is the bottom left pixel
		//everything else uses y+ = down, so flip it to be consistent
		//rect.pos.y = Renderer::getScreenHeight() - rect.pos.y - rect.size.y;
		box[1] = Renderer::getScreenHeight() - box.y() - box[3];

		//make sure the box fits within clipStack.top(), and clip further accordingly
		if(clipStack.size())
		{
			Eigen::Vector4i& top = clipStack.top();
			if(top[0] > box[0])
				box[0] = top[0];
			if(top[1] > box[1])
				box[1] = top[1];
			if(top[0] + top[2] < box[0] + box[2])
				box[2] = (top[0] + top[2]) - box[0];
			if(top[1] + top[3] < box[1] + box[3])
				box[3] = (top[1] + top[3]) - box[1];
		}

		if(box[2] < 0)
			box[2] = 0;
		if(box[3] < 0)
			box[3] = 0;

		clipStack.push(box);
		glScissor(box[0], box[1], box[2], box[3]);
		glEnable(GL_SCISSOR_TEST);
	}

	void popClipRect()
	{
		if(clipStack.empty())
		{
			LOG(LogError) << "Tried to popClipRect while the stack was empty!";
			return;
		}

		clipStack.pop();
		if(clipStack.empty())
		{
			glDisable(GL_SCISSOR_TEST);
		}else{
			Eigen::Vector4i top = clipStack.top();
			glScissor(top[0], top[1], top[2], top[3]);
		}
	}

	void drawRect(float x, float y, float w, float h, unsigned int color, GLenum blend_sfactor, GLenum blend_dfactor)
	{
		drawRect((int)round(x), (int)round(y), (int)round(w), (int)round(h), color, blend_sfactor, blend_dfactor);
	}

	void drawRect(int x, int y, int w, int h, unsigned int color, GLenum blend_sfactor, GLenum blend_dfactor)
	{
		ShaderVector* shader = dynamic_cast<ShaderVector*>(ResourceManager::getInstance()->shader(ResourceManager::SHADER_VECTOR));
		shader->use();

		GLshort points[12];

		points[0] = x; points [1] = y;
		points[2] = x; points[3] = y + h;
		points[4] = x + w; points[5] = y;

		points[6] = x + w; points[7] = y;
		points[8] = x; points[9] = y + h;
		points[10] = x + w; points[11] = y + h;

		glEnable(GL_BLEND);
		glBlendFunc(blend_sfactor, blend_dfactor);

		glDisableVertexAttribArray(ShaderVector::ATTRIBUTE_COLOUR);
		glVertexAttrib4f(ShaderVector::ATTRIBUTE_COLOUR, COLOR_INT_TO_GL_RED(color), COLOR_INT_TO_GL_GREEN(color),
													   COLOR_INT_TO_GL_BLUE(color), COLOR_INT_TO_GL_ALPHA(color));

        glVertexAttribPointer(ShaderVector::ATTRIBUTE_VERTEX, 2, GL_SHORT, 0, 0, points);
        glEnableVertexAttribArray(ShaderVector::ATTRIBUTE_VERTEX);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisable(GL_BLEND);

		shader->endUse();
	}

	void setMatrix(const Eigen::Affine3f& matrix)
	{
		Shader::modelViewMatrix(matrix);
		glLoadMatrixf((float*)matrix.data());
	}
};
