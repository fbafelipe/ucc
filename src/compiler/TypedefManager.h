#ifndef TYPEDEF_MANAGER_H
#define TYPEDEF_MANAGER_H

#include "compiler/Type.h"
#include "compiler/TypedefTable.h"

#include <parser/Pointer.h>

#include <vector>

class Type;

class TypedefManager {
	public:
		TypedefManager();
		~TypedefManager();
		
		void scopeBegin();
		void scopeEnd();
		
		bool isType(const std::string & name) const;
		
		const Pointer<Type> & getType(const std::string & name) const;
		void typeDef(const std::string & name, const Pointer<Type> & t);
		
	private:
		typedef std::vector<TypedefTable> TypedefTableList;
		
		TypedefTableList typedefs;
		
};

#endif
