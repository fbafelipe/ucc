#include "compiler/Declaration.h"

#include "compiler/PrimitiveType.h"

Declaration::Declaration() : typedefDecl(false), externDecl(false), staticDecl(false) {
	baseType = new PrimitiveType<int>();
}

Declaration::~Declaration() {}

const Pointer<Type> & Declaration::getBaseType() const {
	return baseType;
}

void Declaration::setBaseType(const Pointer<Type> & t) {
	baseType = t;
}

void Declaration::addDeclarator(const Pointer<Declarator> & v) {
	declarators.push_back(v);
}

const DeclaratorList & Declaration::getDeclarators() const {
	return declarators;
}

bool Declaration::isTypeDef() const {
	return typedefDecl;
}

void Declaration::setTypedef(bool td) {
	typedefDecl = td;
}

bool Declaration::isExtern() const {
	return externDecl;
}

void Declaration::setExtern(bool ext) {
	externDecl = ext;
}

bool Declaration::isStatic() const {
	return staticDecl;
}

void Declaration::setStatic(bool s) {
	staticDecl = s;
}
