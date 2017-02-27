/*
 * ShaderVector.h
 *
 *  Created on: 22 Feb 2017
 *      Author: rhopkins
 */

#ifndef ES_CORE_SRC_RESOURCES_SHADERVECTOR_H_
#define ES_CORE_SRC_RESOURCES_SHADERVECTOR_H_

#include <resources/Shader.h>

class ShaderVector: public Shader
{
public:
	enum {
		ATTRIBUTE_VERTEX
	};

	ShaderVector();
	virtual ~ShaderVector();

	virtual void init();

	void colour(unsigned Vector);

private:
	GLuint	mVertex;
	GLuint	mFragment;
	GLint	mUniformColour;
};

#endif /* ES_CORE_SRC_RESOURCES_SHADERI420_H_ */
