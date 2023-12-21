#include "preprocessor/PreprocessorScanner.h"

#include "preprocessor/DefineMap.h"
#include "PreprocessorParserBuffer.h"
#include "UccDefs.h"

#include <parser/Input.h>

#include <cassert>
#include <cstring>
#include <cstdlib>

PreprocessorScanner::PreprocessorScanner(const Pointer<ScannerAutomata> & a, Input *in) :
		Scanner(a, in), state(LINE_BEGIN), cachedToken(NULL) {}

PreprocessorScanner::~PreprocessorScanner() {
	delete(cachedToken);
}

ParsingTree::Token *PreprocessorScanner::nextToken() {
	Token *token = NULL;
	
	if (cachedToken) {
		token = cachedToken;
		cachedToken = NULL;
		return token;
	}
	
	switch (state) {
		case LINE_BEGIN: return readTokenLineBegin();
		case INCLUDE_LINE: return readTokenIncludeLine();
		case OTHER: return readTokenOther();
	}
	
	// unreachable
	abort();
}

ParsingTree::Token *PreprocessorScanner::readTokenLineBegin() {
	assert(state == LINE_BEGIN);
	assert(!cachedToken);
	
	Token *token = Scanner::nextToken();
	if (!token) return NULL;
	
	assert(!token->getToken().empty());
	
	if (token->getToken()[0] == '#') {
		if (token->getTokenTypeId() == PREPROCESSORPARSERBUFFER_TOKEN_INCLUDE) {
			state = INCLUDE_LINE;
		}
		else state = OTHER;
	}
	else token = readTextToken(token);
	
	return token;
}

ParsingTree::Token *PreprocessorScanner::readTokenIncludeLine() {
	assert(state == INCLUDE_LINE);
	assert(!cachedToken);
	
	Token *token = Scanner::nextToken();
	if (!token) return NULL;
	
	if (token->getTokenTypeId() == PREPROCESSORPARSERBUFFER_TOKEN_LESS) {
		InputLocation loc = token->getInputLocation();
		delete(token);
		token = readSysInclude(loc);
		assert(!cachedToken);
	}
	else if (token->getTokenTypeId() == PREPROCESSORPARSERBUFFER_TOKEN_STRING_LITERAL) {
		state = OTHER;
	}
	
	assert(state != INCLUDE_LINE);
	
	return token;
}

ParsingTree::Token *PreprocessorScanner::readTokenOther() {
	assert(state == OTHER);
	assert(!cachedToken);
	
	Token *token = Scanner::nextToken();
	if (!token) return NULL;
	
	if (token->getTokenTypeId() == PREPROCESSORPARSERBUFFER_TOKEN_DIRECTIVE_END) {
		state = LINE_BEGIN;
	}
	
	return token;
}

ParsingTree::Token *PreprocessorScanner::readSysInclude(InputLocation loc) {
	assert(!cachedToken);
	
	std::string result = "<";
	
	Input *input = getInput();
	
	char c = input->nextChar();
	while (c && c != '>' && c != '\n') {
		result.push_back(c);
		c = input->nextChar();
	}
	result.push_back('>');
	
	if (c != '>') throw ParserError(loc, "Missing \'>\'");
	
	state = OTHER;
	
	return new Token(PREPROCESSORPARSERBUFFER_TOKEN_FILENAME_SYSTEM, result, loc);
}

ParsingTree::Token *PreprocessorScanner::readTextToken(Token *startToken) {
	assert(!cachedToken);
	
	Token *token = startToken;
	
	InputLocation loc = token->getInputLocation();
	std::string tok;
	
	while (token && token->getToken()[0] != '#') {
		while (token && token->getTokenTypeId() != PREPROCESSORPARSERBUFFER_TOKEN_DIRECTIVE_END) {
			appendToken(tok, token);
			delete(token);
			
			// since appendToken may need to look forward,
			// it may put something in cachedToken
			if (cachedToken) token = cachedToken;
			else token = Scanner::nextToken();
		}
		
		if (token) {
			// just a '\n', no need to call appendToken
			appendToken(tok, token);
			delete(token);
			token = Scanner::nextToken();
		}
	}
	
	cachedToken = token;
	token = new Token(PREPROCESSORPARSERBUFFER_TOKEN_TEXT, tok, loc);
	
	return token;
}

void PreprocessorScanner::appendToken(std::string & result, Token *token) {
	const std::string & tok = token->getToken();
	
	for (std::string::const_iterator it = tok.begin(); it != tok.end(); ++it) {
		result.push_back(*it);
	}
	
	result.push_back(DELIMITER);
}
