#ifndef ARGUMENT_OPTIONS_H
#define ARGUMENT_OPTIONS_H

#include <string>
#include <vector>

class ArgumentOptions {
	public:
		typedef std::vector<std::string> FileList;
		
		enum Step {
			PREPROCESS = 0,
			CHECK_SYNTAX,
			COMPILE,
			LINK,
			EXECUTE
		};
		
		ArgumentOptions(int argc, char * const argv[]);
		
		const FileList & getFiles() const;
		const FileList & getIncludeDirs() const;
		
		const char *getOutputFile() const;
		bool outputDefined() const;
		Step getStep() const;
		bool isPreprocess() const;
		bool isVerbose() const;
		
		static void showUsage();
		static void showVersion();
		
	private:
		void addIncludeDir(const char *path);
		
		FileList files;
		FileList includeDirs;
		
		const char *output;
		Step step;
		bool preprocess;
		
		bool verbose;
};

#endif
