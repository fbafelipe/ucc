#include "compiler/FunctionType.h"

#include "compiler/PrimitiveType.h"
#include "vm/RegisterUtils.h"

#include <cassert>
#include <cstdlib>

FunctionType::FunctionType(const Pointer<Type> & ret, const TypeList & tList,
		bool el) : returnType(ret), typeList(tList), ellipsis(el), undefined(false) {}

FunctionType::~FunctionType() {}

const Pointer<Type> & FunctionType::getReturnType() const {
	return returnType;
}

const TypeList & FunctionType::getTypeList() const {
	return typeList;
}

void FunctionType::setTypeList(const TypeList & tList) {
	typeList = tList;
}

bool FunctionType::hasEllipsis() const {
	return ellipsis;
}

void FunctionType::setEllipsis(bool el) {
	ellipsis = el;
}

bool FunctionType::isUndefined() const {
	return undefined;
}

void FunctionType::setUndefined(bool un) {
	undefined = un;
}

unsigned int FunctionType::getSize() const {
	return REGISTER_SIZE;
}

unsigned int FunctionType::getIncrement() const {
	return getSize();
}

Type::TypeEnum FunctionType::getTypeEnum() const {
	return FUNCTION;
}

Type & FunctionType::getBaseType() {
	return *this;
}

bool FunctionType::allowImplicitlyCastTo(const Pointer<Type> & other) const {
	return *this == *other;
}

bool FunctionType::allowExplicitCastTo(const Pointer<Type> & other) const {
	if (other.instanceOf<PrimitiveTypeBase>()) {
		return other.instanceOf<PrimitiveType<int> >()
				|| other.instanceOf<PrimitiveType<long int> >()
				|| other.instanceOf<PrimitiveType<long long int> >()
				|| other.instanceOf<PrimitiveType<unsigned int> >()
				|| other.instanceOf<PrimitiveType<unsigned long int> >()
				|| other.instanceOf<PrimitiveType<unsigned long long int> >();
	}
	
	return getSize() == other->getSize();
}

Type *FunctionType::clone() const {
	return new FunctionType(returnType, typeList, ellipsis);
}

bool FunctionType::operator==(const Type & other) const {
	if (!dynamic_cast<const FunctionType *>(&other)) return false;
	
	const FunctionType & t = (const FunctionType &)other;
	if (*returnType != *t.returnType) return false;
	if (ellipsis != t.ellipsis) return false;
	if (typeList.size() != t.typeList.size()) return false;
	
	for (unsigned int i = 0; i < typeList.size(); ++i) {
		if (*typeList[i] != *t.typeList[i]) return false;
	}
	
	return true;
}

std::string FunctionType::toString() const {
	std::string result =  returnType->toString() + " function(";
	
	for (unsigned int i = 0; i < typeList.size(); ++i) {
		result += typeList[i]->toString();
		if (i < typeList.size() - 1) result += ", ";
	}
	
	result += ")";
	
	return result;
}
