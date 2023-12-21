#ifndef INDIRECT_DECLARATOR_H
#define INDIRECT_DECLARATOR_H

#include "compiler/Declarator.h"

class IndirectDeclarator : public Declarator {
	public:
		IndirectDeclarator(const Pointer<Declarator> & decl);
		virtual ~IndirectDeclarator();
		
		virtual const std::string & getName() const;
		virtual void setName(const std::string & n);
		
		virtual const Pointer<Type> & getType() const;
		virtual void setType(const Pointer<Type> & t);
		
		virtual void setInitializer(ParsingTree::NonTerminal *nt);
		virtual ParsingTree::NonTerminal *getInitializer() const;
		virtual bool hasInitializer() const;
		
	protected:
		Pointer<Declarator> base;
};

#endif
