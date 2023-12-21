#ifndef PREPROCESSOR_MACRO_SCANNER_H
#define PREPROCESSOR_MACRO_SCANNER_H

#include "preprocessor/PreprocessorScanner.h"
#include "preprocessor/ScannerWrapper.h"

#include <parser/Pointer.h>
#include <parser/Regex.h>
#include <parser/ScannerAutomata.h>

#include <set>
#include <string>

class DefineMap;
class InputWrapper;

class PreprocessorMacroScanner : public PreprocessorScanner, private ScannerWrapper {
	public:
		typedef std::set<std::string> IdentifierSet;
		
		PreprocessorMacroScanner(const Pointer<ScannerAutomata> & a, Input *in,
				const DefineMap & defMap);
		virtual ~PreprocessorMacroScanner();
		
	protected:
		virtual Token *readTextToken(Token *startToken);
		
		void append(const Pointer<ScannerWrapper> & wrapper, std::string & result,
				Token *token, IdentifierSet & expanded);
		
		void expand(const Pointer<ScannerWrapper> & wrapper, std::string & result,
				Token *token, IdentifierSet & expanded);
		
		void expandDefine(const Pointer<ScannerWrapper> & wrapper, std::string & result,
				const std::string & defineName, IdentifierSet & expanded);
		void expandMacro(const Pointer<ScannerWrapper> & wrapper, std::string & result,
				const std::string & macroName, IdentifierSet & expanded);
		
		void readMacroArgument(const Pointer<ScannerWrapper> & wrapper, std::string & result,
				IdentifierSet & expanded);
		
		void defExpanded(const Pointer<ScannerWrapper> & wrapper,
				std::string & result, const std::string & expansion, IdentifierSet & expanded);
		
		void flushOutput();
		
	private:
		virtual Token *scannerNextToken();
		
		bool isDefined(Token *token, IdentifierSet & expanded);
		std::string getMacroName(Token *token) const;
		
		bool isIdentifierToken(Token *token) const;
		bool isMacroToken(Token *token) const;
		
		const DefineMap & defineMap;
		
		static Regex identifierRegex;
		static Regex macroRegex;
		
		static Pointer<ScannerAutomata> preprocessorMacroIdScannerAutomata;
};

#endif
