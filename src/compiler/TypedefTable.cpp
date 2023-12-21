#include "compiler/TypedefTable.h"

#include <cassert>

TypedefTable::TypedefTable() {}

TypedefTable::~TypedefTable() {}

bool TypedefTable::isType(const std::string & name) const {
	return typeMap.find(name) != typeMap.end();
}

const Pointer<Type> & TypedefTable::getType(const std::string & name) const {
	assert(isType(name));
	
	return typeMap.find(name)->second;
}

void TypedefTable::typeDef(const std::string & name, const Pointer<Type> & t) {
	assert(!isType(name));
	
	typeMap[name] = t;
}
