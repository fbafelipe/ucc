#include "preprocessor/PreprocessorFileWrapper.h"

#include "preprocessor/InputWrapper.h"
#include "preprocessor/Preprocessor.h"
#include "preprocessor/PreprocessorContext.h"
#include "preprocessor/PreprocessorMacroScanner.h"
#include "PreprocessorParserBuffer.h"
#include "UccUtils.h"

#include <parser/ListInput.h>
#include <parser/MemoryInput.h>
#include <parser/OffsetInput.h>
#include <parser/Parser.h>

#include <cassert>
#include <cstdlib>
#include <cstring>

PreprocessorFileWrapper::PreprocessorFileWrapper(PreprocessorContext & preprocCtx,
		DefineMap & defMap) : PreprocessorParserBase(preprocCtx, defMap), inputWrapper(NULL) {}

PreprocessorFileWrapper::~PreprocessorFileWrapper() {
	delete(inputWrapper);
}

void PreprocessorFileWrapper::parse(Input *in) {
	assert(!inputWrapper);
	
	inputWrapper = new InputWrapper(in);
	
	const Preprocessor *preproc = preprocessorContext.getPreprocessor();
	
	preprocessorContext.setInput(in);
	
	Scanner *scan = new PreprocessorMacroScanner(preproc->getScannerAutomata(), in, defineMap);
	parser = new Parser(preproc->getParserTable(), scan);
	parser->setParserAction(this);
	
	NonTerminal *root = (NonTerminal *)parser->parse();
	delete(root);
	
	flushOutput();
}

void PreprocessorFileWrapper::recognized(NonTerminal *nt) {
	switch (nt->getNonTerminalId()) {
		case PREPROCESSORPARSERBUFFER_NONTERMINAL_CODE:
			writeCodeNonRecursive(nt);
			break;
		case PREPROCESSORPARSERBUFFER_NONTERMINAL_CONTROL_LINE:
			parseControlLine(nt);
			break;
		default:
			// just ignore it
			break;
	}
}

/*
 * <CONTROL_LINE> ::= <DEFINE_LINE>
 *		| <UNDEF_LINE>
 *		| <INCLUDE_LINE>
 *		| <CONDITIONAL>
 *		| <LINE_LINE>
 *		| <ERROR_LINE>
 *		| <WARNING_LINE>
 *		| <PRAGMA_LINE>
 *		;
 */
void PreprocessorFileWrapper::parseControlLine(NonTerminal *ctrlLine) {
	assert(ctrlLine->getNonTerminalId() == PREPROCESSORPARSERBUFFER_NONTERMINAL_CONTROL_LINE);
	assert(ctrlLine->getNonTerminalRule() == 4); // <CONTROL_LINE> ::= <LINE_LINE>
	
	parseLine(ctrlLine->getNonTerminalAt(0));
}

/*
 * <LINE_LINE> ::= LINE CONSTANT STRING_LITERAL DIRECTIVE_END
 *		| LINE CONSTANT DIRECTIVE_END
 *		;
 */
void PreprocessorFileWrapper::parseLine(NonTerminal *nonTerminal) {
	assert(nonTerminal->getNonTerminalId() == PREPROCESSORPARSERBUFFER_NONTERMINAL_LINE_LINE);
	
	flushOutput();
	
	if (nonTerminal->getNonTerminalRule() == 0) {
		int line = evaluateConstant(nonTerminal->getTokenAt(1)).intValue();
		const std::string & name = cleanToken(nonTerminal->getTokenAt(2));
		inputWrapper->setLocation(name, line);
	}
	else {
		assert(nonTerminal->getNonTerminalRule() == 1);
		
		int line = evaluateConstant(nonTerminal->getTokenAt(1)).intValue();
		inputWrapper->setLocation(inputWrapper->currentLocation().getName(), line);
	}
}

void PreprocessorFileWrapper::flushOutput() {
	InputLocation loc = inputWrapper->currentLocation();
	
	currentOutput.push_back('\n');
	Input *in = new MemoryInput(currentOutput);
	OffsetInput *offIn = new OffsetInput(in, loc.getLine() - 1, loc.getName());
	offIn->setRenameInput(true);
	in = offIn;
	listInput->addInput(in);
	
	currentOutput = "";
}

std::string PreprocessorFileWrapper::cleanToken(const std::string & token) const {
	assert(token.size() >= 2);
	
	char buf[token.size()];
	strcpy(buf, token.c_str() + 1); // remove the first character
	buf[strlen(buf) - 1] = '\0'; // remove the last character
	return std::string(buf);
}

std::string PreprocessorFileWrapper::cleanToken(Token *token) const {
	return cleanToken(token->getToken());
}
