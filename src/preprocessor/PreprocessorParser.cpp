#include "preprocessor/PreprocessorParser.h"

#include "preprocessor/DefineMap.h"
#include "preprocessor/Preprocessor.h"
#include "preprocessor/PreprocessorContext.h"
#include "preprocessor/PreprocessorExpParser.h"
#include "preprocessor/PreprocessorMacroScanner.h"
#include "PreprocessorParserBuffer.h"
#include "UccDefs.h"
#include "UccUtils.h"

#include <parser/FileInput.h>
#include <parser/Input.h>
#include <parser/IOError.h>
#include <parser/ListInput.h>
#include <parser/MemoryInput.h>
#include <parser/OffsetInput.h>
#include <parser/Parser.h>
#include <parser/ParserError.h>
#include <parser/Scanner.h>

#include <cstdlib>
#include <cstring>
#include <iostream>

PreprocessorParser::PreprocessorParser(PreprocessorContext & preprocCtx,
		DefineMap & defMap) : PreprocessorParserBase(preprocCtx, defMap),
		includeDirs(preprocCtx.getPreprocessor()->getIncludeDirs()) {
	
	const Preprocessor *preproc = preprocessorContext.getPreprocessor();
	expParser = new PreprocessorExpParser(preproc, defineMap);
	
	setShowWarnings(true);
}

PreprocessorParser::PreprocessorParser(PreprocessorContext & preprocCtx,
		DefineMap & defMap, ListInput *output) :
		PreprocessorParserBase(preprocCtx, defMap, output),
		includeDirs(preprocCtx.getPreprocessor()->getIncludeDirs()) {
	
	const Preprocessor *preproc = preprocessorContext.getPreprocessor();
	expParser = new PreprocessorExpParser(preproc, defineMap);
}

PreprocessorParser::~PreprocessorParser() {
	delete(expParser);
}

void PreprocessorParser::parse(Input *in) {
	const Preprocessor *preproc = preprocessorContext.getPreprocessor();
	
	preprocessorContext.setInput(in);
	
	Scanner *scan = new PreprocessorMacroScanner(preproc->getScannerAutomata(), in, defineMap);
	parser = new Parser(preproc->getParserTable(), scan);
	
	NonTerminal *root = (NonTerminal *)parser->parse();
	parsePreprocessor(root);
	
	delete(root);
	
	flushOutput();
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
void PreprocessorParser::parseControlLine(NonTerminal *ctrlLine) {
	assert(ctrlLine->getNonTerminalId() == PREPROCESSORPARSERBUFFER_NONTERMINAL_CONTROL_LINE);
	
	switch (ctrlLine->getNonTerminalRule()) {
		case 0: // <CONTROL_LINE> ::= <DEFINE_LINE>
			parseDefine(ctrlLine->getNonTerminalAt(0));
			break;
		case 1: // <CONTROL_LINE> ::= <UNDEF_LINE>
			parseUndef(ctrlLine->getNonTerminalAt(0));
			break;
		case 2: // <CONTROL_LINE> ::= <INCLUDE_LINE>
			parseInclude(ctrlLine->getNonTerminalAt(0));
			break;
		case 3: // <CONTROL_LINE> ::= <CONDITIONAL>
			parseConditional(ctrlLine->getNonTerminalAt(0));
			break;
		case 4: // <CONTROL_LINE> ::= <LINE_LINE>
			parseLine(ctrlLine->getNonTerminalAt(0));
			break;
		case 5: // <CONTROL_LINE> ::= <ERROR_LINE>
			throw ParserError(ctrlLine->getInputLocation(), getCode(ctrlLine->getNonTerminalAt(1)));
			break;
		case 6: // <CONTROL_LINE> ::= <WARNING_LINE>
			// always show, even if showWarnings is false
			std::cerr << ctrlLine->getInputLocation() << " warning: "
					<< cleanToken(ctrlLine->getTokenAt(1)) << std::endl;
			break;
		case 7: // <CONTROL_LINE> ::= <PRAGMA_LINE>
			if (showWarnings) {
				std::cerr << "warning: ignoring #pragma at " << ctrlLine->getInputLocation() << std::endl;
			}
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
void PreprocessorParser::parseDefine(NonTerminal *nonTerminal) {
	PreprocessorParserBase::parseDefine(nonTerminal);
	
	// it will be parsed in the second step of the preprocessor
	writeNonTerminal(nonTerminal);
}

/*
 * <UNDEF_LINE> ::= UNDEF IDENTIFIER DIRECTIVE_END;
 */
void PreprocessorParser::parseUndef(NonTerminal *nonTerminal) {
	PreprocessorParserBase::parseUndef(nonTerminal);
	
	// it will be parsed again in the second step of the preprocessor
	writeNonTerminal(nonTerminal);
}

/*
 * <INCLUDE_LINE> ::= INCLUDE FILENAME_SYSTEM DIRECTIVE_END
 *		| INCLUDE STRING_LITERAL DIRECTIVE_END
 *		| INCLUDE IDENTIFIER DIRECTIVE_END
 *		;
 */
void PreprocessorParser::parseInclude(NonTerminal *nonTerminal) {
	assert(nonTerminal->getNonTerminalId() == PREPROCESSORPARSERBUFFER_NONTERMINAL_INCLUDE_LINE);
	
	// if true, it will be an error if the file is not found
	bool errorOnNF = false;
	std::string fileName;
	
	switch (nonTerminal->getNonTerminalRule()) {
		case 0: // <INCLUDE_LINE> ::= INCLUDE FILENAME_SYSTEM DIRECTIVE_END
			fileName = cleanToken(nonTerminal->getTokenAt(1));
			break;
		case 1: // <INCLUDE_LINE> ::= INCLUDE STRING_LITERAL DIRECTIVE_END
			errorOnNF = true;
			fileName = cleanToken(nonTerminal->getTokenAt(1));
			break;
		case 2: // <INCLUDE_LINE> ::= INCLUDE IDENTIFIER DIRECTIVE_END
		{
			if (!defineMap.defineDefined(nonTerminal->getTokenAt(1)->getToken())) {
				throw ParserError(nonTerminal->getInputLocation(), "Missing filename");
			}
			std::string def = defineMap.getDefine(nonTerminal->getTokenAt(1)->getToken());
			
			if (def.size() < 2) throw ParserError(nonTerminal->getInputLocation(), "Bad include define.");
			
			char f = def[0];
			char l = def[def.size() - 1];
			if ((f == '\"' &&  l == '\"') || (f == '<' &&  l == '>')) {
				fileName = cleanToken(def);
			}
			else throw ParserError(nonTerminal->getInputLocation(), "Bad include define.");
			
			break;
		}
		default:
			abort();
	}
	
	Input *fileInput = NULL;
	
	for (FileList::const_iterator it = includeDirs.begin(); it != includeDirs.end(); ++it) {
		try {
			fileInput = new FileInput(*it + fileName);
			break;
		}
		catch (IOError & error) {}
	}
	
	if (fileInput) {
		currentOutput += std::string("\n#line ") + "1 \"" + fileInput->getInputName() + "\"";
		
		flushOutput();
		
		preprocessorContext.incIncludeLevel();
		PreprocessorParser p(preprocessorContext, defineMap, listInput);
		p.setShowWarnings(showWarnings && errorOnNF);
		p.parse(fileInput);
		preprocessorContext.setInput(parser->getScanner()->getInput());
		preprocessorContext.decIncludeLevel();
	}
	else {
		std::string error = std::string("Include file not found: ") + fileName;
		if (errorOnNF && showWarnings) throw ParserError(nonTerminal->getInputLocation(), error);
		else if (showWarnings) std::cerr << nonTerminal->getInputLocation() << ": warning: " << error << std::endl;
	}
}

/*
 * <LINE_LINE> ::= LINE CONSTANT STRING_LITERAL DIRECTIVE_END
 *		| LINE CONSTANT DIRECTIVE_END
 *		;
 */
void PreprocessorParser::parseLine(NonTerminal *nonTerminal) {
	assert(nonTerminal->getNonTerminalId() == PREPROCESSORPARSERBUFFER_NONTERMINAL_LINE_LINE);
	
	// it will be parsed in the last step of the preprocessor
	writeNonTerminal(nonTerminal);
}

/*
 * <CONDITIONAL> ::= <IF_LINE> <PREPROCESSOR> <ELIF_PARTS>;
 */
void PreprocessorParser::parseConditional(NonTerminal *nonTerminal) {
	assert(nonTerminal->getNonTerminalId() == PREPROCESSORPARSERBUFFER_NONTERMINAL_CONDITIONAL);
	assert(nonTerminal->getNonTerminalRule() == 0);
	
	if (evaluateIfLine(nonTerminal->getNonTerminalAt(0))) {
		parsePreprocessor(nonTerminal->getNonTerminalAt(1));
	}
	else parseElifParts(nonTerminal->getNonTerminalAt(2));
}

/*
 * <ELIF_PARTS> ::= <ELIF_LINE> <PREPROCESSOR> <ELIF_PARTS>
 *		| <ELSE_PART>
 *		;
 */
void PreprocessorParser::parseElifParts(NonTerminal *nonTerminal) {
	assert(nonTerminal->getNonTerminalId() == PREPROCESSORPARSERBUFFER_NONTERMINAL_ELIF_PARTS);
	
	if (nonTerminal->getNonTerminalRule() == 0) {
		if (evaluateElifLine(nonTerminal->getNonTerminalAt(0))) {
			parsePreprocessor(nonTerminal->getNonTerminalAt(1));
		}
		else parseElifParts(nonTerminal->getNonTerminalAt(2));
	}
	else {
		assert(nonTerminal->getNonTerminalRule() == 1);
		parseElsePart(nonTerminal->getNonTerminalAt(0));
	}
}
/*
 * <ELSE_PART> ::= <ELSE_LINE> <PREPROCESSOR> <ENDIF_LINE>
 *		| <ENDIF_LINE>
 *		;
 */
void PreprocessorParser::parseElsePart(NonTerminal *nonTerminal) {
	assert(nonTerminal->getNonTerminalId() == PREPROCESSORPARSERBUFFER_NONTERMINAL_ELSE_PART);
	
	if (nonTerminal->getNonTerminalRule() == 0) {
		parsePreprocessor(nonTerminal->getNonTerminalAt(1));
	}
	else {
		assert(nonTerminal->getNonTerminalRule() == 1);
	}
}

/*
 * <IF_LINE> ::= IF <CODE_WITHOUT_DIRECTEND> DIRECTIVE_END
 *		| IFDEF IDENTIFIER DIRECTIVE_END
 *		| IFNDEF IDENTIFIER DIRECTIVE_END
 *		;
 */
bool PreprocessorParser::evaluateIfLine(NonTerminal *nonTerminal) {
	assert(nonTerminal->getNonTerminalId() == PREPROCESSORPARSERBUFFER_NONTERMINAL_IF_LINE);
	
	bool result = false;
	
	switch (nonTerminal->getNonTerminalRule()) {
		case 0: // <IF_LINE> ::= IF <CODE_WITHOUT_DIRECTEND> DIRECTIVE_END
			result = evaluateExpression(nonTerminal->getNonTerminalAt(1));
			break;
		case 1: // <IF_LINE> ::= IFDEF IDENTIFIER DIRECTIVE_END
			result = defineMap.isDefined(nonTerminal->getTokenAt(1)->getToken());
			break;
		case 2: // <IF_LINE> ::= IFNDEF IDENTIFIER DIRECTIVE_END
			result = !defineMap.isDefined(nonTerminal->getTokenAt(1)->getToken());
			break;
		default:
			abort();
	}
	
	return result;
}

/*
 * <ELIF_LINE> ::= ELIF <CODE_WITHOUT_DIRECTEND> DIRECTIVE_END;
 */
bool PreprocessorParser::evaluateElifLine(NonTerminal *nonTerminal) {
	assert(nonTerminal->getNonTerminalId() == PREPROCESSORPARSERBUFFER_NONTERMINAL_ELIF_LINE);
	assert(nonTerminal->getNonTerminalRule() == 0);
	
	return evaluateExpression(nonTerminal->getNonTerminalAt(1));
}

void PreprocessorParser::writeCode(NonTerminal *code) {
	std::string c = getCode(code);
	
	for (std::string::const_iterator it = c.begin(); it != c.end(); ++it) {
		currentOutput.push_back(*it);
	}
}

std::string PreprocessorParser::getCode(NonTerminal *code) const {
	assert(code->getNonTerminalId() == PREPROCESSORPARSERBUFFER_NONTERMINAL_CODE
			|| code->getNonTerminalId() == PREPROCESSORPARSERBUFFER_NONTERMINAL_CODE_WITHOUT_DIRECTEND);
	
	// the tokens must be added in the reverse order
	// so let's use a temporary list
	std::vector<Token *> tokenList;
	for (NonTerminal *nt = code; nt->getNonTerminalRule() != 1; nt = nt->getNonTerminalAt(0)) {
		ParsingTree::Node *node = nt->getNodeAt(1);

		while (node->getNodeType() == ParsingTree::NODE_NON_TERMINAL) {
			assert(((NonTerminal *)node)->getNodeList().size() == 1);
			node = ((NonTerminal *)node)->getNodeAt(0);
		}

		assert(node->getNodeType() == ParsingTree::NODE_TOKEN);
		tokenList.push_back((Token *)node);
	}

	// dump the tokens in de inverse order
	std::string result;
	for (std::vector<Token *>::reverse_iterator it = tokenList.rbegin(); it != tokenList.rend(); ++it) {
		result += (*it)->getToken() + DELIMITER;
	}
	
	return result;
}

bool PreprocessorParser::evaluateExpression(NonTerminal *nonTerminal) const {
	assert(nonTerminal->getNonTerminalId() == PREPROCESSORPARSERBUFFER_NONTERMINAL_CODE_WITHOUT_DIRECTEND);
	
	// expand the nonTerminal to a string
	std::string exp = getNtString(nonTerminal);
	
	InputLocation loc = nonTerminal->getInputLocation();
	Input *in = new MemoryInput(exp, loc.getName());
	in = new OffsetInput(in, loc.getLine());
	
	return expParser->parseExp(in);
}

std::string PreprocessorParser::getNtString(const NonTerminal *nonTerminal) const {
	std::string result;
	
	const NodeList & nodeList = nonTerminal->getNodeList();
	
	for (NodeList::const_iterator it = nodeList.begin(); it != nodeList.end(); ++it) {
		if ((*it)->getNodeType() == ParsingTree::NODE_NON_TERMINAL) {
			result += getNtString((const NonTerminal *)*it);
		}
		else {
			assert((*it)->getNodeType() == ParsingTree::NODE_TOKEN);
			
			// insert a space to avoid messing two tokens
			result += ((const Token *)*it)->getToken() + " ";
		}
	}
	
	return result;
}

std::string PreprocessorParser::cleanToken(const std::string & token) const {
	assert(token.size() >= 2);
	
	char buf[token.size()];
	strcpy(buf, token.c_str() + 1); // remove the first character
	buf[strlen(buf) - 1] = '\0'; // remove the last character
	return std::string(buf);
}

std::string PreprocessorParser::cleanToken(Token *token) const {
	return cleanToken(token->getToken());
}
