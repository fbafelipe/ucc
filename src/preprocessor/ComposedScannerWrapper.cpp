#include "preprocessor/ComposedScannerWrapper.h"

#include <cassert>

ComposedScannerWrapper::ComposedScannerWrapper(const Pointer<ScannerWrapper> & w1,
		const Pointer<ScannerWrapper> & w2) : wrapper1(w1), wrapper2(w2) {
	
	assert(wrapper1);
	assert(wrapper2);
}

ComposedScannerWrapper::~ComposedScannerWrapper() {}

ParsingTree::Token *ComposedScannerWrapper::scannerNextToken() {
	ParsingTree::Token *token = wrapper1->scannerNextToken();
	if (!token) token = wrapper2->scannerNextToken();
	return token;
}
