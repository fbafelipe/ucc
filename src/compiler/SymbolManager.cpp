#include "compiler/SymbolManager.h"

#include "compiler/GlobalSymbolTable.h"
#include "compiler/SymbolTable.h"

#include <cassert>

SymbolManager::SymbolManager() {
	globalSymbols = new GlobalSymbolTable();
	symbolTables.push_back(globalSymbols);
}

SymbolManager::~SymbolManager() {
	for (SymbolTableList::iterator it = symbolTables.begin(); it != symbolTables.end(); ++it) {
		delete(*it);
	}
}

void SymbolManager::scopeBegin() {
	symbolTables.push_back(new SymbolTable());
}

void SymbolManager::scopeEnd() {
	delete(symbolTables.back());
	symbolTables.pop_back();
	
	assert(!symbolTables.empty());
}

GlobalSymbolTable *SymbolManager::getGlobalSymbolTable() const {
	return globalSymbols;
}

bool SymbolManager::hasVariable(const std::string & var) const {
	for (SymbolTableList::const_reverse_iterator it = symbolTables.rbegin(); it != symbolTables.rend(); ++it) {
		if ((*it)->hasVariable(var)) return true;
	}
	
	return false;
}

Pointer<Variable> SymbolManager::getVariable(const std::string & var) const {
	for (SymbolTableList::const_reverse_iterator it = symbolTables.rbegin(); it != symbolTables.rend(); ++it) {
		if ((*it)->hasVariable(var)) return (*it)->getVariable(var);
	}
	
	return NULL;
}

void SymbolManager::addVariable(const std::string & name, const Pointer<Variable> & var) {
	assert(!hasVariable(name));
	
	symbolTables.back()->addVariable(name, var);
}
