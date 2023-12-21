#ifndef ARRAY_DECLARATOR_H
#define ARRAY_DECLARATOR_H

#include "compiler/IndirectDeclarator.h"

class ArrayDeclarator : public IndirectDeclarator {
	public:
		ArrayDeclarator(const Pointer<Declarator> & decl, int c = -1);
		virtual ~ArrayDeclarator();
		
		const Pointer<Type> & getElementType() const;
		virtual void setType(const Pointer<Type> & t);
		
		int getCount() const;
		void setCount(int c);
		
	protected:
		Pointer<Type> elementType;
		
		// the number of elements or -1
		int count;
};

#endif
