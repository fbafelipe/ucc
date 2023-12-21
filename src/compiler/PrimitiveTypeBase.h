#ifndef PRIMITIVE_TYPE_BASE_H
#define PRIMITIVE_TYPE_BASE_H

#include "compiler/Type.h"

class PrimitiveTypeBase : public Type {
	public:
		PrimitiveTypeBase();
		virtual ~PrimitiveTypeBase();
		
		virtual bool isInteger() const = 0;
		
		virtual TypeEnum getTypeEnum() const;
		
		virtual Type & getBaseType();
};

#endif
