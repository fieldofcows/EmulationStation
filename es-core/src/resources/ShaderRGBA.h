/*
 * ShaderRGBA.h
 *
 *  Created on: 22 Feb 2017
 *      Author: rhopkins
 */

#ifndef ES_CORE_SRC_RESOURCES_SHADERRGBA_H_
#define ES_CORE_SRC_RESOURCES_SHADERRGBA_H_

#include <resources/Shader.h>

class ShaderRGBA: public Shader
{
public:
	enum {
		ATTRIBUTE_VERTEX,
		ATTRIBUTE_TEXCOORD,
		ATTRIBUTE_COLOUR
	};

	ShaderRGBA();
	virtual ~ShaderRGBA();

	virtual void init();

	void texture(GLuint rgba);
	void colour(unsigned rgba);

private:
	GLuint	mVertex;
	GLuint	mFragment;
	GLint	mUniformColour;
	GLint	mUniformTexture;
};

#endif /* ES_CORE_SRC_RESOURCES_SHADERI420_H_ */
