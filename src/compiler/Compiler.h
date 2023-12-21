#ifndef COMPILER_H
#define COMPILER_H

#include "compiler/StaticMemory.h"

#include <parser/ParserTable.h>
#include <parser/Pointer.h>
#include <parser/ScannerAutomata.h>

class Input;
class Program;

class Compiler {
	public:
		Compiler();
		~Compiler();
		
		Program *compile(Input *input) const;
		
		void checkSyntax(Input *input) const;
		
		const Pointer<ScannerAutomata> & getScannerAutomata() const;
		const Pointer<ParserTable> & getParserTable() const;
		
		const StaticMemoryList & getStaticMemoryList() const;
		
	private:
		Pointer<ScannerAutomata> scannerAutomata;
		Pointer<ParserTable> parserTable;
		
		StaticMemoryList staticMemoryList;
};

#endif
