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
attribute vec4	a_colour;										\
uniform mat4 	u_modelMatrix;									\
uniform mat4 	u_projectionMatrix;								\
varying vec4	v_colour;										\
  																\
void main(void)													\
{																\
     gl_Position = u_projectionMatrix * u_modelMatrix * a_position;	\
     v_colour = a_colour;										\
}																\
";

static const char* fragment_shader =
"																\
varying vec4	v_colour;										\
																\
void main()														\
{																\
    gl_FragColor = v_colour;									\
}																\
";

ShaderVector::ShaderVector() : Shader(), mVertex(0), mFragment(0)
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
}
