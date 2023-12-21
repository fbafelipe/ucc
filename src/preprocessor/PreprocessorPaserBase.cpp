#include "PreprocessorParserBase.h"

#include "preprocessor/DefineMap.h"
#include "preprocessor/Preprocessor.h"
#include "preprocessor/PreprocessorContext.h"
#include "preprocessor/PreprocessorScanner.h"
#include "PreprocessorParserBuffer.h"
#include "UccDefs.h"

#include <parser/FileInput.h>
#include <parser/Input.h>
#include <parser/ListInput.h>
#include <parser/MemoryInput.h>
#include <parser/Parser.h>
#include <parser/ParserError.h>
#include <parser/Scanner.h>

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>

PreprocessorParserBase::PreprocessorParserBase(PreprocessorContext & preprocCtx,
		DefineMap & defMap) : preprocessorContext(preprocCtx), defineMap(defMap), parser(NULL), showWarnings(false) {
	
	listInput = new ListInput();
}

PreprocessorParserBase::PreprocessorParserBase(PreprocessorContext & preprocCtx,
		DefineMap & defMap, ListInput *output) : preprocessorContext(preprocCtx),
		defineMap(defMap), listInput(output), parser(NULL), showWarnings(false) {}

PreprocessorParserBase::~PreprocessorParserBase() {
	delete(parser);
}

void PreprocessorParserBase::parse(Input *in) {
	const Preprocessor *preproc = preprocessorContext.getPreprocessor();
	
	preprocessorContext.setInput(in);
	
	Scanner *scan = new PreprocessorScanner(preproc->getScannerAutomata(), in);
	parser = new Parser(preproc->getParserTable(), scan);
	
	NonTerminal *root = (NonTerminal *)parser->parse();
	parsePreprocessor(root);
	
	delete(root);
	
	flushOutput();
}

Input *PreprocessorParserBase::getResult() const {
	return listInput;
}

void PreprocessorParserBase::setShowWarnings(bool show) {
	showWarnings = show;
}

/*
 * <PREPROCESSOR> ::= <CODE> <CONTROL_LINE> <PREPROCESSOR>
 *		| <CODE>
 *		;
 */
void PreprocessorParserBase::parsePreprocessor(NonTerminal *nonTerminal) {
	assert(nonTerminal->getNonTerminalId() == PREPROCESSORPARSERBUFFER_NONTERMINAL_PREPROCESSOR);
	
	if (nonTerminal->getNonTerminalRule() == 0) {
		writeCode(nonTerminal->getNonTerminalAt(0));
		parseControlLine(nonTerminal->getNonTerminalAt(1));
		parsePreprocessor(nonTerminal->getNonTerminalAt(2));
	}
	else {
		assert(nonTerminal->getNonTerminalRule() == 1);
		writeCode(nonTerminal->getNonTerminalAt(0));
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
void PreprocessorParserBase::parseControlLine(NonTerminal *ctrlLine) {
	assert(ctrlLine->getNonTerminalId() == PREPROCESSORPARSERBUFFER_NONTERMINAL_CONTROL_LINE);
	
	switch (ctrlLine->getNonTerminalRule()) {
		case 0: // <CONTROL_LINE> ::= <DEFINE_LINE>
			parseDefine(ctrlLine->getNonTerminalAt(0));
			break;
		case 1: // <CONTROL_LINE> ::= <UNDEF_LINE>
			parseUndef(ctrlLine->getNonTerminalAt(0));
			break;
		case 4: // <CONTROL_LINE> ::= <LINE_LINE>
			// it will be processed only in the last step
			writeNonTerminal(ctrlLine);
			break;
		default:
			abort();
	}
}

/*
 * <DEFINE_LINE> ::= DEFINE IDENTIFIER <CODE_WITHOUT_DIRECTEND> DIRECTIVE_END
 *		| DEFINE MACRO <IDENTIFIER_LIST> P_CLOSE <CODE_WITHOUT_DIRECTEND> DIRECTIVE_END
 *		| DEFINE MACRO P_CLOSE <CODE_WITHOUT_DIRECTEND> DIRECTIVE_END
 *		;
 */
void PreprocessorParserBase::parseDefine(NonTerminal *nonTerminal) {
	assert(nonTerminal->getNonTerminalId() == PREPROCESSORPARSERBUFFER_NONTERMINAL_DEFINE_LINE);
	
	switch (nonTerminal->getNonTerminalRule()) {
		case 0: // <DEFINE_LINE> ::= DEFINE IDENTIFIER <CODE_WITHOUT_DIRECTEND> DIRECTIVE_END
		{
			const std::string & name = nonTerminal->getTokenAt(1)->getToken();
			
			if (showWarnings && defineMap.isDefined(name)) {
				std::cerr << nonTerminal->getInputLocation() << ": warning: "
						<< name << " already defined." << std::endl;
			}
			defineMap.define(name, getCode(nonTerminal->getNonTerminalAt(2)));
			
			break;
		}
		case 1: // <DEFINE_LINE> ::= DEFINE MACRO <IDENTIFIER_LIST> P_CLOSE <CODE_WITHOUT_DIRECTEND> DIRECTIVE_END
		{
			const std::string & name = getMacroName(nonTerminal->getTokenAt(1));
			ArgumentIndex args;
			parseArgumentList(nonTerminal->getNonTerminalAt(2), args);
			defineMacro(name, args, nonTerminal->getNonTerminalAt(4));
			break;
		}
		case 2: // <DEFINE_LINE> ::= DEFINE MACRO P_CLOSE <CODE_WITHOUT_DIRECTEND> DIRECTIVE_END
		{
			const std::string & name = getMacroName(nonTerminal->getTokenAt(1));
			ArgumentIndex args;
			defineMacro(name, args, nonTerminal->getNonTerminalAt(3));
			break;
		}
		default:
			abort();
	}
}

/*
 * <UNDEF_LINE> ::= UNDEF IDENTIFIER DIRECTIVE_END;
 */
void PreprocessorParserBase::parseUndef(NonTerminal *nonTerminal) {
	assert(nonTerminal->getNonTerminalId() == PREPROCESSORPARSERBUFFER_NONTERMINAL_UNDEF_LINE);
	
	const std::string & name = nonTerminal->getTokenAt(1)->getToken();
	if (defineMap.isDefined(name)) defineMap.undef(name);
}

/*
 * <IDENTIFIER_LIST> ::= IDENTIFIER COMMA <IDENTIFIER_LIST>
 *		| IDENTIFIER
 *		;
 */
void PreprocessorParserBase::parseArgumentList(NonTerminal *nt, ArgumentIndex & args) const {
	assert(nt->getNonTerminalId() == PREPROCESSORPARSERBUFFER_NONTERMINAL_IDENTIFIER_LIST);
	
	unsigned int index = 0;
	
	while (nt) {
		const std::string & arg = nt->getTokenAt(0)->getToken();
		if (args.find(arg) != args.end()) {
			throw ParserError(nt->getInputLocation(), std::string("Duplicated parametter \"") + arg + "\".");
		}
		args[arg] = index++;
		
		if (nt->getNonTerminalRule() == 0) nt = nt->getNonTerminalAt(2);
		else {
			assert(nt->getNonTerminalRule() == 1);
			nt = NULL;
		}
	}
}

void PreprocessorParserBase::defineMacro(const std::string & name, const ArgumentIndex & args, NonTerminal *code) {
	assert(code->getNonTerminalId() == PREPROCESSORPARSERBUFFER_NONTERMINAL_CODE_WITHOUT_DIRECTEND);
	
	if (showWarnings && defineMap.isDefined(name)) {
		std::cerr << code->getInputLocation() << ": warning: "
				<< name << " already defined." << std::endl;
	}
	
	DefineMap::Macro *macro = new DefineMap::Macro(args.size());
	
	TokenList tokenList;
	getCodeTokens(code, tokenList);
	
	std::string constant;
	
	for (TokenList::const_iterator it = tokenList.begin(); it != tokenList.end(); ++it) {
		ArgumentIndex::const_iterator argsIt = args.find((*it)->getToken());
		if (argsIt != args.end()) {
			macro->addConstant(constant);
			constant = "";
			
			macro->addVariable(argsIt->second);
		}
		else {
			const std::string & tok = (*it)->getToken();
			for (std::string::const_iterator sIt = tok.begin(); sIt != tok.end(); ++sIt) {
				constant.push_back(*sIt);
			}
			constant.push_back(DELIMITER);
		}
	}
	
	if (!constant.empty()) macro->addConstant(constant);
	
	
	defineMap.defineMacro(name, macro);
}

void PreprocessorParserBase::writeCode(NonTerminal *code) {
	std::string c = getCode(code);
	
	for (std::string::const_iterator it = c.begin(); it != c.end(); ++it) {
		currentOutput.push_back(*it);
	}
}

std::string PreprocessorParserBase::getCode(NonTerminal *code) const {
	TokenList tokenList;
	getCodeTokens(code, tokenList);

	// dump the tokens
	std::string result;
	for (TokenList::iterator it = tokenList.begin(); it != tokenList.end(); ++it) {
		const std::string & tok = (*it)->getToken();
		for (std::string::const_iterator sIt = tok.begin(); sIt != tok.end(); ++sIt) {
			result.push_back(*sIt);
		}
		result.push_back(DELIMITER);
	}
	
	return result;
}

void PreprocessorParserBase::getCodeTokens(NonTerminal *code, TokenList & tokenList) const {
	assert(code->getNonTerminalId() == PREPROCESSORPARSERBUFFER_NONTERMINAL_CODE
			|| code->getNonTerminalId() == PREPROCESSORPARSERBUFFER_NONTERMINAL_CODE_WITHOUT_DIRECTEND);
	
	for (NonTerminal *nt = code; nt->getNonTerminalRule() != 1; nt = nt->getNonTerminalAt(0)) {
		ParsingTree::Node *node = nt->getNodeAt(1);

		while (node->getNodeType() == ParsingTree::NODE_NON_TERMINAL) {
			assert(((NonTerminal *)node)->getNodeList().size() == 1);
			node = ((NonTerminal *)node)->getNodeAt(0);
		}

		assert(node->getNodeType() == ParsingTree::NODE_TOKEN);
		tokenList.push_front((Token *)node);
	}
}

void PreprocessorParserBase::writeCodeNonRecursive(NonTerminal *code) {
	std::string c = getCodeNonRecursive(code);
	
	for (std::string::const_iterator it = c.begin(); it != c.end(); ++it) {
		currentOutput.push_back(*it);
	}
}

/*
 * <CODE> ::= <CODE> <CODE_TOKEN>
 *		| // empty 
 *		;
 *
 *<CODE_TOKEN> ::= <CODE_TOKEN_WITHOUT_DIRECTEND>
 *		| DIRECTIVE_END
 *		;
 */
std::string PreprocessorParserBase::getCodeNonRecursive(NonTerminal *code) const {
	assert(code->getNonTerminalId() == PREPROCESSORPARSERBUFFER_NONTERMINAL_CODE);
	
	Token *token = NULL;
	
	if (code->getNonTerminalRule() == 0) {
		Node *node = code->getNonTerminalAt(1);
		assert(((NonTerminal *)node)->getNonTerminalId() == PREPROCESSORPARSERBUFFER_NONTERMINAL_CODE_TOKEN);
		
		while (node->getNodeType() == ParsingTree::NODE_NON_TERMINAL) {
			assert(((NonTerminal *)node)->getNodeList().size() == 1);
			node = ((NonTerminal *)node)->getNodeAt(0);
		}

		assert(node->getNodeType() == ParsingTree::NODE_TOKEN);
		token = (Token *)node;
	}
	else {
		assert(code->getNonTerminalRule() == 1);
		return "";
	}
	
	assert(token);
	
	return token->getToken();
}

void PreprocessorParserBase::writeNonTerminal(NonTerminal *nonTerminal) {
	getNonTerminalDump(currentOutput, nonTerminal);
}

void PreprocessorParserBase::getNonTerminalDump(std::string & result, NonTerminal *nonTerminal) const {
	const NodeList & nodeList = nonTerminal->getNodeList();
	
	for (NodeList::const_iterator it = nodeList.begin(); it != nodeList.end(); ++it) {
		if ((*it)->getNodeType() == ParsingTree::NODE_NON_TERMINAL) {
			getNonTerminalDump(result, (NonTerminal *)*it);
		}
		else {
			assert((*it)->getNodeType() == ParsingTree::NODE_TOKEN);
			
			const std::string & tok = ((Token *)*it)->getToken();
			for (std::string::const_iterator tIt = tok.begin(); tIt != tok.end(); ++tIt) {
				result.push_back(*tIt);
			}
			result.push_back(' ');
		}
	}
}

void PreprocessorParserBase::flushOutput() {
	currentOutput.push_back('\n');
	Input *in = new MemoryInput(currentOutput, parser->getScanner()->getInput()->getInputName());
	listInput->addInput(in);
	
	char buf[32];
	sprintf(buf, "%d", parser->getScanner()->getInput()->getInputLine() - 1);
	currentOutput = std::string("\n#line ") + buf + " \"" + parser->getScanner()->getInput()->getInputName() + "\"\n";
}

std::string PreprocessorParserBase::getMacroName(Token *token) const {
	assert(token->getTokenTypeId() == PREPROCESSORPARSERBUFFER_TOKEN_MACRO);
	
	const std::string & macro = token->getToken();
	
	assert(macro.size() > 1);
	assert(macro[macro.size() - 1] == '(');
	
	char buf[macro.size() + 1];
	strcpy(buf, macro.c_str());
	buf[macro.size() - 1] = '\0';
	
	return std::string(buf);
}
