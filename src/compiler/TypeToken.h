#ifndef TYPE_TOKEN_H
#define TYPE_TOKEN_H

#include "compiler/Type.h"

#include <parser/ParsingTree.h>
#include <parser/Pointer.h>

class Type;

// a TYPE_NAME token
class TypeToken : public ParsingTree::Token {
	public:
		TypeToken(const Pointer<Type> & t);
		TypeToken(const Pointer<Type> & t, const std::string & tok, const InputLocation & location);
		virtual ~TypeToken();
		
		const Pointer<Type> & getType() const;
		
	private:
		Pointer<Type> type;
};

#endif
