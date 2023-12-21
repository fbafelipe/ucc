#include "ArgumentOptions.h"

#include "compiler/Compiler.h"
#include "linker/Assembler.h"
#include "linker/Linker.h"
#include "preprocessor/Preprocessor.h"
#include "vm/Program.h"
#include "vm/VirtualMachine.h"
#include "UccUtils.h"

#include <parser/Input.h>
#include <parser/FileInput.h>
#include <parser/ParserError.h>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <vector>

typedef std::vector<Input *> InputList;
typedef std::vector<Program *> ProgramList;

static InputList getInputsToCompile(const ArgumentOptions & options);
static InputList getInputsToLink(const ArgumentOptions & options);
static Input *getInputToRun(const ArgumentOptions & options);

static InputList runPreprocessor(const ArgumentOptions & options, const InputList & inputList);
static ProgramList runCompiler(const ArgumentOptions & options, const InputList & inputList);
static void runAssembler(ProgramList & programs, const ArgumentOptions & options,
		const InputList & inputList);
static Program *runLinker(const ArgumentOptions & options, ProgramList & programList);
static void runProgram(const ArgumentOptions & options, Program *program, bool force);

int main(int argc, char *argv[]) {
	ArgumentOptions options(argc, argv);
	
	try {
		InputList toCompile = getInputsToCompile(options);
		InputList toLink = getInputsToLink(options);
		Input *executable = getInputToRun(options);
		
		Program *program = NULL;
		
		bool forceExecution = false;
		
		if (executable) {
			if (!toCompile.empty() || !toLink.empty()) {
				std::cerr << "Cannot compile one file and run a precompiled one." << std::endl;
				return -1;
			}
			
			program = Assembler().assemblyProgram(executable);
			forceExecution = true;
		}
		else {
			InputList inputList = runPreprocessor(options, toCompile);
			ProgramList programs = runCompiler(options, inputList);
			runAssembler(programs, options, toLink);
			
			program = runLinker(options, programs);
		}
		
		runProgram(options, program, forceExecution);
	}
	catch (ParserError & error) {
		std::cerr << error.getMessage() << std::endl;
		return -1;
	}
	catch (std::exception & error) {
		std::cerr << error.what() << std::endl;
		return -1;
	}
	
	return 0;
}

static InputList getInputsToCompile(const ArgumentOptions & options) {
	const ArgumentOptions::FileList & files = options.getFiles();
	InputList result;
	
	for (ArgumentOptions::FileList::const_iterator it = files.begin(); it != files.end(); ++it) {
		if (getFileUpperCaseExtension(*it) == "C") result.push_back(new FileInput(*it));
	}
	
	return result;
}

static InputList getInputsToLink(const ArgumentOptions & options) {
	const ArgumentOptions::FileList & files = options.getFiles();
	InputList result;
	
	for (ArgumentOptions::FileList::const_iterator it = files.begin(); it != files.end(); ++it) {
		if (getFileUpperCaseExtension(*it) == "ASM") result.push_back(new FileInput(*it));
	}
	
	return result;
}

static Input *getInputToRun(const ArgumentOptions & options) {
	const ArgumentOptions::FileList & files = options.getFiles();
	
	for (ArgumentOptions::FileList::const_iterator it = files.begin(); it != files.end(); ++it) {
		std::string ext = getFileUpperCaseExtension(*it);
		if (ext == "VM" || ext == "") return new FileInput(*it);
	}
	
	return NULL;
}

static InputList runPreprocessor(const ArgumentOptions & options, const InputList & inputList) {
	InputList result;
	
	if (options.isPreprocess()) {
		Preprocessor preprocessor(options.getIncludeDirs());
		
		for (InputList::const_iterator it = inputList.begin(); it != inputList.end(); ++it) {
			result.push_back(preprocessor.preprocess(*it));
		}
	}
	else result = inputList;
	
	return result;
}

static ProgramList runCompiler(const ArgumentOptions & options, const InputList & inputList) {
	ProgramList programs;
	
	if (options.getStep() >= ArgumentOptions::COMPILE) {
		Compiler compiler;
		
		programs.reserve(inputList.size());
		
		for (InputList::const_iterator it = inputList.begin(); it != inputList.end(); ++it) {
			programs.push_back(compiler.compile(*it));
		}
	}
	else if (options.getStep() >= ArgumentOptions::CHECK_SYNTAX) {
		Compiler compiler;
		
		for (InputList::const_iterator it = inputList.begin(); it != inputList.end(); ++it) {
			compiler.checkSyntax(*it);
		}
		
		std::cout << "OK" << std::endl;
	}
	else {
		std::fstream output(options.getOutputFile(), std::ios::out);
		
		// just dump the inputs to the output
		for (InputList::const_iterator it = inputList.begin(); it != inputList.end(); ++it) {
			(*it)->dumpInput(output);
			delete(*it);
		}
		
		output.close();
	}
	
	if (options.getStep() == ArgumentOptions::COMPILE) {
		Assembler assembler;
		
		std::fstream output(options.getOutputFile(), std::ios::out);
		
		for (ProgramList::const_iterator it = programs.begin(); it != programs.end(); ++it) {
			output << assembler.disassemblyProgram(*it);
			delete(*it);
		}
		
		output.close();
		
		programs.clear();
	}
	
	return programs;
}

static void runAssembler(ProgramList & programs, const ArgumentOptions & options,
		const InputList & inputList) {
	
	Assembler assembler;
	
	for (InputList::const_iterator it = inputList.begin(); it != inputList.end(); ++it) {
		programs.push_back(assembler.assemblyProgram(*it));
	}
}

static Program *runLinker(const ArgumentOptions & options, ProgramList & programList) {
	Program *program = NULL;
	
	if (options.getStep() >= ArgumentOptions::LINK) {
		Linker linker;
		
		program = linker.linkProgram(programList);
		
		if (options.getStep() == ArgumentOptions::LINK || options.outputDefined()) {
			std::fstream output(options.getOutputFile(), std::ios::out);
			
			output << Assembler().disassemblyProgram(program);
			
			output.close();
		}
	}
	
	return program;
}

static void runProgram(const ArgumentOptions & options, Program *program, bool force) {
	if (options.getStep() >= ArgumentOptions::EXECUTE || force) {
		VirtualMachine vm(program);
		vm.setVerbose(options.isVerbose());
		vm.run();
	}
	
	delete(program);
}
