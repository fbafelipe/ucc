#include "compiler/ExpResult.h"

#include "compiler/CompilerContext.h"
#include "vm/ArithmeticInstruction.h"
#include "vm/LoadInstruction.h"
#include "vm/SetInstruction.h"

#include <cassert>

ExpResult::ExpResult() : resultType(VOID), offsetFromBase(0), needDeallocate(false), dynamic(false) {}

ExpResult::ExpResult(ResultType t) : resultType(t), offsetFromBase(0), needDeallocate(false), dynamic(false) {}

ExpResult::~ExpResult() {}

const Pointer<Type> & ExpResult::getType() const {
	return type;
}

void ExpResult::setType(const Pointer<Type> & t) {
	type = t;
}

ExpResult::ResultType ExpResult::getResultType() const {
	return resultType;
}

void ExpResult::setResultType(ResultType t) {
	resultType = t;
}

const Number & ExpResult::getConstant() const {
	assert(resultType == CONSTANT);
	
	return constantResult;
}

void ExpResult::setConstant(const Number & c) {
	assert(resultType == CONSTANT);
	
	constantResult = c;
}

void ExpResult::setConstant(unsigned int c) {
	assert(resultType == CONSTANT);
	
	constantResult = Number(Number::INT, (RegisterInt)c);
}

unsigned int ExpResult::getOffsetFromBase() const {
	assert(resultType == STACKED);
	
	return offsetFromBase;
}

void ExpResult::setOffsetFromBase(unsigned int off) {
	assert(resultType == STACKED);
	
	offsetFromBase = off;
}

void ExpResult::setOffsetFromBase(CompilerContext & context) {
	assert(resultType == STACKED);
	
	offsetFromBase = context.getCurrentFunction()->getStackBaseOffset();
}

bool ExpResult::isNeedDeallocate() const {
	assert(resultType == STACKED);
	
	return needDeallocate;
}

void ExpResult::setNeedDealocate(bool n) {
	assert(resultType == STACKED);
	
	needDeallocate = n;
}

bool ExpResult::isDynamic() const {
	assert(resultType == STACKED);
	
	return dynamic;
}

void ExpResult::setDynamic(bool d) {
	assert(resultType == STACKED);
	
	dynamic = d;
}

unsigned int ExpResult::getSPOffset(const CompilerContext & context) const {
	assert(resultType == STACKED);
	
	unsigned int stackBaseOff = context.getCurrentFunction()->getStackBaseOffset();
	
	assert(stackBaseOff >= offsetFromBase);
	
	return stackBaseOff - offsetFromBase;
}

unsigned int ExpResult::getStackAllocSize() const {
	assert(resultType == STACKED);
	assert(type);
	
	if (dynamic) return REGISTER_SIZE;
	return type->getSize();
}

Register ExpResult::getValue(CompilerContext & context) const {
	assert(resultType != VOID);
	assert(type);
	assert(type->fitRegister());
	
	Register reg = REG_NOTUSED;
	
	if (resultType == CONSTANT) {
		reg = context.allocateConstant(constantResult);
		
		assert(reg != REG_NOTUSED && "Register overflow");
	}
	else {
		assert(resultType == STACKED);
		
		if (type->isFloatingPoint()) reg = context.allocateFPRegister();
		else reg = context.allocatePRRegister();
		
		assert(reg != REG_NOTUSED && "Register overflow");
		
		if (dynamic) {
			context.addInstruction(new LoadInstruction(reg, REG_SP, REGISTER_SIZE, getSPOffset(context)));
			context.addInstruction(new LoadInstruction(reg, reg, type->getSize(), 0));
		}
		else context.addInstruction(new LoadInstruction(reg, REG_SP, type->getSize(), getSPOffset(context)));
	}
	
	return reg;
}

Register ExpResult::getPointer(CompilerContext & context) const {
	assert(resultType == STACKED);
	
	Register reg = context.allocatePRRegister();
	
	assert(reg != REG_NOTUSED && "Register overflow");
	
	if (dynamic) {
		context.addInstruction(new LoadInstruction(reg, REG_SP, REGISTER_SIZE, getSPOffset(context))); 
	}
	else {
		context.addInstruction(new SetInstruction(reg, getSPOffset(context)));
		context.addInstruction(new AddInstruction(reg, reg, REG_SP));
	}
	
	return reg;
}
