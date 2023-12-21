#include "compiler/PrimitiveTypeBase.h"

PrimitiveTypeBase::PrimitiveTypeBase() {}

PrimitiveTypeBase::~PrimitiveTypeBase() {}

Type::TypeEnum PrimitiveTypeBase::getTypeEnum() const {
	return PRIMITIVE;
}

Type & PrimitiveTypeBase::getBaseType() {
	return *this;
}
