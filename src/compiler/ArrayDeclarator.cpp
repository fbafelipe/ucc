#include "compiler/ArrayDeclarator.h"

#include "compiler/ArrayType.h"

ArrayDeclarator::ArrayDeclarator(const Pointer<Declarator> & decl, int c) :
		IndirectDeclarator(decl), count(c) {
	
	assert(count >= -1);
	
	setType(base->getType());
}

ArrayDeclarator::~ArrayDeclarator() {}

const Pointer<Type> & ArrayDeclarator::getElementType() const {
	return elementType;
}

void ArrayDeclarator::setType(const Pointer<Type> & t) {
	elementType = t;
	
	base->setType(new ArrayType(elementType, count));
}

int ArrayDeclarator::getCount() const {
	return count;
}

void ArrayDeclarator::setCount(int c) {
	assert(c >= -1);
	
	count = c;
}
