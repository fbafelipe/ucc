#include "preprocessor/PreprocessorMacroParser.h"

#include "preprocessor/Preprocessor.h"
#include "preprocessor/PreprocessorContext.h"
#include "preprocessor/PreprocessorMacroScanner.h"
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

PreprocessorMacroParser::PreprocessorMacroParser(PreprocessorContext & preprocCtx,
		DefineMap & defMap) : PreprocessorParserBase(preprocCtx, defMap) {}

PreprocessorMacroParser::PreprocessorMacroParser(PreprocessorContext & preprocCtx,
		DefineMap & defMap, ListInput *output) :
		PreprocessorParserBase(preprocCtx, defMap, output) {}

PreprocessorMacroParser::~PreprocessorMacroParser() {}

void PreprocessorMacroParser::parse(Input *in) {
	const Preprocessor *preproc = preprocessorContext.getPreprocessor();
	
	preprocessorContext.setInput(in);
	
	Scanner *scan = new PreprocessorMacroScanner(preproc->getScannerAutomata(), in, defineMap);
	parser = new Parser(preproc->getParserTable(), scan);
	parser->setParserAction(this);
	
	NonTerminal *root = (NonTerminal *)parser->parse();
	delete(root);
	
	flushOutput();
}

void PreprocessorMacroParser::recognized(NonTerminal *nt) {
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
