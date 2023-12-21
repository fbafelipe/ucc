#include "compiler/TypedefManager.h"

#include <cassert>
#include <cstdlib>

TypedefManager::TypedefManager() {
	// insert the root TypedefTable (global scope)
	scopeBegin();
}

TypedefManager::~TypedefManager() {}

void TypedefManager::scopeBegin() {
	typedefs.push_back(TypedefTable());
}

void TypedefManager::scopeEnd() {
	typedefs.pop_back();
	
	if (typedefs.empty()) {
		// we have a syntax error with an extra '}',
		// but we cant leave typedefs empty
		// let's create another scope and let the parser generate an error message
		scopeBegin();
	}
}

bool TypedefManager::isType(const std::string & name) const {
	for (TypedefTableList::const_reverse_iterator it = typedefs.rbegin(); it != typedefs.rend(); ++it) {
		if (it->isType(name)) return true;
	}
	
	return false;
}

const Pointer<Type> & TypedefManager::getType(const std::string & name) const {
	assert(isType(name));
	
	for (TypedefTableList::const_reverse_iterator it = typedefs.rbegin(); it != typedefs.rend(); ++it) {
		if (it->isType(name)) return it->getType(name);
	}
	
	// name is not a type
	abort();
}

void TypedefManager::typeDef(const std::string & name, const Pointer<Type> & t) {
	assert(!isType(name));
	
	typedefs.back().typeDef(name, t);
}
