#ifndef ARRAY_TYPE_H
#define ARRAY_TYPE_H

#include "compiler/Type.h"

#include <parser/Pointer.h>

class ArrayType : public Type {
	public:
		ArrayType(const Pointer<Type> & t, int c = -1);
		virtual ~ArrayType();
		
		int getCount() const;
		bool isCountDefined() const;
		
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
		
		// the number of elements or -1 if not defined
		int count;
};

#endif
