#include "compiler/TypeToken.h"

#include "CParserBuffer.h"

TypeToken::TypeToken(const Pointer<Type> & t) : ParsingTree::Token(CPARSERBUFFER_TOKEN_TYPE_NAME), type(t) {}

TypeToken::TypeToken(const Pointer<Type> & t, const std::string & tok, const InputLocation & location) :
		ParsingTree::Token(CPARSERBUFFER_TOKEN_TYPE_NAME, tok, location), type(t) {}

TypeToken::~TypeToken() {}

const Pointer<Type> & TypeToken::getType() const {
	return type;
}
