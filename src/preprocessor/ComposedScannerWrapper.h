#ifndef COMPOSED_SCANNER_WRAPPER_H
#define COMPOSED_SCANNER_WRAPPER_H

#include "preprocessor/ScannerWrapper.h"

#include <parser/Pointer.h>

class ComposedScannerWrapper : public ScannerWrapper {
	public:
		ComposedScannerWrapper(const Pointer<ScannerWrapper> & w1, const Pointer<ScannerWrapper> & w2);
		virtual ~ComposedScannerWrapper();
		
		ParsingTree::Token *scannerNextToken();
		
	private:
		Pointer<ScannerWrapper> wrapper1;
		Pointer<ScannerWrapper> wrapper2;
};

#endif
