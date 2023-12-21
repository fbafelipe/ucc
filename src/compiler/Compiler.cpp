#include "compiler/Compiler.h"

#include "compiler/CompilerContext.h"
#include "compiler/CParser.h"
#include "compiler/CScanner.h"
#include "CParserBuffer.h"

#include <parser/Parser.h>
#include <parser/ParserLoader.h>
#include <parser/Scanner.h>

Compiler::Compiler() {
	scannerAutomata = ParserLoader::bufferToAutomata(c_parser_buffer_scanner);
	parserTable = ParserLoader::bufferToTable(c_parser_buffer_parser);
}

Compiler::~Compiler() {}

Program *Compiler::compile(Input *input) const {
	CompilerContext context(this);
	
	CParser parser(context);
	
	return parser.parse(input);
}

void Compiler::checkSyntax(Input *input) const {
	Scanner *scan = new CScanner(scannerAutomata, input);
	Parser *parser = new Parser(parserTable, scan);
	
	delete(parser->parse());
	delete(parser);
}

const Pointer<ScannerAutomata> & Compiler::getScannerAutomata() const {
	return scannerAutomata;
}

const Pointer<ParserTable> & Compiler::getParserTable() const {
	return parserTable;
}

const StaticMemoryList & Compiler::getStaticMemoryList() const {
	return staticMemoryList;
}
