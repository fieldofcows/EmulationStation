/*
 * ShaderI420.h
 *
 *  Created on: 22 Feb 2017
 *      Author: rhopkins
 */

#ifndef ES_CORE_SRC_RESOURCES_SHADERI420_H_
#define ES_CORE_SRC_RESOURCES_SHADERI420_H_

#include <resources/Shader.h>

class ShaderI420: public Shader
{
public:
	ShaderI420();
	virtual ~ShaderI420();

	virtual void init();

	void modelMatrix(const Eigen::Affine3f& mat);
	void projectionMatrix(const Eigen::Affine3f& mat);
	void textures(GLuint y, GLuint u, GLuint v);
	void fadeIn(float fade);

private:
	GLuint	mVertex;
	GLuint	mFragment;
	GLint	mUniformModel;
	GLint	mUniformProjection;
	GLint	mUniformY;
	GLint	mUniformU;
	GLint	mUniformV;
	GLint	mUniformFade;
};

#endif /* ES_CORE_SRC_RESOURCES_SHADERI420_H_ */
