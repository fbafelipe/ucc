#ifndef VARIABLE_H
#define VARIABLE_H

#include "compiler/Type.h"

#include <parser/Pointer.h>

#include <vector>

class Variable {
	public:
		enum VariableType {
			GLOBAL,
			LOCAL
		};
		
		Variable();
		Variable(const Pointer<Type> & t, VariableType vt, unsigned int pos = 0);
		~Variable();
		
		const Pointer<Type> & getType() const;
		void setType(const Pointer<Type> & t);
		
		bool isGlobal() const;
		bool isLocal() const;
		
		void setGlobal();
		void setLocal();
		
		VariableType getVariableType() const;
		void setVariableType(VariableType t);
		
		unsigned int getPosition() const;
		void setPosition(unsigned int pos);
		
	private:
		Pointer<Type> type;
		VariableType variableType;
		
		// if global relative to REG_GP
		// if local relative to REG_SP + stackBaseOffset
		// if dynamic then position is meaning less and souldn't be used
		unsigned int position;
};

typedef std::vector<Variable> VariableList;

#endif
