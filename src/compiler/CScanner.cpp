#include "compiler/CScanner.h"

#include "compiler/TypeToken.h"
#include "compiler/TypedefManager.h"
#include "CParserBuffer.h"

CScanner::CScanner(const Pointer<ScannerAutomata> & a, Input *in) : Scanner(a, in) {}

CScanner::~CScanner() {}

ParsingTree::Token *CScanner::nextToken() {
	Token *token = Scanner::nextToken();
	
	if (token) {
		switch (token->getTokenTypeId()) {
			case CPARSERBUFFER_TOKEN_BEGIN:
				typedefManager.scopeBegin();
				break;
			case CPARSERBUFFER_TOKEN_END:
				typedefManager.scopeEnd();
				break;
			case CPARSERBUFFER_TOKEN_IDENTIFIER:
				if (typedefManager.isType(token->getToken())) {
					// save the type with the token
					Token *typeToken = new TypeToken(
							typedefManager.getType(token->getToken()),
							token->getToken(),
							token->getInputLocation());
					
					delete(token);
					token = typeToken;
				}
				break;
		}
	}
	
	return token;
}

TypedefManager & CScanner::getTypedefManager() {
	return typedefManager;
}
