#include "preprocessor/PreprocessorMacroScanner.h"

#include "preprocessor/DefineMap.h"
#include "preprocessor/ComposedScannerWrapper.h"
#include "preprocessor/DefaultScannerWrapper.h"
#include "PreprocessorParserBuffer.h"
#include "PreprocessorMacroIdScannerBuffer.h"
#include "UccDefs.h"

#include <parser/Input.h>
#include <parser/MemoryInput.h>
#include <parser/OffsetInput.h>
#include <parser/ParserLoader.h>

#include <cstring>

// we need a smart pointer to ScannerWrapper, but making a smart pointer from
// this would cause a double free
class ScannerWrapperWrapper : public ScannerWrapper {
	public:
		ScannerWrapperWrapper(ScannerWrapper *wrap) : wrapper(wrap) {}
		virtual ~ScannerWrapperWrapper() {}
		
		virtual ParsingTree::Token *scannerNextToken() {
			return wrapper->scannerNextToken();
		}
		
	private:
		ScannerWrapper *wrapper;
};

PreprocessorMacroScanner::PreprocessorMacroScanner(const Pointer<ScannerAutomata> & a, Input *in,
				const DefineMap & defMap) : PreprocessorScanner(a, in), defineMap(defMap) {
	
	if (!preprocessorMacroIdScannerAutomata) {
		preprocessorMacroIdScannerAutomata = ParserLoader::bufferToAutomata(prepro_macro_id_buffer_scanner);
	}
}

PreprocessorMacroScanner::~PreprocessorMacroScanner() {}

ParsingTree::Token *PreprocessorMacroScanner::readTextToken(Token *startToken) {
	assert(!cachedToken);
	
	Token *token = startToken;
	if (!token || token->getTokenTypeId() == PREPROCESSORPARSERBUFFER_TOKEN_DIRECTIVE_END) return token;
	
	InputLocation loc = token->getInputLocation();
	std::string tok;
	
	while (token && token->getTokenTypeId() != PREPROCESSORPARSERBUFFER_TOKEN_DIRECTIVE_END) {
		IdentifierSet expanded;
		Pointer<ScannerWrapper> wrapper = new ScannerWrapperWrapper(this);
		append(wrapper, tok, token, expanded);
		
		delete(token);
		
		token = scannerNextToken();
	}
	cachedToken = token;
	
	token = new Token(PREPROCESSORPARSERBUFFER_TOKEN_TEXT, tok, loc);
	
	return token;
}

ParsingTree::Token *PreprocessorMacroScanner::scannerNextToken() {
	assert(!cachedToken);
	return Scanner::nextToken();
}

void PreprocessorMacroScanner::append(const Pointer<ScannerWrapper> & wrapper, std::string & result,
		Token *token, IdentifierSet & expanded) {
	
	if (isDefined(token, expanded)) expand(wrapper, result, token, expanded);
	else appendToken(result, token);
}

void PreprocessorMacroScanner::expand(const Pointer<ScannerWrapper> & wrapper, std::string & result,
		Token *token, IdentifierSet & expanded) {
	
	if (isIdentifierToken(token)) {
		const std::string & name = token->getToken();
		
		if (defineMap.defineDefined(name)) expandDefine(wrapper, result, name, expanded);
		else {
			assert(defineMap.macroDefined(name));
			
			Token *t = wrapper->scannerNextToken();
			if (t->getToken() == "(") expandMacro(wrapper, result, name, expanded);
			else {
				appendToken(result, token);
				cachedToken = t;
			}
		}
	}
	else if (isMacroToken(token)) {
		std::string name = getMacroName(token);
		
		if (defineMap.defineDefined(name)) expandDefine(wrapper, result, name, expanded);
		else {
			assert(defineMap.macroDefined(name));
			expandMacro(wrapper, result, name, expanded);
		}
	}
}

void PreprocessorMacroScanner::expandDefine(const Pointer<ScannerWrapper> & wrapper, std::string & result,
		const std::string & defineName, IdentifierSet & expanded) {
	
	defExpanded(wrapper, result, defineMap.getDefine(defineName), expanded);
}

void PreprocessorMacroScanner::expandMacro(const Pointer<ScannerWrapper> & wrapper, std::string & result,
		const std::string & macroName, IdentifierSet & expanded) {
	
	assert(!cachedToken);
	
	DefineMap::Macro *macro = defineMap.getMacro(macroName);
	assert(macro);
	
	DefineMap::ArgumentList args;
	
	unsigned int numVars = macro->getVariablesCount();
	
	for (unsigned int i = 0; i < numVars; ++i) {
		std::string arg;
		readMacroArgument(wrapper, arg, expanded);
		args.push_back(arg);
		
		if (i < numVars - 1) {
			assert(cachedToken);
			Token *t = cachedToken;
			cachedToken = NULL;
			if (t->getToken() != ",") {
				InputLocation loc = t->getInputLocation();
				delete(t);
				throw ParserError(loc, "Expecting \',\'");
			}
			delete(t);
		}
	}
	
	assert(cachedToken);
	Token *t = cachedToken;
	cachedToken = NULL;
	if (t->getToken() != ")") {
		InputLocation loc = t->getInputLocation();
		delete(t);
		throw ParserError(loc, "Expecting \')\'");
	}
	delete(t);
	
	std::string macroResult = macro->expandMacro(args);
	
	defExpanded(wrapper, result, macroResult, expanded);
}

void PreprocessorMacroScanner::defExpanded(const Pointer<ScannerWrapper> & wrapper,
		std::string & result, const std::string & expansion, IdentifierSet & expanded) {
	
	InputLocation loc = getInput()->getCurrentLocation();
	Input *in = new OffsetInput(new MemoryInput(expansion), loc.getLine() - 1, loc.getName());
	Pointer<ScannerWrapper> w = new DefaultScannerWrapper(new Scanner(preprocessorMacroIdScannerAutomata, in));
	
	Pointer<ScannerWrapper> composed = new ComposedScannerWrapper(w, wrapper);
	
	Token *token;
	while ((token = w->scannerNextToken())) {
		result.push_back(DELIMITER);
		append(composed, result, token, expanded);
		delete(token);
	}
	result.push_back(DELIMITER);
}

void PreprocessorMacroScanner::readMacroArgument(const Pointer<ScannerWrapper> & wrapper, std::string & result,
		IdentifierSet & expanded) {
	
	Token *t = NULL;
	int openPar = 0;
	
	while (openPar >= 0) {
		t = wrapper->scannerNextToken();
		if (!t) throw ParserError("Unexpected end of file.");
		
		if (t->getToken() == ")") {
			if (--openPar < 0) break;
		}
		else if (t->getToken() == "(") ++openPar;
		else if (t->getToken() == "," && openPar <= 0)  break;
		
		appendToken(result, t);
	}
	
	assert(t);
	cachedToken = t;
}

bool PreprocessorMacroScanner::isDefined(Token *token, IdentifierSet & expanded) {
	if (isIdentifierToken(token)) {
		const std::string & name = token->getToken();
		if (defineMap.isDefined(name) && expanded.find(name) == expanded.end()) {
			expanded.insert(name);
			return true;
		}
	}
	else if (isMacroToken(token)) {
		const std::string & name = getMacroName(token);
		if (defineMap.isDefined(name) && expanded.find(name) == expanded.end()) {
			expanded.insert(name);
			return true;
		}
	}
	
	return false;
}

std::string PreprocessorMacroScanner::getMacroName(Token *token) const {
	assert(token);
	const std::string & tok = token->getToken();
	if (token->getTokenTypeId() == PREPROCESSORPARSERBUFFER_TOKEN_IDENTIFIER) return tok;
	assert(token->getTokenTypeId() == PREPROCESSORPARSERBUFFER_TOKEN_MACRO);
	
	assert(tok.size() > 1);
	
	char buf[tok.size() + 1];
	strcpy(buf, tok.c_str());
	
	// remove the '('
	buf[tok.size() - 1] = '\0';
	
	return std::string(buf);
}

bool PreprocessorMacroScanner::isIdentifierToken(Token *token) const {
	assert(token);
	
	// ScannerWrapper may use a different id, so we need to check thy the regex
	return identifierRegex.matches(token->getToken());
}

bool PreprocessorMacroScanner::isMacroToken(Token *token) const {
	assert(token);
	
	// ScannerWrapper may use a different id, so we need to check thy the regex
	return macroRegex.matches(token->getToken());
}

Regex PreprocessorMacroScanner::identifierRegex = Regex("\\w(\\w|\\d)*");
Regex PreprocessorMacroScanner::macroRegex = Regex("\\w(\\w|\\d)*\\(");

Pointer<ScannerAutomata> PreprocessorMacroScanner::preprocessorMacroIdScannerAutomata;
