#include "compiler/GlobalSymbolTable.h"

#include <cassert>

GlobalSymbolTable::GlobalSymbolTable() {}

GlobalSymbolTable::~GlobalSymbolTable() {}

bool GlobalSymbolTable::hasFunction(const std::string & func) const {
	return functions.find(func) != functions.end();
}

Pointer<Function> GlobalSymbolTable::getFunction(const std::string & func) const {
	FunctionMap::const_iterator it = functions.find(func);
	if (it != functions.end()) return it->second;
	return NULL;
}

void GlobalSymbolTable::addFunction(const std::string & name, const Pointer<Function> & func) {
	assert(!hasFunction(name));
	
	functions[name] = func;
}
