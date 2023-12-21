#ifndef PREPROCESSOR_PARSER_H
#define PREPROCESSOR_PARSER_H

#include "preprocessor/PreprocessorParserBase.h"

#include <parser/ParsingTree.h>

#include <map>
#include <string>
#include <vector>

class DefineMap;
class Input;
class ListInput;
class Parser;
class PreprocessorContext;
class PreprocessorExpParser;

class PreprocessorParser : public PreprocessorParserBase {
	public:
		typedef std::vector<std::string> FileList;
		
		PreprocessorParser(PreprocessorContext & preprocCtx, DefineMap & defMap);
		PreprocessorParser(PreprocessorContext & preprocCtx, DefineMap & defMap, ListInput *output);
		virtual ~PreprocessorParser();
		
		virtual void parse(Input *in);
		
	protected:
		virtual void parseControlLine(NonTerminal *ctrlLine);
		
		virtual void parseDefine(NonTerminal *nonTerminal);
		virtual void parseUndef(NonTerminal *nonTerminal);
		
		void parseInclude(NonTerminal *nonTerminal);
		void parseLine(NonTerminal *nonTerminal);
		
		void parseConditional(NonTerminal *nonTerminal);
		void parseElifParts(NonTerminal *nonTerminal);
		void parseElsePart(NonTerminal *nonTerminal);
		
		bool evaluateIfLine(NonTerminal *nonTerminal);
		bool evaluateElifLine(NonTerminal *nonTerminal);
		
		void writeCode(NonTerminal *code);
		std::string getCode(NonTerminal *code) const;
		
		bool getConditionalResult() const;
		
		bool evaluateExpression(NonTerminal *nonTerminal) const;
		
		// convert a non terminal to a string
		std::string getNtString(const NonTerminal *nonTerminal) const;
		
		// remove the first and the last character of the token
		std::string cleanToken(const std::string & token) const;
		std::string cleanToken(Token *token) const;
		
		PreprocessorExpParser *expParser;
		const FileList & includeDirs;
};

#endif
