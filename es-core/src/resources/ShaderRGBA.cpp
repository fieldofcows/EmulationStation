/*
 * ShaderRGBA.cpp
 *
 *  Created on: 22 Feb 2017
 *      Author: rhopkins
 */

#include <resources/ShaderRGBA.h>
#include "Renderer.h"

static const char* vertex_shader =
"																\
attribute vec4 	a_position;										\
attribute vec2 	a_texcoord;										\
attribute vec4	a_colour;										\
varying vec2 	v_texcoord;										\
varying vec4	v_colour;										\
uniform mat4 	u_modelMatrix;									\
uniform mat4 	u_projectionMatrix;								\
  																\
void main(void)													\
{																\
     gl_Position = u_projectionMatrix * u_modelMatrix * a_position;	\
     v_texcoord = a_texcoord.xy;								\
     v_colour = a_colour;										\
}																\
";

static const char* fragment_shader =
"																\
varying vec2 		v_texcoord;									\
varying vec4		v_colour;									\
uniform sampler2D 	u_tex;										\
																\
void main()														\
{																\
    gl_FragColor = texture2D(u_tex, v_texcoord) * v_colour;		\
}																\
";

ShaderRGBA::ShaderRGBA() : Shader(), mVertex(0), mFragment(0),
			mUniformTexture(0)
{
}

ShaderRGBA::~ShaderRGBA()
{
}

void ShaderRGBA::init()
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
	mUniformTexture = glGetUniformLocation(mProgram, "u_tex");
}

void ShaderRGBA::texture(GLuint rgba)
{
	use();
	glUniform1i(mUniformTexture, rgba);
}

void ShaderRGBA::colour(unsigned int rgba)
{
	use();
	GLfloat val[4];
	val[0] = (GLfloat)((rgba & 0xff000000) >> 24) / 255.0f;
	val[1] = (GLfloat)((rgba & 0x00ff0000) >> 16) / 255.0f;
	val[2] = (GLfloat)((rgba & 0x0000ff00) >> 8) / 255.0f;
	val[3] = (GLfloat)((rgba & 0x000000ff)) / 255.0f;
}

