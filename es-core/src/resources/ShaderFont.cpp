/*
 * ShaderFont.cpp
 *
 *  Created on: 22 Feb 2017
 *      Author: rhopkins
 */

#include <resources/ShaderFont.h>
#include "Renderer.h"

static const char* vertex_shader =
"																\
attribute vec4 	a_position;										\
attribute vec2 	a_texcoord;										\
varying vec2 	v_texcoord;										\
uniform mat4 	u_modelMatrix;									\
uniform mat4 	u_projectionMatrix;								\
  																\
void main(void)													\
{																\
     gl_Position = u_projectionMatrix * u_modelMatrix * a_position;	\
     v_texcoord = a_texcoord.xy;								\
}																\
";

static const char* fragment_shader =
"																\
varying vec2 		v_texcoord;									\
uniform sampler2D 	u_tex;										\
uniform vec4		u_colour;									\
																\
void main()														\
{																\
    vec4 color = u_colour;										\
	color.a *= texture2D(u_tex, v_texcoord).a;					\
    gl_FragColor = color;										\
}																\
";

ShaderFont::ShaderFont() : Shader(), mVertex(0), mFragment(0),
			mUniformTexture(0), mUniformColour(0)
{
}

ShaderFont::~ShaderFont()
{
}

void ShaderFont::init()
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
	mUniformColour = glGetUniformLocation(mProgram, "u_colour");

	ATTRIBUTE_VERTEX = glGetAttribLocation(mProgram, "a_position");
	ATTRIBUTE_TEXCOORD = glGetAttribLocation(mProgram, "a_texcoord");
}

void ShaderFont::texture(GLuint rgba)
{
	use();
	glUniform1i(mUniformTexture, rgba);
}

void ShaderFont::colour(unsigned int rgba)
{
	use();
	GLfloat val[4];
	val[0] = (GLfloat)((rgba & 0xff000000) >> 24) / 255.0f;
	val[1] = (GLfloat)((rgba & 0x00ff0000) >> 16) / 255.0f;
	val[2] = (GLfloat)((rgba & 0x0000ff00) >> 8) / 255.0f;
	val[3] = (GLfloat)((rgba & 0x000000ff)) / 255.0f;
	glUniform4fv(mUniformColour, 1, val);
}
