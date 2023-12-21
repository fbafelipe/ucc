#include "compiler/DeclaratorBase.h"

#include <cassert>

DeclaratorBase::DeclaratorBase(const Pointer<Type> & t) : type(t), initializer(NULL) {
	assert(type);
}

DeclaratorBase::~DeclaratorBase() {}

const std::string & DeclaratorBase::getName() const {
	return name;
}

void DeclaratorBase::setName(const std::string & n) {
	name = n;
}

const Pointer<Type> & DeclaratorBase::getType() const {
	return type;
}

void DeclaratorBase::setType(const Pointer<Type> & t) {
	type = t;
}

void DeclaratorBase::setInitializer(ParsingTree::NonTerminal *nt) {
	initializer = nt;
}

ParsingTree::NonTerminal *DeclaratorBase::getInitializer() const {
	return initializer;	
}

bool DeclaratorBase::hasInitializer() const {
	return initializer != NULL;
}
