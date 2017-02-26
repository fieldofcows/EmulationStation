/*
 * Shaders.cpp
 *
 *  Created on: 20 Feb 2017
 *      Author: rhopkins
 */

#include <resources/Shader.h>
#include <iostream>

GLuint 	Shader::mCurrentProgram = 0;

Shader::Shader() : mProgram(0)
{
	// TODO Auto-generated constructor stub

}

Shader::~Shader()
{
	// TODO Auto-generated destructor stub
}

bool Shader::parseShader(std::string code, GLuint tp, GLuint* shaderObject)
{
	GLint			length;
	GLint			ok = 0;
	char*			code_str = (char*)code.c_str();

	// Create and compile the shader
	length = code.size();
	*shaderObject = glCreateShader(tp);
	glShaderSource(*shaderObject, 1, &code_str, &length);
	glCompileShader(*shaderObject);

	// Check it compiled ok
	glGetShaderiv(*shaderObject, GL_COMPILE_STATUS, &ok);
	if (!ok)
	{
		// Get the error string
		GLint blen = 0;
		GLsizei slen = 0;
		glGetShaderiv(*shaderObject, GL_INFO_LOG_LENGTH , &blen);
		if (blen > 0)
		{
			GLchar* log = new GLchar[blen];
			glGetShaderInfoLog(*shaderObject, blen, &slen, log);
			std::cout << "Shader compile error [" << ((tp == GL_VERTEX_SHADER) ? "vertex" : "fragment") << "]: " << log;
			delete[] log;
		}
	}
	return ok ? true : false;
}

bool Shader::linkShaders(std::vector<GLuint> shaders, GLuint* program)
{
	GLint 		ok = 0;

	*program = glCreateProgram();
	for (auto it = shaders.begin(); it != shaders.end(); ++it)
	{
		glAttachShader(*program, *it);
	}
	glLinkProgram(*program);
	glGetProgramiv(*program, GL_LINK_STATUS, &ok);
	if (!ok)
	{
		// Get the error string
		GLint blen = 0;
		GLsizei slen = 0;
		glGetProgramiv(*program, GL_INFO_LOG_LENGTH , &blen);
		if (blen > 0)
		{
			GLchar* log = new GLchar[blen];
			glGetProgramInfoLog(*program, blen, &slen, log);
			std::cout << "Shader link error: " << log;
			delete[] log;
		}
	}
	return ok ? true : false;
}

void Shader::use()
{
	if (mCurrentProgram != mProgram)
	{
		glUseProgram(mProgram);
		mCurrentProgram = mProgram;
	}
}

void Shader::endUse()
{
	if (mCurrentProgram != 0)
	{
		glUseProgram(0);
		mCurrentProgram = 0;
	}
}

Eigen::Affine3f Shader::getOrthoProjection(float left, float right,float bottom, float top,float near, float far)
{
	Eigen::Affine3f proj;
	float a = 2.0f / (right - left);
	float b = 2.0f / (top - bottom);
	float c = -2.0f / (far - near);

	float tx = - (right + left)/(right - left);
	float ty = - (top + bottom)/(top - bottom);
	float tz = - (far + near)/(far - near);

	proj.data()[0] = a;
	proj.data()[5] = b;
	proj.data()[10] = c;
	proj.data()[12] = tx;
	proj.data()[13] = ty;
	proj.data()[14] = tz;
	proj.data()[15] = 1.0f;

	return proj;
}
