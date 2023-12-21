#ifndef POINTER_TYPE_H
#define POINTER_TYPE_H

#include "compiler/Type.h"

#include <parser/Pointer.h>

class PointerType : public Type {
	public:
		PointerType(const Pointer<Type> & t);
		virtual ~PointerType();
		
		virtual const Pointer<Type> & dereference() const;
		
		virtual unsigned int getSize() const;
		virtual unsigned int getIncrement() const;
		
		virtual TypeEnum getTypeEnum() const;
		
		virtual Type & getBaseType();
		
		virtual bool allowImplicitlyCastTo(const Pointer<Type> & other) const;
		virtual bool allowExplicitCastTo(const Pointer<Type> & other) const;
		
		virtual Type *clone() const;
		
		virtual bool operator==(const Type & other) const;
		
		virtual std::string toString() const;
		
	private:
		Pointer<Type> baseType;
};

#endif
