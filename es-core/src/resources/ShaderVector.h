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
		ATTRIBUTE_VERTEX,
		ATTRIBUTE_COLOUR
	};

	ShaderVector();
	virtual ~ShaderVector();

	virtual void init();

private:
	GLuint	mVertex;
	GLuint	mFragment;
};

#endif /* ES_CORE_SRC_RESOURCES_SHADERI420_H_ */
