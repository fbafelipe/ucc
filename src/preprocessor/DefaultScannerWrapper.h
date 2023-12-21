#ifndef DEFAULT_SCANNER_WRAPPER_H
#define DEFAULT_SCANNER_WRAPPER_H

#include "preprocessor/ScannerWrapper.h"

#include <parser/Pointer.h>
#include <parser/Scanner.h>

class DefaultScannerWrapper : public ScannerWrapper {
	public:
		DefaultScannerWrapper(Scanner *scan);
		virtual ~DefaultScannerWrapper();
		
		ParsingTree::Token *scannerNextToken();
		
	private:
		Pointer<Scanner> scanner;
};

#endif
