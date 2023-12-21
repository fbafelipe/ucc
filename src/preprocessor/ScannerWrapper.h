#ifndef SCANNER_WRAPPER_H
#define SCANNER_WRAPPER_H

#include <parser/ParsingTree.h>

class ScannerWrapper {
	public:
		ScannerWrapper();
		virtual ~ScannerWrapper();
		
		virtual ParsingTree::Token *scannerNextToken() = 0;
};

#endif
