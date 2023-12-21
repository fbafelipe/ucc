#include "preprocessor/DefaultScannerWrapper.h"

#include <cassert>

DefaultScannerWrapper::DefaultScannerWrapper(Scanner *scan) : scanner(scan) {
	assert(scanner);
}

DefaultScannerWrapper::~DefaultScannerWrapper() {}

ParsingTree::Token *DefaultScannerWrapper::scannerNextToken() {
	return scanner->nextToken();
}
