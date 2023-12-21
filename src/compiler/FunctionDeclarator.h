#ifndef FUNCTION_DECLARATOR_H
#define FUNCTION_DECLARATOR_H

#include "compiler/IndirectDeclarator.h"

#include "compiler/Type.h"

class FunctionDeclarator : public IndirectDeclarator {
	public:
		FunctionDeclarator(const Pointer<Declarator> & decl,
				const DeclaratorList & params = DeclaratorList(),
				bool el = false);
		virtual ~FunctionDeclarator();
		
		void setUndefinedParameters(bool un);
		
		const Pointer<Type> & getReturnType() const;
		virtual void setType(const Pointer<Type> & t);
		
		const DeclaratorList & getParameters();
		void getParametersTypeList(TypeList & typeList) const;
		bool hasEllipsis() const;
		
	private:
		Pointer<Type> returnType;
		DeclaratorList parameters;
		bool ellipsis;
};

#endif
