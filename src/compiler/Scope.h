#ifndef SCOPE_H
#define SCOPE_H

#include "vm/RegisterUtils.h"

#include <vector>

class CompilerContext;
class JumpInstruction;

class Scope {
	public:
		enum ScopeFlag {
			CAN_BREAK = 1,
			CAN_CONTINUE = 2
		};
		
		Scope(unsigned int b);
		~Scope();
		
		void setInstructionsEnd(unsigned int e);
		
		// inclusive
		unsigned int getInstructionsBegin() const;
		
		// exclusive
		unsigned int getInstructionsEnd() const;
		
		unsigned int getInstructionCount() const;
		
		// only local variables stack memory should add here
		// do not add temporaries
		void allocateStack(unsigned int s);
		
		unsigned int getAllocatedStack() const;
		
		void addScopeFlag(ScopeFlag flag);
		bool hasScopeFlag(ScopeFlag flag) const;
		unsigned int getScopeFlags() const;
		void setScopeFlags(unsigned int flags);
		
		// create a JumpInstruction to the begin of the scope
		JumpInstruction *createJumpInstScopeBegin(CompilerContext & context);
		
		// create a JumpInstruction to the end of the scope
		JumpInstruction *createJumpInstScopeEnd(CompilerContext & context);
		
	private:
		typedef std::vector<JumpInstruction *> InstOffsetList;
		
		unsigned int begin;
		unsigned int end;
		
		// how much stack memory this scope is using
		unsigned int allocatedStack;
		
		unsigned int scopeFlags;
		
		// list with all instructions that need the offset to the end of the scope
		// the JumpInstruction should contain the offset from the scope begin when
		// it's added in the instOffsetList
		// when the scope end, the instruction will be updated to contain the offset from
		// the position of the instruction to the end of the scope
		InstOffsetList instOffsetList;
};

#endif
