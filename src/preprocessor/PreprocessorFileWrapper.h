#ifndef PREPROCESSOR_FILE_WRAPPER_H
#define PREPROCESSOR_FILE_WRAPPER_H

#include "preprocessor/PreprocessorParserBase.h"

#include <parser/ParserAction.h>

class InputWrapper;

// this class fix the input for the compiler
class PreprocessorFileWrapper : public PreprocessorParserBase, private ParserAction {
	public:
		PreprocessorFileWrapper(PreprocessorContext & preprocCtx,
				DefineMap & defMap);
		virtual ~PreprocessorFileWrapper();
		
		virtual void parse(Input *in);
		
	protected:
		virtual void parseControlLine(NonTerminal *ctrlLine);
		void parseLine(NonTerminal *nonTerminal);
		
		void flushOutput();
		
	private:
			virtual void recognized(NonTerminal *nt);
		
		std::string cleanToken(const std::string & token) const;
		std::string cleanToken(Token *token) const;
		
		InputWrapper *inputWrapper;
};

#endif
