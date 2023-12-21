#include "compiler/IndirectDeclarator.h"

#include <cassert>

IndirectDeclarator::IndirectDeclarator(const Pointer<Declarator> & decl) : base(decl) {
	assert(base);
}

IndirectDeclarator::~IndirectDeclarator() {}

const std::string & IndirectDeclarator::getName() const {
	return base->getName();
}
void IndirectDeclarator::setName(const std::string & n) {
	return base->setName(n);
}

const Pointer<Type> & IndirectDeclarator::getType() const {
	return base->getType();
}

void IndirectDeclarator::setType(const Pointer<Type> & t) {
	return base->setType(t);
}

void IndirectDeclarator::setInitializer(ParsingTree::NonTerminal *nt) {
	base->setInitializer(nt);
}

ParsingTree::NonTerminal *IndirectDeclarator::getInitializer() const {
	return base->getInitializer();
}

bool IndirectDeclarator::hasInitializer() const {
	return base->hasInitializer();
}
