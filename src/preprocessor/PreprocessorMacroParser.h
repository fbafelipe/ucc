#ifndef PREPROCESSOR_MACRO_PARSER_H
#define PREPROCESSOR_MACRO_PARSER_H

#include "preprocessor/PreprocessorParserBase.h"

#include <parser/ParserAction.h>

class DefineMap;
class Input;
class ListInput;
class PreprocessorContext;

class PreprocessorMacroParser : public PreprocessorParserBase, private ParserAction {
	public:
		PreprocessorMacroParser(PreprocessorContext & preprocCtx,
				DefineMap & defMap);
		PreprocessorMacroParser(PreprocessorContext & preprocCtx,
				DefineMap & defMap, ListInput *output);
		virtual ~PreprocessorMacroParser();
		
		virtual void parse(Input *in);
		
	private:
		virtual void recognized(NonTerminal *nt);
};

#endif
