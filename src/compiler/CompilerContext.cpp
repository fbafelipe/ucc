#include "compiler/CompilerContext.h"

#include "compiler/GlobalSymbolTable.h"
#include "vm/ArithmeticInstruction.h"
#include "vm/LoadInstruction.h"
#include "vm/SetInstruction.h"
#include "vm/StoreInstruction.h"

//#define SHOW_ALLOCS

#ifdef SHOW_ALLOCS
#include <iostream>

static int allocBytes = 0;
#endif

CompilerContext::CompilerContext(const Compiler *comp) : compiler(comp) {
	startFunction = new Function("_start");
	
	staticMemory = new StaticMemory();
}

CompilerContext::~CompilerContext() {
	// in success, the instruction list will be empty (they will be consumed)
	for (InstructionList::iterator it = instructions.begin(); it != instructions.end(); ++it) {
		delete(*it);
	}
}

const Compiler *CompilerContext::getCompiler() const {
	return compiler;
}

SymbolManager & CompilerContext::getSymbolManager() {
	return symbolManager;
}

const SymbolManager & CompilerContext::getSymbolManager() const {
	return symbolManager;
}

const Pointer<StaticMemory> & CompilerContext::getStaticMemory() const {
	return staticMemory;
}

void CompilerContext::addInstruction(Instruction *inst) {
	instructions.push_back(inst);
}

const CompilerContext::InstructionList & CompilerContext::getInstructions() const {
	return instructions;
}

void CompilerContext::consumeInstructions() {
	instructions.clear();
}

const Pointer<Scope> & CompilerContext::beginScope() {
	symbolManager.scopeBegin();
	
	scopeStack.push_back(new Scope(instructions.size()));
	return scopeStack.back();
}

void CompilerContext::endScope() {
	symbolManager.scopeEnd();
	
	assert(!scopeStack.empty());
	const Pointer<Scope> & scope = scopeStack.back();
	
	unsigned int allocatedStack = scope->getAllocatedStack();
	if (allocatedStack > 0) {
		// deallocate the variables from the scope
		deallocateStack(allocatedStack);
	}
	
	scope->setInstructionsEnd(instructions.size());
	
	scopeStack.pop_back();
}

const Pointer<Scope> & CompilerContext::getCurrentScope() const {
	assert(!scopeStack.empty());
	
	return scopeStack.back();
}

Pointer<Scope> CompilerContext::getScopeWithFlag(Scope::ScopeFlag flag) const {
	for (ScopeStack::const_reverse_iterator it = scopeStack.rbegin(); it != scopeStack.rend(); ++it) {
		if ((*it)->hasScopeFlag(flag)) return *it;
	}
	
	return NULL;
}

unsigned int CompilerContext::getAccumulatedStack(const Pointer<Scope> & scope) const {
	unsigned int mem = 0;
	
	for (ScopeStack::const_reverse_iterator it = scopeStack.rbegin(); it != scopeStack.rend(); ++it) {
		mem += (*it)->getAllocatedStack();
		
		if (*it == scope) break;
	}
	
	return mem;
}

const Pointer<Function> & CompilerContext::beginFunction(const std::string & name) {
	assert(!currentFunction);
	
	GlobalSymbolTable *globalSym = symbolManager.getGlobalSymbolTable();
	
	if (globalSym->hasFunction(name)) currentFunction = globalSym->getFunction(name);
	else {
		currentFunction = new Function(name);
		globalSym->addFunction(name, currentFunction);
	}
	
	return currentFunction;
}

void CompilerContext::endFunction() {
	assert(currentFunction);
	
	assert(currentFunction->getStackBaseOffset() == currentFunction->getReturnValueSize() && "Stack leak");
	
	functions.push_back(currentFunction);
	currentFunction = NULL;
}

const Pointer<Function> & CompilerContext::getCurrentFunction() const {
	if (!currentFunction) {
		// external initialization
		return startFunction;
	}
	
	return currentFunction;
}

const FunctionList & CompilerContext::getFunctionList() const {
	return functions;
}

const Pointer<Function> & CompilerContext::getStartFunction() const {
	return startFunction;
}

Register CompilerContext::allocatePRRegister() {
	return getCurrentFunction()->allocatePRRegister();
}

Register CompilerContext::allocateFPRegister() {
	return getCurrentFunction()->allocateFPRegister();
}

// deallocate a register for the current function
void CompilerContext::deallocateRegister(Register reg) {
	if (reg == REG_ZERO) return;
	
	if (RegisterUtils::isProgrammerRegister(reg)) deallocatePRRegister(reg);
	else deallocateFPRegister(reg);
}

void CompilerContext::deallocatePRRegister(Register reg) {
	getCurrentFunction()->deallocatePRRegister(reg);
}

void CompilerContext::deallocateFPRegister(Register reg) {
	getCurrentFunction()->deallocateFPRegister(reg);
}

Register CompilerContext::allocateConstant(Number value, bool relocable) {
	Register result = REG_NOTUSED;
	
	if (value.isInteger()) result = allocatePRRegister();
	else result = allocateFPRegister();
	
	Instruction *inst = new SetInstruction(result, value);
	if (relocable) inst->setRelocable(true);
	addInstruction(inst);
	
	assert(result != REG_NOTUSED && "Register overflow");
	
	return result;
}

Register CompilerContext::allocateConstant(RegisterInt value) {
	return allocateConstant(Number(Number::INT, value));
}

Register CompilerContext::allocateConstant(int value) {
	return allocateConstant((RegisterInt)value);
}

Register CompilerContext::allocateConstant(unsigned int value) {
	return allocateConstant((RegisterInt)value);
}

Register CompilerContext::allocateConstant(RegisterFloat value) {
	return allocateConstant(Number(Number::FLOAT, (double)value));
}

Register CompilerContext::allocateString(const std::string & str) {
	const Pointer<StaticMemory> & sMemory = getStaticMemory();
	unsigned int pos = sMemory->allocate(str.size() + 1);
	sMemory->initialize(pos, str.c_str(), str.size() + 1);
	
	Register reg = allocateConstant(Number(Number::INT, (RegisterInt)pos), true);
	addInstruction(new AddInstruction(reg, REG_GP, reg));
	return reg;
}

void CompilerContext::stackPush(Register reg, unsigned int size) {
	allocateStack(size);
	addInstruction(new StoreInstruction(reg, REG_SP, size, 0));
}

void CompilerContext::stackPop(Register reg, unsigned int size) {
	addInstruction(new LoadInstruction(reg, REG_SP, size, 0));
	deallocateStack(size);
}

void CompilerContext::allocateStack(unsigned int size) {
#ifdef SHOW_ALLOCS
	for (int i = 0; i < allocBytes; ++i) std::cout << ' ';
	std::cout << "alloc " << size << "\n";
	allocBytes += size;
#endif
	
	getCurrentFunction()->incrementStackBaseOffset(size);
	
	Register regSize = allocateConstant(size);
	addInstruction(new SubInstruction(REG_SP, REG_SP, regSize));
	deallocateRegister(regSize);
}

void CompilerContext::deallocateStack(unsigned int size) {
#ifdef SHOW_ALLOCS
	allocBytes -= size;
	for (int i = 0; i < allocBytes; ++i) std::cout << ' ';
	
	if (allocBytes < 0) std::cout << "(" << allocBytes << ") ";
	std::cout << "dealloc " << size << "\n";
#endif
	
	getCurrentFunction()->decrementStackBaseOffset(size);
	
	Register regSize = allocateConstant(size);
	addInstruction(new AddInstruction(REG_SP, REG_SP, regSize));
	deallocateRegister(regSize);
}
