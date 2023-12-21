#ifndef TYPEDEF_TABLE_H
#define TYPEDEF_TABLE_H

#include "compiler/Type.h"

#include <parser/Pointer.h>

#include <map>
#include <string>

class TypedefTable {
	public:
		TypedefTable();
		~TypedefTable();
		
		bool isType(const std::string & name) const;
		
		const Pointer<Type> & getType(const std::string & name) const;
		void typeDef(const std::string & name, const Pointer<Type> & t);
		
	private:
		typedef std::map<std::string, Pointer<Type> > TypeMap;
		
		TypeMap typeMap;
};

#endif
