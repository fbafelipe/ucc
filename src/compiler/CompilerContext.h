#ifndef COMPILER_CONTEXT_H
#define COMPILER_CONTEXT_H

#include "compiler/Function.h"
#include "compiler/Scope.h"
#include "compiler/StaticMemory.h"
#include "compiler/SymbolManager.h"
#include "vm/Instruction.h"
#include "Number.h"

#include <parser/Pointer.h>

#include <vector>

typedef std::vector<Pointer<Function> > FunctionList;
typedef std::vector<Pointer<Scope> > ScopeStack;

class Compiler;

class CompilerContext {
	public:
		typedef std::vector<Instruction *> InstructionList;
		
		CompilerContext(const Compiler *comp);
		~CompilerContext();
		
		const Compiler *getCompiler() const;
		
		SymbolManager & getSymbolManager();
		const SymbolManager & getSymbolManager() const;
		
		const Pointer<StaticMemory> & getStaticMemory() const;
		
		void addInstruction(Instruction *inst);
		const InstructionList & getInstructions() const;
		void consumeInstructions();
		
		const Pointer<Scope> & beginScope();
		void endScope();
		const Pointer<Scope> & getCurrentScope() const;
		
		const Pointer<Function> & beginFunction(const std::string & name);
		void endFunction();
		const Pointer<Function> & getCurrentFunction() const;
		
		const FunctionList & getFunctionList() const;
		const Pointer<Function> & getStartFunction() const;
		
		// return the toppest scope that has the flag
		Pointer<Scope> getScopeWithFlag(Scope::ScopeFlag flag) const;
		
		// return the total stack memory from the top scope to the specified scope (inclusive)
		unsigned int getAccumulatedStack(const Pointer<Scope> & scope) const;
		
		Register allocatePRRegister();
		Register allocateFPRegister();
		void deallocateRegister(Register reg); // deallocate a register for the current function
		void deallocatePRRegister(Register reg);
		void deallocateFPRegister(Register reg);
		Register allocateConstant(Number value, bool relocable = false);
		Register allocateConstant(RegisterInt value);
		Register allocateConstant(int value);
		Register allocateConstant(unsigned int value);
		Register allocateConstant(RegisterFloat value);
		Register allocateString(const std::string & str);
		
		// stack functions
		void stackPush(Register reg, unsigned int size = REGISTER_SIZE);
		void stackPop(Register reg, unsigned int size = REGISTER_SIZE);
		void allocateStack(unsigned int size);
		void deallocateStack(unsigned int size);
		
	private:
		const Compiler *compiler;
		
		SymbolManager symbolManager;
		
		// the current defining function
		Pointer<Function> currentFunction;
		
		// main caller, does the external initializations
		Pointer<Function> startFunction;
		
		// the defined functions
		FunctionList functions;
		
		ScopeStack scopeStack;
		
		Pointer<StaticMemory> staticMemory;
		
		InstructionList instructions;
};

#endif
