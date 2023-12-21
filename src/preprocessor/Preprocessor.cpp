#include "preprocessor/Preprocessor.h"

#include "preprocessor/DefineMap.h"
#include "preprocessor/PreprocessorContext.h"
#include "preprocessor/PreprocessorFileWrapper.h"
#include "preprocessor/PreprocessorMacroParser.h"
#include "preprocessor/PreprocessorMacroScanner.h"
#include "preprocessor/PreprocessorParser.h"
#include "PreprocessorParserBuffer.h"
#include "PreprocExpParserBuffer.h"

#include <parser/Input.h>
#include <parser/ListInput.h>
#include <parser/ParserLoader.h>

#include <iostream>

Preprocessor::Preprocessor(const FileList & inclDirs) : includeDirs(inclDirs) {
	scannerAutomata = ParserLoader::bufferToAutomata(preprocessor_parser_buffer_scanner);
	parserTable = ParserLoader::bufferToTable(preprocessor_parser_buffer_parser);
	
	expScannerAutomata = ParserLoader::bufferToAutomata(preproc_exp_parser_buffer_scanner);
	expParserTable = ParserLoader::bufferToTable(preproc_exp_parser_buffer_parser);
}

Preprocessor::~Preprocessor() {}

Input *Preprocessor::preprocess(Input *input) const {
	std::string baseName = input->getInputName();
	
	input = preprocessStep1(input);
	input = preprocessStep2(input, baseName);
	input = preprocessStep3(input, baseName);
	
	return input;
}

Input *Preprocessor::preprocessStep1(Input *input) const {
	PreprocessorContext context(this);
	context.setBaseFile(input->getInputName());
	
	DefineMap defineMap(context);
	PreprocessorParser parser(context, defineMap);
	parser.parse(input);
	
	return parser.getResult();
}

Input *Preprocessor::preprocessStep2(Input *input, const std::string & baseName) const {
	PreprocessorContext context(this);
	context.setBaseFile(baseName);
	
	DefineMap defineMap(context);
	PreprocessorMacroParser parser(context, defineMap);
	parser.parse(input);
	
	return parser.getResult();
}

Input *Preprocessor::preprocessStep3(Input *input, const std::string & baseName) const {
	PreprocessorContext context(this);
	context.setBaseFile(baseName);
	
	DefineMap defineMap(context);
	PreprocessorFileWrapper parser(context, defineMap);
	parser.parse(input);
	
	return parser.getResult();
}

const Pointer<ScannerAutomata> & Preprocessor::getScannerAutomata() const {
	return scannerAutomata;
}

const Pointer<ParserTable> & Preprocessor::getParserTable() const {
	return parserTable;
}

const Pointer<ScannerAutomata> & Preprocessor::getExpScannerAutomata() const {
	return expScannerAutomata;
}

const Pointer<ParserTable> & Preprocessor::getExpParserTable() const {
	return expParserTable;
}

const Preprocessor::FileList & Preprocessor::getIncludeDirs() const {
	return includeDirs;
}
