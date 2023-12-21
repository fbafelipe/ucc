#ifndef TYPE_H
#define TYPE_H

#include <parser/Pointer.h>

#include <ostream>
#include <string>
#include <vector>

class Type {
	public:
		enum TypeEnum {
			PRIMITIVE,
			STRUCT,
			UNION,
			FUNCTION,
			POINTER,
			ARRAY
		};
		
		// return the result type from a binary operation or NULL if no implicitly cast is allowed
		static Pointer<Type> getResultingType(const Pointer<Type> & t1, const Pointer<Type> & t2);
		
		Type();
		virtual ~Type();
		
		// used to determine if a PR or FP register will be used
		virtual bool isFloatingPoint() const;
		
		virtual bool isVoid() const;
		
		virtual unsigned int getSize() const = 0;
		
		// return the increment. Integers will return 1
		// pointers will return the size of what it points to
		// types that cannot be incremented will return 0
		virtual unsigned int getIncrement() const = 0;
		
		virtual bool isConstant() const;
		virtual void setConstant(bool c);
		
		virtual bool isVolatile() const;
		virtual void setVolatile(bool v);
		
		virtual TypeEnum getTypeEnum() const = 0;
		
		virtual Type & getBaseType() = 0;
		
		virtual bool allowImplicitlyCastTo(const Pointer<Type> & other) const = 0;
		virtual bool allowExplicitCastTo(const Pointer<Type> & other) const = 0;
		
		// return true if a value of this type fits in a register
		virtual bool fitRegister() const;
		
		virtual Type *clone() const = 0;
		
		virtual bool operator==(const Type & other) const = 0;
		bool operator!=(const Type & other) const;
		
		virtual std::string toString() const = 0;
		
		friend std::ostream & operator<<(std::ostream & stream, const Type & type);
		
	protected:
		bool constant;
		bool volatileType;
};

typedef std::vector<Pointer<Type> > TypeList;

#endif
