#ifndef CSCANNER_H
#define CSCANNER_H

#include "compiler/TypedefManager.h"

#include <parser/Scanner.h>

class CScanner : public Scanner {
	public:
		typedef ParsingTree::Token Token;
		
		CScanner(const Pointer<ScannerAutomata> & a, Input *in);
		virtual ~CScanner();
		
		virtual Token *nextToken();
		
		TypedefManager & getTypedefManager();
		
	private:
		TypedefManager typedefManager;
};

#endif
