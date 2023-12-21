#include "preprocessor/PreprocessorContext.h"

#include "UccDefs.h"

#include <parser/Input.h>

#include <cassert>
#include <cstdio>
#include <cstring>
#include <ctime>

#include <sys/time.h>

PreprocessorContext::PreprocessorContext(const Preprocessor *preproc) :
		preprocessor(preproc), input(NULL), includeLevel(0) {}

PreprocessorContext::~PreprocessorContext() {}

const Preprocessor *PreprocessorContext::getPreprocessor() const {
	return preprocessor;
}

void PreprocessorContext::setInput(Input *in) {
	input = in;
}

void PreprocessorContext::setBaseFile(const std::string & base) {
	baseFile = base;
}

void PreprocessorContext::incIncludeLevel() {
	++includeLevel;
}

void PreprocessorContext::decIncludeLevel() {
	assert(includeLevel > 0);
	--includeLevel;
}

std::string PreprocessorContext::define__FILE__() const {
	assert(input);
	return std::string("\"") + input->getInputName() + "\"";
}

std::string PreprocessorContext::define__LINE__() const {
	assert(input);
	
	char buf[32];
	sprintf(buf, "%d", (int)input->getInputLine());
	
	return std::string(buf);
}

std::string PreprocessorContext::define__INCLUDE_LEVEL__() const {
	char buf[32];
	sprintf(buf, "%u", includeLevel);
	return std::string(buf);
}

std::string PreprocessorContext::define__DATE__() const {
	const char *MONTHS[12] = {
			"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" 
	};
	
	time_t t = time(NULL);
	tm *dateTm = localtime(&t);
	
	assert(dateTm->tm_mon >= 0 && dateTm->tm_mon < 12);
	
	char buf[32];
	sprintf(buf, "\"%s %2d %4d\"", MONTHS[dateTm->tm_mon],dateTm->tm_mday, 1900 + dateTm->tm_year);
	assert(strlen(buf) == 13);
	
	return std::string(buf);
}

std::string PreprocessorContext::define__TIME__() const {
	time_t t = time(NULL);
	tm *timeTm = localtime(&t);
	
	char buf[32];
	sprintf(buf, "\"%02d:%02d:%02d\"", timeTm->tm_hour, timeTm->tm_min, timeTm->tm_sec);
	assert(strlen(buf) == 10);
	
	return std::string(buf);
}

std::string PreprocessorContext::define__STDC__() const {
	return std::string("1");
}

std::string PreprocessorContext::define__BASE_FILE__() const {
	return std::string("\"") + baseFile + "\"";
}

std::string PreprocessorContext::define__VERSION__() const {
	return UCC_VERSION;
}

std::string PreprocessorContext::define__x86__() const {
#ifdef __x86__
	return "1";
#else
	return "0";
#endif
}

std::string PreprocessorContext::define__x86_64__() const {
#ifdef __x86_64__
	return "1";
#else
	return "0";
#endif
}

std::string PreprocessorContext::define__linux__() const {
#ifdef __linux__
	return "1";
#else
	return "0";
#endif
}

std::string PreprocessorContext::define__WIN32() const {
#ifdef __WIN32
	return "1";
#else
	return "0";
#endif
}

std::string PreprocessorContext::define__GNUC__() const {
	return "0";
}
