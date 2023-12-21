#ifndef PREPROCESSOR_EXP_SCANNER_H
#define PREPROCESSOR_EXP_SCANNER_H

#include <parser/ParsingTree.h>
#include <parser/Scanner.h>

#include <queue>

class DefineMap;

class PreprocessorExpScanner : public Scanner {
	public:
		typedef ParsingTree::Token Token;
		
		PreprocessorExpScanner(const Pointer<ScannerAutomata> & a, Input *in, const DefineMap & defMap);
		virtual ~PreprocessorExpScanner();
		
		// return null if the end has been reached
		virtual Token *nextToken();
		
	private:
		typedef std::queue<Token *> TokenQueue;
		
		Token *readToken();
		Token *checkExpand(Token *token);
		
		void expandDefine(Token *token);
		void expandMacro(Token *token);
		
		void expand(const std::string & def);
		
		const DefineMap & defineMap;
		
		TokenQueue tokenQueue;
};

#endif
