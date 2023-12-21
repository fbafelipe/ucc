#ifndef PRIMITIVE_TYPE_H
#define PRIMITIVE_TYPE_H

#include "compiler/PrimitiveTypeBase.h"

#include <cstdlib>

template<typename _T>
class PrimitiveType : public PrimitiveTypeBase {
	public:
		PrimitiveType();
		virtual ~PrimitiveType();
		
		virtual unsigned int getSize() const;
		
		virtual unsigned int getIncrement() const;
		
		virtual bool isInteger() const;
		
		virtual bool isFloatingPoint() const;
		
		virtual bool isVoid() const;
		
		virtual bool allowImplicitlyCastTo(const Pointer<Type> & other) const;
		
		virtual bool allowExplicitCastTo(const Pointer<Type> & other) const;
		
		virtual Type *clone() const;
		
		virtual bool operator==(const Type & other) const;
		
		virtual std::string toString() const;
};

#endif
