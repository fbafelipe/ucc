#include "ArgumentOptions.h"

#include "UccDefs.h"
#include "UccUtils.h"

#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include <getopt.h>
#include <unistd.h>

ArgumentOptions::ArgumentOptions(int argc, char * const argv[]) {
	includeDirs.push_back("./");
	output = NULL;
	step = LINK;
	preprocess = true;
	
	verbose = false;
	
	const char *shortOptions = "cEI:ho:rsevV";
	struct option longOptions[] = {
			{"include", true, NULL, 'I'},
			{"output", true, NULL, 'o'},
			{"run", false, NULL, 'r'},
			{"syntax", false, NULL, 's'},
			{"nopreprocessor", false, NULL, 'e'},
			{"version", false, NULL, 'v'},
			{"verbose", false, NULL, 'V'},
	};
	int longIndex;
	
	int c;
	while ((c = getopt_long(argc, argv, shortOptions, longOptions, &longIndex)) != -1) {
		switch (c) {
			case 'c':
				step = COMPILE;
				break;
			case 'e':
				preprocess = false;
				break;
			case 'E':
				step = PREPROCESS;
				break;
			case 'h':
				showUsage();
				exit(0);
				break;
			case 'I':
				addIncludeDir(optarg);
				break;
			case 'o':
				output = optarg;
				break;
			case 'r':
				step = EXECUTE;
				break;
			case 's':
				step = CHECK_SYNTAX;
				break;
			case 'v':
				showVersion();
				exit(0);
				break;
			case 'V':
				verbose = true;
				break;
			case '?':
				// long opt already printed an error
				showUsage();
				exit(-1);
				break;
			default:
				abort();
		}
	}
	
	assert(optind >= 0);
	for (int i = optind; i < argc; ++i) {
		std::string ext = getFileUpperCaseExtension(argv[i]);
		if (ext != "C" && ext != "ASM" && ext != "VM" && ext != "") {
			std::cerr << "Unknown file type: " << argv[i] << std::endl;
			exit(-1);
		}
		
		files.push_back(argv[i]);
	}
	
	if (files.empty()) {
		std::cerr << "ucc: no input files." << std::endl;
		exit(-1);
	}
}

const ArgumentOptions::FileList & ArgumentOptions::getFiles() const {
	return files;
}

const ArgumentOptions::FileList & ArgumentOptions::getIncludeDirs() const {
	return includeDirs;
}

const char *ArgumentOptions::getOutputFile() const {
	if (!output) return "a.out";
	return output;
}

bool ArgumentOptions::outputDefined() const {
	return output;
}

ArgumentOptions::Step ArgumentOptions::getStep() const {
	return step;
}

bool ArgumentOptions::isPreprocess() const {
	return preprocess;
}

bool ArgumentOptions::isVerbose() const {
	return verbose;
}

void ArgumentOptions::addIncludeDir(const char *path) {
	unsigned int len = strlen(path);
	if (!len) return;
	if (path[len - 1] == '/') includeDirs.push_back(path);
	else includeDirs.push_back(std::string(path) + "/");
}

void ArgumentOptions::showUsage() {
	std::cerr << "Usage: ucc [OPTIONS] FILE..." << std::endl;
	
	std::cerr << "  -c\t\t\t Compile only." << std::endl;
	std::cerr << "  -e, --no-preprocessor\t Do not run the preprocessor." << std::endl;
	std::cerr << "  -E\t\t\t Preprocess only." << std::endl;
	std::cerr << "  -h, --help\t\t Show this help and exit." << std::endl;
	std::cerr << "  -o, --output <file>\t Specify the output file." << std::endl;
	std::cerr << "  -r, --run\t\t Run the program." << std::endl;
	std::cerr << "  -s, --syntax\t\t Syntax check only." << std::endl;
	std::cerr << "  -v, --version\t\t Show the version and exit." << std::endl;
	std::cerr << "  -V, --verbose\t\t Show debug infromation." << std::endl;
}

void ArgumentOptions::showVersion() {
	std::cerr << "ucc " << UCC_VERSION << std::endl;
}
