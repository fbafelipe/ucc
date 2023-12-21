#include "compiler/Scope.h"

#include "compiler/CompilerContext.h"
#include "vm/JumpInstruction.h"

#include <cassert>

Scope::Scope(unsigned int b) : begin(b), end(0), allocatedStack(0), scopeFlags(0) {}

Scope::~Scope() {}

void Scope::setInstructionsEnd(unsigned int e) {
	// cannot set the end twice
	assert(end == 0);
	
	end = e;
	
	// update instOffsetList
	unsigned int scopeSize = getInstructionCount();
	for (InstOffsetList::const_iterator it = instOffsetList.begin(); it != instOffsetList.end(); ++it) {
		assert((int)scopeSize >= (*it)->getTarget());
		
		unsigned int offset = scopeSize - (*it)->getTarget() - 1;
		
		(*it)->setTarget(offset);
	}
}

unsigned int Scope::getInstructionsBegin() const {
	return begin;
}

// exclusive
unsigned int Scope::getInstructionsEnd() const {
	return end;
}

unsigned int Scope::getInstructionCount() const {
	assert(end >= begin);
	
	return end - begin;
}

void Scope::allocateStack(unsigned int s) {
	allocatedStack += s;
}

unsigned int Scope::getAllocatedStack() const {
	return allocatedStack;
}

void Scope::addScopeFlag(ScopeFlag flag) {
	scopeFlags |= flag;
}

bool Scope::hasScopeFlag(ScopeFlag flag) const {
	return scopeFlags & flag;
}

unsigned int Scope::getScopeFlags() const {
	return scopeFlags;
}

void Scope::setScopeFlags(unsigned int flags) {
	scopeFlags = flags;
}

JumpInstruction *Scope::createJumpInstScopeBegin(CompilerContext & context) {
	return new JumpInstruction((int)begin - (int)context.getInstructions().size() - 1);
}

JumpInstruction *Scope::createJumpInstScopeEnd(CompilerContext & context) {
	JumpInstruction *inst = new JumpInstruction(context.getInstructions().size() - begin);
	instOffsetList.push_back(inst);
	return inst;
}
