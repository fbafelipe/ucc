#include "compiler/ArrayType.h"

#include "compiler/PrimitiveType.h"
#include "compiler/PointerType.h"
#include "vm/RegisterUtils.h"
#include "CParserBuffer.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

ArrayType::ArrayType(const Pointer<Type> & t, int c) : baseType(t), count(c) {
	assert(count >= -1);
}

ArrayType::~ArrayType() {}

int ArrayType::getCount() const {
	return count;
}

bool ArrayType::isCountDefined() const {
	return count != -1;
}

const Pointer<Type> & ArrayType::dereference() const {
	return baseType;
}

unsigned int ArrayType::getSize() const {
	return REGISTER_SIZE;
}

unsigned int ArrayType::getIncrement() const {
	return baseType->getSize();
}

Type::TypeEnum ArrayType::getTypeEnum() const {
	return ARRAY;
}

Type & ArrayType::getBaseType() {
	return baseType->getBaseType();
}

bool ArrayType::allowImplicitlyCastTo(const Pointer<Type> & other) const {
	if (!other.instanceOf<PointerType>()) return false;
	
	Pointer<PointerType> otherPtr = other.staticCast<PointerType>();
	Pointer<Type> otherBase = otherPtr->dereference();
	
	return *baseType == *otherBase || baseType->isVoid() || otherBase->isVoid();
}

bool ArrayType::allowExplicitCastTo(const Pointer<Type> & other) const {
	if (other.instanceOf<PrimitiveTypeBase>()) {
		return other.instanceOf<PrimitiveType<int> >()
				|| other.instanceOf<PrimitiveType<long int> >()
				|| other.instanceOf<PrimitiveType<long long int> >()
				|| other.instanceOf<PrimitiveType<unsigned int> >()
				|| other.instanceOf<PrimitiveType<unsigned long int> >()
				|| other.instanceOf<PrimitiveType<unsigned long long int> >();
	}
	
	return getSize() == other->getSize();
}

Type *ArrayType::clone() const {
	return new ArrayType(baseType, count);
}

bool ArrayType::operator==(const Type & other) const {
	if (!dynamic_cast<const ArrayType *>(&other)) return false;
	const ArrayType & t = (const ArrayType &)other;
	return *t.baseType == *baseType;
}

std::string ArrayType::toString() const {
	char buf[32];
	if (count > -1) sprintf(buf, "[%d]", count);
	else strcpy(buf, "[]");
	
	return baseType->toString() + buf;
}
