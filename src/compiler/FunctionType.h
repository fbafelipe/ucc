#ifndef FUNCTION_TYPE_H
#define FUNCTION_TYPE_H

#include "compiler/Type.h"

#include <parser/Pointer.h>

class FunctionType : public Type {
	public:
		FunctionType(const Pointer<Type> & ret, const TypeList & tList, bool el = false);
		virtual ~FunctionType();
		
		const Pointer<Type> & getReturnType() const;
		
		const TypeList & getTypeList() const;
		void setTypeList(const TypeList & tList);
		
		bool hasEllipsis() const;
		void setEllipsis(bool el);
		
		bool isUndefined() const;
		void setUndefined(bool un);
		
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
		Pointer<Type> returnType;
		
		TypeList typeList;
		
		bool ellipsis;
		
		bool undefined;
};

#endif
