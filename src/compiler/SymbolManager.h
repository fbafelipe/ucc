#ifndef SYMBOL_MANAGER_H
#define SYMBOL_MANAGER_H

#include "compiler/Variable.h"

#include <parser/Pointer.h>

#include <string>
#include <vector>

class GlobalSymbolTable;
class SymbolTable;

class SymbolManager {
	public:
		SymbolManager();
		~SymbolManager();
		
		void scopeBegin();
		void scopeEnd();
		
		GlobalSymbolTable *getGlobalSymbolTable() const;
		
		bool hasVariable(const std::string & var) const;
		Pointer<Variable> getVariable(const std::string & var) const;
		void addVariable(const std::string & name, const Pointer<Variable> & var);
		
	private:
		typedef std::vector<SymbolTable *> SymbolTableList;
		
		GlobalSymbolTable *globalSymbols;
		SymbolTableList symbolTables;
		
};

#endif
