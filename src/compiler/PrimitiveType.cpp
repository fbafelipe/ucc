#include "compiler/PrimitiveType.h"

#include "compiler/PointerType.h"

template<typename _T>
PrimitiveType<_T>::PrimitiveType() {}

template<typename _T>
PrimitiveType<_T>::~PrimitiveType() {}

template<>
unsigned int PrimitiveType<void>::getSize() const {
	return 0;
}

template<typename _T>
unsigned int PrimitiveType<_T>::getSize() const {
	return sizeof(_T);
}

template<typename _T>
unsigned int PrimitiveType<_T>::getIncrement() const {
	if (isInteger()) return 1;
	return 0;
}

template<typename _T>
bool PrimitiveType<_T>::isInteger() const {
	return !isFloatingPoint();
}

template<>
bool PrimitiveType<float>::isFloatingPoint() const {
	return true;
}

template<>
bool PrimitiveType<double>::isFloatingPoint() const {
	return true;
}

template<>
bool PrimitiveType<long double>::isFloatingPoint() const {
	return true;
}

template<typename _T>
bool PrimitiveType<_T>::isFloatingPoint() const {
	return false;
}

template<>
bool PrimitiveType<void>::isVoid() const {
	return true;
}

template<typename _T>
bool PrimitiveType<_T>::isVoid() const {
	return false;
}

template<typename _T>
bool PrimitiveType<_T>::allowImplicitlyCastTo(const Pointer<Type> & other) const {
	return other.instanceOf<PrimitiveTypeBase>();
}

template<typename _T>
bool PrimitiveType<_T>::allowExplicitCastTo(const Pointer<Type> & other) const {
	return other.instanceOf<PrimitiveTypeBase>()
			|| (other.instanceOf<PointerType>() && !isFloatingPoint());
}

template<typename _T>
Type *PrimitiveType<_T>::clone() const {
	return new PrimitiveType();
}

template<typename _T>
bool PrimitiveType<_T>::operator==(const Type & other) const {
	return dynamic_cast<const PrimitiveType<_T> *>(&other);
}

template<> std::string PrimitiveType<void>::toString() const { return "void"; }

template<> std::string PrimitiveType<bool>::toString() const { return "bool"; }
template<> std::string PrimitiveType<char>::toString() const { return "char"; }
template<> std::string PrimitiveType<unsigned char>::toString() const { return "unsigned char void"; }

template<> std::string PrimitiveType<int>::toString() const { return "int"; }
template<> std::string PrimitiveType<short int>::toString() const { return "short int"; }
template<> std::string PrimitiveType<long int>::toString() const { return "long int"; }
template<> std::string PrimitiveType<long long int>::toString() const { return "long long int"; }

template<> std::string PrimitiveType<unsigned int>::toString() const { return "unsigned int"; }
template<> std::string PrimitiveType<unsigned short int>::toString() const { return "short int"; }
template<> std::string PrimitiveType<unsigned long int>::toString() const { return "unsigned long int"; }
template<> std::string PrimitiveType<unsigned long long int>::toString() const { return "unsigned long long int"; }

template<> std::string PrimitiveType<float>::toString() const { return "float"; }
template<> std::string PrimitiveType<double>::toString() const { return "double"; }
template<> std::string PrimitiveType<long double>::toString() const { return "long double"; }

template class PrimitiveType<void>;

template class PrimitiveType<bool>;
template class PrimitiveType<char>;
template class PrimitiveType<unsigned char>;

template class PrimitiveType<int>;
template class PrimitiveType<short int>;
template class PrimitiveType<long int>;
template class PrimitiveType<long long int>;

template class PrimitiveType<unsigned int>;
template class PrimitiveType<unsigned short int>;
template class PrimitiveType<unsigned long int>;
template class PrimitiveType<unsigned long long int>;

template class PrimitiveType<float>;
template class PrimitiveType<double>;
template class PrimitiveType<long double>;

