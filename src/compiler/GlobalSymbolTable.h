#ifndef GLOBAL_SYMBOL_TABLE_H
#define GLOBAL_SYMBOL_TABLE_H

#include "compiler/Function.h"
#include "compiler/SymbolTable.h"

class GlobalSymbolTable : public SymbolTable {
	public:
		GlobalSymbolTable();
		virtual ~GlobalSymbolTable();
		
		bool hasFunction(const std::string & func) const;
		Pointer<Function> getFunction(const std::string & func) const;
		void addFunction(const std::string & name, const Pointer<Function> & func);
		
	private:
		typedef std::map<std::string, Pointer<Function> > FunctionMap;
		
		FunctionMap functions;
		
};

#endif
