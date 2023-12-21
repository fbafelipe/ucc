#ifndef DECLARATION_H
#define DECLARATION_H

#include "compiler/Declarator.h"
#include "compiler/Type.h"

#include <parser/Pointer.h>

#include <vector>

class Declaration {
	public:
		Declaration();
		~Declaration();
		
		const Pointer<Type> & getBaseType() const;
		void setBaseType(const Pointer<Type> & t);
		
		void addDeclarator(const Pointer<Declarator> & v);
		const DeclaratorList & getDeclarators() const;
		
		bool isTypeDef() const;
		void setTypedef(bool td);
		
		bool isExtern() const;
		void setExtern(bool ext);
		
		bool isStatic() const;
		void setStatic(bool s);
		
	private:
		Pointer<Type> baseType;
		
		DeclaratorList declarators;
		
		bool typedefDecl;
		bool externDecl;
		bool staticDecl;
};

typedef std::vector<Pointer<Declaration> > DeclarationList;

#endif
