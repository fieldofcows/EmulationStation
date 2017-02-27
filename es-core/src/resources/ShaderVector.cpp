/*
 * ShaderVector.cpp
 *
 *  Created on: 22 Feb 2017
 *      Author: rhopkins
 */

#include <resources/ShaderVector.h>
#include "Renderer.h"

static const char* vertex_shader =
"																\
attribute vec4 	a_position;										\
uniform mat4 	u_modelMatrix;									\
uniform mat4 	u_projectionMatrix;								\
  																\
void main(void)													\
{																\
     gl_Position = u_projectionMatrix * u_modelMatrix * a_position;	\
}																\
";

static const char* fragment_shader =
"																\
uniform vec4		u_colour;									\
																\
void main()														\
{																\
    gl_FragColor = u_colour;									\
}																\
";

ShaderVector::ShaderVector() : Shader(), mVertex(0), mFragment(0),
			mUniformColour(0)
{
}

ShaderVector::~ShaderVector()
{
}

void ShaderVector::init()
{
	std::vector<GLuint> shaders;

	// Create the video view I420 colour conversion shader
	parseShader(vertex_shader, GL_VERTEX_SHADER, &mVertex);
	parseShader(fragment_shader, GL_FRAGMENT_SHADER, &mFragment);
	shaders.push_back(mVertex);
	shaders.push_back(mFragment);
	linkShaders(shaders, &mProgram);

	mUniformModelView = glGetUniformLocation(mProgram, "u_modelMatrix");
	mUniformProjection = glGetUniformLocation(mProgram, "u_projectionMatrix");
	mUniformColour = glGetUniformLocation(mProgram, "u_colour");
}

void ShaderVector::colour(unsigned int rgba)
{
	use();
	GLfloat val[4];
	val[0] = (GLfloat)((rgba & 0xff000000) >> 24) / 255.0f;
	val[1] = (GLfloat)((rgba & 0x00ff0000) >> 16) / 255.0f;
	val[2] = (GLfloat)((rgba & 0x0000ff00) >> 8) / 255.0f;
	val[3] = (GLfloat)((rgba & 0x000000ff)) / 255.0f;
	glUniform4fv(mUniformColour, 1, val);
}

