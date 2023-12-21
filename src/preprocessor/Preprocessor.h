#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#include <parser/ParserTable.h>
#include <parser/Pointer.h>
#include <parser/ScannerAutomata.h>

#include <map>
#include <string>
#include <vector>

class DefineMap;
class Input;

class Preprocessor {
	public:
		typedef std::vector<std::string> FileList;
		
		Preprocessor(const FileList & inclDirs);
		~Preprocessor();
		
		Input *preprocess(Input *input) const;
		
		const Pointer<ScannerAutomata> & getScannerAutomata() const;
		const Pointer<ParserTable> & getParserTable() const;
		
		const Pointer<ScannerAutomata> & getExpScannerAutomata() const;
		const Pointer<ParserTable> & getExpParserTable() const;
		
		const FileList & getIncludeDirs() const;
		
	private:
		Input *preprocessStep1(Input *input) const;
		Input *preprocessStep2(Input *input, const std::string & baseName) const;
		Input *preprocessStep3(Input *input, const std::string & baseName) const;
		
		Pointer<ScannerAutomata> scannerAutomata;
		Pointer<ParserTable> parserTable;
		
		Pointer<ScannerAutomata> expScannerAutomata;
		Pointer<ParserTable> expParserTable;
		
		const FileList & includeDirs;
};

#endif
