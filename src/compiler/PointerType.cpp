#include "compiler/PointerType.h"

#include "compiler/PrimitiveType.h"
#include "vm/RegisterUtils.h"

#include <cstdlib>

PointerType::PointerType(const Pointer<Type> & t) : baseType(t) {}

PointerType::~PointerType() {}

const Pointer<Type> & PointerType::dereference() const {
	return baseType;
}

unsigned int PointerType::getSize() const {
	return REGISTER_SIZE;
}

unsigned int PointerType::getIncrement() const {
	return baseType->getSize();
}

Type::TypeEnum PointerType::getTypeEnum() const {
	return POINTER;
}

Type & PointerType::getBaseType() {
	return baseType->getBaseType();
}

bool PointerType::allowImplicitlyCastTo(const Pointer<Type> & other) const {
	if (!other.instanceOf<PointerType>()) return false;
	
	Pointer<PointerType> otherPtr = other.staticCast<PointerType>();
	
	return *baseType == *otherPtr->baseType
			|| baseType->isVoid() || otherPtr->baseType->isVoid();
}

bool PointerType::allowExplicitCastTo(const Pointer<Type> & other) const {
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

Type *PointerType::clone() const {
	return new PointerType(baseType);
}

bool PointerType::operator==(const Type & other) const {
	if (!dynamic_cast<const PointerType *>(&other)) return false;
	const PointerType & t = (const PointerType &)other;
	return *t.baseType == *baseType;
}

std::string PointerType::toString() const {
	return baseType->toString() + "*";
}
