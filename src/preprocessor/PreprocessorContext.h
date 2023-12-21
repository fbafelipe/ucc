#ifndef PREPROCESSOR_CONTEXT_H
#define PREPROCESSOR_CONTEXT_H

#include <string>

class Input;
class Preprocessor;

class PreprocessorContext {
	public:
		PreprocessorContext(const Preprocessor *preproc);
		~PreprocessorContext();
		
		const Preprocessor *getPreprocessor() const;
		
		void setInput(Input *in);
		
		void setBaseFile(const std::string & base);
		
		void incIncludeLevel();
		void decIncludeLevel();
		
		// standard defines
		std::string define__FILE__() const;
		std::string define__LINE__() const;
		std::string define__INCLUDE_LEVEL__() const;
		std::string define__DATE__() const;
		std::string define__TIME__() const;
		std::string define__STDC__() const;
		std::string define__BASE_FILE__() const;
		std::string define__VERSION__() const;
		
		std::string define__x86__() const;
		std::string define__x86_64__() const;
		std::string define__linux__() const;
		std::string define__WIN32() const;
		
		// compatibility with libc
		std::string define__GNUC__() const;
		
	private:
		const Preprocessor *preprocessor;
		
		Input *input;
		
		std::string baseFile;
		
		unsigned int includeLevel;
};

#endif
