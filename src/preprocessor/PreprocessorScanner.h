#ifndef PREPROCESSOR_SCANNER_H
#define PREPROCESSOR_SCANNER_H

#include <parser/InputLocation.h>
#include <parser/Pointer.h>
#include <parser/Scanner.h>
#include <parser/ScannerAutomata.h>

class Input;

class PreprocessorScanner : public Scanner {
	public:
		typedef ParsingTree::Token Token;
		
		PreprocessorScanner(const Pointer<ScannerAutomata> & a, Input *in);
		virtual ~PreprocessorScanner();
		
		// return null if the end has been reached
		virtual Token *nextToken();
		
	protected:
		enum State {
			LINE_BEGIN,
			INCLUDE_LINE,
			OTHER
		};
		
		Token *readTokenLineBegin();
		Token *readTokenIncludeLine();
		Token *readTokenOther();
		
		/*
		 * Read the content in the system include (between '<' and '>')
		 * When calling this method the '<' must already be readed,
		 * After this method returns the '>' will already be readed.
		 * 
		 * loc is the location of the returned token.
		 */
		Token *readSysInclude(InputLocation loc);
		
		virtual Token *readTextToken(Token *startToken);
		void appendToken(std::string & result, Token *token);
		
		State state;
		
		// some times we need to look a token forward
		// if we do not use it, we need to save it
		Token *cachedToken;
};

#endif
