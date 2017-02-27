/*
 * Shaders.h
 *
 *  Created on: 20 Feb 2017
 *      Author: rhopkins
 */

#ifndef ES_CORE_SRC_RESOURCES_SHADER_H_
#define ES_CORE_SRC_RESOURCES_SHADER_H_

#include <GLES2/gl2.h>
#include <string>
#include <vector>
#include <Eigen/Dense>

class Shader {
public:
	Shader();
	virtual ~Shader();
	virtual void init() = 0;

	virtual void use();
	virtual void endUse();

	static Eigen::Affine3f getOrthoProjection(float left, float right,float bottom, float top,float near, float far);
	static void projectionMatrix(const Eigen::Affine3f& mat);
	static void modelViewMatrix(const Eigen::Affine3f& mat);

protected:
	bool parseShader(std::string code, GLuint tp, GLuint* shaderObject);
	bool linkShaders(std::vector<GLuint> shaders, GLuint* program);


	static Eigen::Affine3f 	mProjection;
	static Eigen::Affine3f 	mModelView;
	GLint					mUniformProjection;
	GLint					mUniformModelView;

	GLuint					mProgram;
	static GLuint 			mCurrentProgram;
};

#endif /* ES_CORE_SRC_RESOURCES_SHADER_H_ */
