#include "compiler/FunctionDeclarator.h"

#include "compiler/FunctionType.h"
#include "compiler/PointerType.h"

FunctionDeclarator::FunctionDeclarator(const Pointer<Declarator> & decl,
		const DeclaratorList & params, bool el) : IndirectDeclarator(decl),
		parameters(params), ellipsis(el) {
	
	setType(base->getType());
}

FunctionDeclarator::~FunctionDeclarator() {}

void FunctionDeclarator::setUndefinedParameters(bool un) {
	assert(base->getType().instanceOf<FunctionType>());
	base->getType().staticCast<FunctionType>()->setUndefined(un);
}

const Pointer<Type> & FunctionDeclarator::getReturnType() const {
	return returnType;
}

void FunctionDeclarator::setType(const Pointer<Type> & t) {
	bool undefinedParameters = false;
	
	assert(base);
	if (base->getType().instanceOf<FunctionType>()) {
		undefinedParameters = base->getType().staticCast<FunctionType>()->isUndefined();
	}
	
	returnType = t;
	
	TypeList typeList;
	getParametersTypeList(typeList);
	base->setType(new FunctionType(returnType, typeList, ellipsis));
	
	if (undefinedParameters) setUndefinedParameters(undefinedParameters);
}

const DeclaratorList & FunctionDeclarator::getParameters() {
	return parameters;
}

void FunctionDeclarator::getParametersTypeList(TypeList & typeList) const {
	for (DeclaratorList::const_iterator it = parameters.begin(); it != parameters.end(); ++it) {
		typeList.push_back((*it)->getType());
	}
}

bool FunctionDeclarator::hasEllipsis() const {
	return ellipsis;
}
