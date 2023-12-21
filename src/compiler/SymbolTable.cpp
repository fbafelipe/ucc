#include "compiler/SymbolTable.h"

#include <cassert>

SymbolTable::SymbolTable() {}

SymbolTable::~SymbolTable() {}

bool SymbolTable::hasVariable(const std::string & var) const {
	return variables.find(var) != variables.end();
}

Pointer<Variable> SymbolTable::getVariable(const std::string & var) const {
	VariableMap::const_iterator it = variables.find(var);
	if (it != variables.end()) return it->second;
	return NULL;
}

void SymbolTable::addVariable(const std::string & name, const Pointer<Variable> & var) {
	assert(!hasVariable(name));
	
	variables[name] = var;
}
