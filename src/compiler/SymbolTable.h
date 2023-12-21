#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "compiler/Type.h"
#include "compiler/Variable.h"

#include <parser/Pointer.h>

#include <map>
#include <string>

class SymbolTable {
	public:
		SymbolTable();
		virtual ~SymbolTable();
		
		bool hasVariable(const std::string & var) const;
		Pointer<Variable> getVariable(const std::string & var) const;
		void addVariable(const std::string & name, const Pointer<Variable> & var);
		
	private:
		typedef std::map<std::string, Pointer<Variable> > VariableMap;
		
		VariableMap variables;
};

#endif
