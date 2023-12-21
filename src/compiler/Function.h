#ifndef FUNCTION_H
#define FUNCTION_H

#include "compiler/FunctionType.h"
#include "vm/RegisterUtils.h"

#include <parser/Pointer.h>

#include <set>
#include <string>

class Function {
	public:
		Function(const std::string & n);
		
		const std::string & getName() const;
		
		const Pointer<FunctionType> & getType() const;
		void setType(const Pointer<FunctionType> & t);
		
		unsigned int getCurrentUsedRegisters() const;
		bool allRegistersFree() const;
		
		Register allocatePRRegister();
		void deallocatePRRegister(Register reg);
		
		Register allocateFPRegister();
		void deallocateFPRegister(Register reg);
		
		// return the offset of the return value
		// this offset is relative to $SP, it already
		// considered the stack base
		unsigned int getReturnValueSPOffset();
		
		// return the offset of the return value
		// this offset is relative to $SP, it already
		// considered the stack base
		unsigned int getReturnAddrSPOffset();
		
		unsigned int getStackBaseOffset() const;
		void incrementStackBaseOffset(unsigned int value);
		void decrementStackBaseOffset(unsigned int value);
		
		unsigned int getReturnValueSize() const;
		
		bool wasImplemented() const;
		void setImplemented();
		
	private:
		typedef std::set<Register> RegisterSet;
		
		std::string name;
		
		// the type of this function
		Pointer<FunctionType> type;
		
		// offset of the stack base to the $SP
		// $SP + stackBaseOffset = stack base
		// the stack base points to the return address
		unsigned int stackBaseOffset;
		
		RegisterSet prRegisters;
		RegisterSet fpRegisters;
		
		// set to true after the function definition
		// this variable is used to avoid implementing a function twice
		bool implemented;
};

#endif
