#include "preprocessor/PreprocessorExpScanner.h"

#include "preprocessor/DefineMap.h"
#include "PreprocExpParserBuffer.h"

#include <parser/MemoryInput.h>
#include <parser/OffsetInput.h>

#include <cstring>
#include <vector>

static std::string getMacroName(const std::string & macro);

PreprocessorExpScanner::PreprocessorExpScanner(const Pointer<ScannerAutomata> & a,
		Input *in, const DefineMap & defMap) : Scanner(a, in), defineMap(defMap) {}

PreprocessorExpScanner::~PreprocessorExpScanner() {}

ParsingTree::Token *PreprocessorExpScanner::nextToken() {
	Token *token = NULL;
	
	if (!tokenQueue.empty()) {
		token = tokenQueue.front();
		tokenQueue.pop();
	}
	else token = readToken();
	
	return token;
}

PreprocessorExpScanner::Token *PreprocessorExpScanner::readToken() {
	Token *token = Scanner::nextToken();
	if (token) token = checkExpand(token);
	return token;
}

PreprocessorExpScanner::Token *PreprocessorExpScanner::checkExpand(Token *token) {
	if (token->getTokenTypeId() == PREPROCEXPPARSERBUFFER_TOKEN_IDENTIFIER) {
		if (defineMap.defineDefined(token->getToken())) {
			expandDefine(token);
			token = nextToken();
		}
		else if (defineMap.macroDefined(token->getToken())) {
			Token *tok = Scanner::nextToken();
			if (tok->getTokenTypeId() != PREPROCEXPPARSERBUFFER_TOKEN_P_OPEN) {
				delete(tok);
				throw ParserError(tok->getInputLocation(), "Expecting \'(\'");
			}
			delete(tok);
			
			expandMacro(token);
			token = nextToken();
		}
	}
	else if (token->getTokenTypeId() == PREPROCEXPPARSERBUFFER_TOKEN_MACRO) {
		if (defineMap.macroDefined(token->getToken())) {
			expandMacro(token);
			token = nextToken();
		}
	}
	else if (token->getTokenTypeId() == PREPROCEXPPARSERBUFFER_TOKEN_DEFINED) {
		// do not expand the next identifier
		
		Token *tok = Scanner::nextToken();
		if (tok->getTokenTypeId() == PREPROCEXPPARSERBUFFER_TOKEN_P_OPEN) {
			tokenQueue.push(tok);
			tok = Scanner::nextToken();
		}
		tokenQueue.push(tok);
	}
	
	return token;
}

void PreprocessorExpScanner::expandDefine(Token *token) {
	std::string def = defineMap.getDefine(token->getToken());
	expand(def);
}

void PreprocessorExpScanner::expandMacro(Token *token) {
	Token *macroToken = token;
	
	DefineMap::Macro *macro = defineMap.getMacro(getMacroName(token->getToken()));
	
	DefineMap::ArgumentList args;
	token = nextToken();
	bool comma = false;
	bool first = true;
	while (token) {
		bool pClose = token->getTokenTypeId() == PREPROCEXPPARSERBUFFER_TOKEN_P_CLOSE;
		
		if ((first || comma) && pClose) break;
		first = false;
		
		if (comma) {
			if (token->getTokenTypeId() != PREPROCEXPPARSERBUFFER_TOKEN_COMMA) {
				throw ParserError(token->getInputLocation(), "Expecting \",\".");
			}
			comma = false;
			token = nextToken();
			continue;
		}
		comma = true;
		
		args.push_back(token->getToken());
		
		token = nextToken();
	}
	
	if (!token || token->getTokenTypeId() != PREPROCEXPPARSERBUFFER_TOKEN_P_CLOSE) {
		throw ParserError(macroToken->getInputLocation(), "Missing macro \")\".");
	}
	
	std::string def = macro->expandMacro(args);
	expand(def);
}

void PreprocessorExpScanner::expand(const std::string & def) {
	Input *in = new MemoryInput(def, getInput()->getInputName());
	in = new OffsetInput(in, getInput()->getInputLine());
	
	Scanner *scanner = new Scanner(getScannerAutomata(), in);
	
	Token *token;
	while ((token = scanner->nextToken())) {
		token = checkExpand(token);
		tokenQueue.push(token);
	}
	
	delete(scanner);
}

static std::string getMacroName(const std::string & macro) {
	assert(macro.size() > 1);
	
	if (macro[macro.size() - 1] == '(') {
		char buf[macro.size() + 1];
		strcpy(buf, macro.c_str());
		buf[macro.size() - 1] = '\0';
		
		return std::string(buf);
	}
	
	return macro;
}
