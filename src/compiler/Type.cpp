#include "compiler/Type.h"

#include "vm/RegisterUtils.h"

#include <cstdlib>

Pointer<Type> Type::getResultingType(const Pointer<Type> & t1, const Pointer<Type> & t2) {
	if (*t1 == *t2) return t1;
	
	// TODO
	abort();
}

Type::Type() : constant(false), volatileType(false) {}

Type::~Type() {}

bool Type::isConstant() const {
	return constant;
}

void Type::setConstant(bool c) {
	constant = c;
}

bool Type::isVolatile() const {
	return volatileType;
}

void Type::setVolatile(bool v) {
	volatileType = v;
}

bool Type::isFloatingPoint() const {
	return false;
}

bool Type::isVoid() const {
	return false;
}

bool Type::fitRegister() const {
	return getSize() <= REGISTER_SIZE;
}

bool Type::operator!=(const Type & other) const {
	return !(*this == other);
}

std::ostream & operator<<(std::ostream & stream, const Type & type) {
	return stream << type.toString();
}
