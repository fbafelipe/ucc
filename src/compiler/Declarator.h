#ifndef DECLARATOR_H
#define DECLARATOR_H

#include <parser/ParsingTree.h>
#include <parser/Pointer.h>

#include <string>
#include <vector>

class Type;

class Declarator {
	public:
		Declarator();
		virtual ~Declarator();
		
		virtual const std::string & getName() const = 0;
		virtual void setName(const std::string & n) = 0;
		
		virtual const Pointer<Type> & getType() const = 0;
		virtual void setType(const Pointer<Type> & t) = 0;
		
		// not yet parsed initializer
		virtual void setInitializer(ParsingTree::NonTerminal *nt) = 0;
		virtual ParsingTree::NonTerminal *getInitializer() const = 0;
		virtual bool hasInitializer() const = 0;
};

typedef std::vector<Pointer<Declarator> > DeclaratorList;

#endif
