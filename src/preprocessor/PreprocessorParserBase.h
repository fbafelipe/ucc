#ifndef PREPROCESSOR_PARSER_BASE_H
#define PREPROCESSOR_PARSER_BASE_H

#include <parser/ParsingTree.h>

#include <map>
#include <list>
#include <string>

class DefineMap;
class Input;
class ListInput;
class Parser;
class Preprocessor;
class PreprocessorContext;

class PreprocessorParserBase {
	public:
		typedef ParsingTree::Node Node;
		typedef ParsingTree::NodeList NodeList;
		typedef ParsingTree::NonTerminal NonTerminal;
		typedef ParsingTree::Token Token;
		typedef std::list<Token *> TokenList;
		typedef std::map<std::string, unsigned int> ArgumentIndex;
		
		PreprocessorParserBase(PreprocessorContext & preprocCtx,
				DefineMap & defMap);
		PreprocessorParserBase(PreprocessorContext & preprocCtx,
				DefineMap & defMap, ListInput *output);
		virtual ~PreprocessorParserBase();
		
		virtual void parse(Input *in) = 0;
		
		Input *getResult() const;
		
		void setShowWarnings(bool show);
		
	protected:
		virtual void parsePreprocessor(NonTerminal *nonTerminal);
		virtual void parseControlLine(NonTerminal *ctrlLine);
		
		virtual void parseDefine(NonTerminal *nonTerminal);
		virtual void parseUndef(NonTerminal *nonTerminal);
		
		void parseArgumentList(NonTerminal *nt, ArgumentIndex & args) const;
		void defineMacro(const std::string & name, const ArgumentIndex & args, NonTerminal *code);
		
		virtual void writeCode(NonTerminal *code);
		virtual std::string getCode(NonTerminal *code) const;
		void getCodeTokens(NonTerminal *code, TokenList & tokenList) const;
		
		void writeCodeNonRecursive(NonTerminal *code);
		std::string getCodeNonRecursive(NonTerminal *code) const;
		
		void writeNonTerminal(NonTerminal *nonTerminal);
		void getNonTerminalDump(std::string & result, NonTerminal *nonTerminal) const;
		
		virtual void flushOutput();
		
		std::string getMacroName(Token *token) const;
		
		PreprocessorContext & preprocessorContext;
		DefineMap & defineMap;
		
		ListInput *listInput;
		
		Parser *parser;
		
		std::string currentOutput;
		
		bool showWarnings;
};

#endif
