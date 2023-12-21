#ifndef EXP_RESULT_H
#define EXP_RESULT_H

#include "compiler/Type.h"
#include "vm/RegisterUtils.h"
#include "Number.h"

#include <parser/Pointer.h>

#include <string>

class CompilerContext;

class ExpResult {
	public:
		enum ResultType {
			// a numeric constant
			CONSTANT,
			
			// a value in the stack
			STACKED,
			
			// no result (call to a function with return type void)
			VOID
		};
		
		ExpResult();
		ExpResult(ResultType t);
		~ExpResult();
		
		const Pointer<Type> & getType() const;
		void setType(const Pointer<Type> & t);
		
		ResultType getResultType() const;
		void setResultType(ResultType t);
		
		// valid only when ResultType is CONSTANT
		const Number & getConstant() const;
		void setConstant(const Number & c);
		void setConstant(unsigned int c);
		
		// valid only when ResultType is STACKED
		unsigned int getOffsetFromBase() const;
		void setOffsetFromBase(unsigned int off);
		// set for the offset assuming that the value was pushed before this call
		void setOffsetFromBase(CompilerContext & context);
		bool isNeedDeallocate() const;
		void setNeedDealocate(bool n);
		bool isDynamic() const;
		void setDynamic(bool d);
		unsigned int getSPOffset(const CompilerContext & context) const;
		unsigned int getStackAllocSize() const; // return the size this result uses from the stack 
		
		// allocate a register with the value of the expression
		// the type cannot be a struct/union
		Register getValue(CompilerContext & context) const;
		
		// allocate a register with the position where the value is
		// it will point to the stack
		// valid only when ResultType is STACKED
		Register getPointer(CompilerContext & context) const;
		
	private:
		ResultType resultType;
		
		Pointer<Type> type;
		
		// used only when ResultType is CONSTANT
		Number constantResult;
		
		// used only when ResultType is STACKED
		// offset from stack base, the address in stak is StackBase - offsetFromBase
		unsigned int offsetFromBase;
		bool needDeallocate; // true if it need to be deallocated
		bool dynamic; // true if the value in the stack is a pointer to the real value
};

#endif
