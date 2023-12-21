#include "compiler/Function.h"

#include <cassert>

Function::Function(const std::string & n) : name(n), stackBaseOffset(0), implemented(false) {
	for (Register r = REG_PR0; r <= REG_PR7; ++r) prRegisters.insert(r);
	for (Register r = REG_FP0; r <= REG_FP3; ++r) fpRegisters.insert(r);
}

const std::string & Function::getName() const {
	return name;
}

const Pointer<FunctionType> & Function::getType() const {
	assert(type);
	
	return type;
}

void Function::setType(const Pointer<FunctionType> & t) {
	assert(t);
	
	type = t;
}

unsigned int Function::getCurrentUsedRegisters() const {
	return 12 - prRegisters.size() - fpRegisters.size();
}

bool Function::allRegistersFree() const {
	return prRegisters.size() == 8 && fpRegisters.size() == 4;
}

Register Function::allocatePRRegister() {
	assert(!prRegisters.empty() && "Register overflow");
	
	RegisterSet::iterator it = prRegisters.begin();
	Register reg = *it;
	prRegisters.erase(*it);
	
	return reg;
}

void Function::deallocatePRRegister(Register reg) {
	assert(RegisterUtils::isProgrammerRegister(reg));
	assert(prRegisters.find(reg) == prRegisters.end());
	
	prRegisters.insert(reg);
}

Register Function::allocateFPRegister() {
	assert(!fpRegisters.empty() && "Register overflow");
	
	RegisterSet::iterator it = fpRegisters.begin();
	Register reg = *it;
	fpRegisters.erase(*it);
	
	return reg;
}

void Function::deallocateFPRegister(Register reg) {
	assert(RegisterUtils::isFloatingPointRegister(reg));
	assert(fpRegisters.find(reg) == fpRegisters.end());
	
	fpRegisters.insert(reg);
}

unsigned int Function::getReturnValueSPOffset() {
	assert(stackBaseOffset >= getReturnValueSize());
	
	return stackBaseOffset - getReturnValueSize();
}

unsigned int Function::getReturnAddrSPOffset() {
	return stackBaseOffset;
}

unsigned int Function::getStackBaseOffset() const {
	return stackBaseOffset;
}

void Function::incrementStackBaseOffset(unsigned int value) {
	stackBaseOffset += value;
}

void Function::decrementStackBaseOffset(unsigned int value) {
	assert(stackBaseOffset >= value);
	
	stackBaseOffset -= value;
}

unsigned int Function::getReturnValueSize() const {
	assert(type);
	
	return type->getReturnType()->getSize();
}

bool Function::wasImplemented() const {
	return implemented;
}

void Function::setImplemented() {
	implemented = true;
}
