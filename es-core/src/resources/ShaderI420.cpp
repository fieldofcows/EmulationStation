/*
 * ShaderI420.cpp
 *
 *  Created on: 22 Feb 2017
 *      Author: rhopkins
 */

#include <resources/ShaderI420.h>
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
uniform sampler2D 	u_tex_y;									\
uniform sampler2D 	u_tex_u;									\
uniform sampler2D 	u_tex_v;									\
uniform float		u_fade;										\
																\
void main()														\
{																\
    float y = texture2D(u_tex_y, v_texcoord).r;  				\
    float u = texture2D(u_tex_u, v_texcoord).r - 0.5;  			\
    float v = texture2D(u_tex_v, v_texcoord).r - 0.5;  			\
    float r = (y + 1.402 * v) * u_fade;							\
    float g = (y - 0.344 * u - 0.714 * v) * u_fade;				\
    float b = (y + 1.772 * u) * u_fade;							\
    gl_FragColor = vec4(r, g, b, 1.0);							\
}																\
";

ShaderI420::ShaderI420() : Shader(), mVertex(0), mFragment(0),
			mUniformY(0), mUniformU(0), mUniformV(0), mUniformFade(0)
{
}

ShaderI420::~ShaderI420()
{
}

void ShaderI420::init()
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
	mUniformY = glGetUniformLocation(mProgram, "u_tex_y");
	mUniformU = glGetUniformLocation(mProgram, "u_tex_u");
	mUniformV = glGetUniformLocation(mProgram, "u_tex_v");
	mUniformFade = glGetUniformLocation(mProgram, "u_fade");

	ATTRIBUTE_VERTEX = glGetAttribLocation(mProgram, "a_position");
	ATTRIBUTE_TEXCOORD = glGetAttribLocation(mProgram, "a_texcoord");
}

void ShaderI420::textures(GLuint y, GLuint u, GLuint v)
{
	use();
	glUniform1i(mUniformY, y);
	glUniform1i(mUniformU, u);
	glUniform1i(mUniformV, v);
}

void ShaderI420::fadeIn(float fade)
{
	use();
	glUniform1f(mUniformFade, fade);
}
