#ifndef DECLARATOR_BASE_H
#define DECLARATOR_BASE_H

#include "compiler/Declarator.h"
#include "compiler/Type.h"

#include <parser/ParsingTree.h>
#include <parser/Pointer.h>

#include <string>

class DeclaratorBase : public Declarator {
	public:
		DeclaratorBase(const Pointer<Type> & t);
		virtual ~DeclaratorBase();
		
		virtual const std::string & getName() const;
		virtual void setName(const std::string & n);
		
		virtual const Pointer<Type> & getType() const;
		virtual void setType(const Pointer<Type> & t);
		
		virtual void setInitializer(ParsingTree::NonTerminal *nt);
		virtual ParsingTree::NonTerminal *getInitializer() const;
		virtual bool hasInitializer() const;
		
	protected:
		std::string name;
		Pointer<Type> type;
		
		ParsingTree::NonTerminal *initializer;
};

#endif
