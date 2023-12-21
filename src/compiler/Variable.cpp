#include "compiler/Variable.h"

Variable::Variable() : variableType(GLOBAL), position(0) {}

Variable::Variable(const Pointer<Type> & t, VariableType vt, unsigned int pos) :
		type(t), variableType(vt), position(pos) {}

Variable::~Variable() {}

const Pointer<Type> & Variable::getType() const {
	return type;
}

void Variable::setType(const Pointer<Type> & t) {
	type = t;
}

bool Variable::isGlobal() const {
	return variableType == GLOBAL;
}

bool Variable::isLocal() const {
	return variableType == LOCAL;
}

void Variable::setGlobal() {
	variableType = GLOBAL;
}

void Variable::setLocal() {
	variableType = LOCAL;
}

Variable::VariableType Variable::getVariableType() const {
	return variableType;
}

void Variable::setVariableType(VariableType t) {
	variableType = t;
}

unsigned int Variable::getPosition() const {
	return position;
}

void Variable::setPosition(unsigned int pos) {
	position = pos;
}
