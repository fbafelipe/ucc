#include "preprocessor/DefineMap.h"

#include "preprocessor/PreprocessorContext.h"

#include <parser/ParserError.h>

#include <cassert>

// check if a macro name is valid
inline static void checkMacroName(const std::string & name);

/*****************************************************************************
 * DefineMap::MacroToken
 *****************************************************************************/
DefineMap::MacroToken::MacroToken() {}

DefineMap::MacroToken::~MacroToken() {}

/*****************************************************************************
 * DefineMap::MacroVariable
 *****************************************************************************/
DefineMap::MacroVariable::MacroVariable(unsigned int varIndex) : variableIndex(varIndex) {}

DefineMap::MacroVariable::~MacroVariable() {}

std::string DefineMap::MacroVariable::expandToken(const ArgumentList & args) const {
	assert(variableIndex < args.size());
	return args[variableIndex];
}

/*****************************************************************************
 * DefineMap::MacroConstant
 *****************************************************************************/
DefineMap::MacroConstant::MacroConstant(const std::string & c) : constant(c) {}

DefineMap::MacroConstant::~MacroConstant() {}

std::string DefineMap::MacroConstant::expandToken(const ArgumentList & args) const {
	return constant;
}


/*****************************************************************************
 * DefineMap::Macro
 *****************************************************************************/
DefineMap::Macro::Macro(unsigned int numVars) : numVariables(numVars) {}

DefineMap::Macro::~Macro() {
	for (MacroTokenList::iterator it = tokens.begin(); it != tokens.end(); ++it) {
		delete(*it);
	}
}

void DefineMap::Macro::addVariable(unsigned int varIndex) {
	assert(varIndex < numVariables);
	tokens.push_back(new MacroVariable(varIndex));
}

void DefineMap::Macro::addConstant(const std::string & c) {
	tokens.push_back(new MacroConstant(c));
}

unsigned int DefineMap::Macro::getVariablesCount() const {
	return numVariables;
}

std::string DefineMap::Macro::expandMacro(const ArgumentList & args) const {
	std::string result;
	
	if (args.size() != numVariables) throw ParserError("Wrong number of arguments to macro");
	
	for (MacroTokenList::const_iterator it = tokens.begin(); it != tokens.end(); ++it) {
		 result += (*it)->expandToken(args);
	}
	
	return result;
}

/*****************************************************************************
 * DefineMap
 *****************************************************************************/
DefineMap::DefineMap(const PreprocessorContext & pc) : preprocessorContext(pc) {
	stdDefMap["__FILE__"] = &PreprocessorContext::define__FILE__;
	stdDefMap["__LINE__"] = &PreprocessorContext::define__LINE__;
	stdDefMap["__INCLUDE_LEVEL__"] = &PreprocessorContext::define__INCLUDE_LEVEL__;
	stdDefMap["__DATE__"] = &PreprocessorContext::define__DATE__;
	stdDefMap["__TIME__"] = &PreprocessorContext::define__TIME__;
	stdDefMap["__STDC__"] = &PreprocessorContext::define__STDC__;
	stdDefMap["__BASE_FILE__"] = &PreprocessorContext::define__BASE_FILE__;
	stdDefMap["__VERSION__"] = &PreprocessorContext::define__VERSION__;
	
#ifdef __x86__
	stdDefMap["__x86__"] = &PreprocessorContext::define__x86__;
#endif
#ifdef __x86_64__
	stdDefMap["__x86_64__"] = &PreprocessorContext::define__x86_64__;
#endif
#ifdef __linux__
	stdDefMap["__linux__"] = &PreprocessorContext::define__linux__;
#endif
#ifdef __WIN32
	stdDefMap["__WIN32"] = &PreprocessorContext::define__WIN32;
#endif
	
	// compatibility with libc
	stdDefMap["__GNUC__"] = &PreprocessorContext::define__GNUC__;
	
	// these are not hard defined, they exists only for compatibility with libc
	define("__inline", "");
	define("__THROW", "");
	define("__const", "const");
	define("__signed", "signed");
	define("__volatile", "volatile");
	
	Macro *macroNTH = new Macro(1);
	macroNTH->addVariable(0);
	defineMacro("__NTH", macroNTH);
}

DefineMap::~DefineMap() {
	for (MacroMap::iterator it = macroMap.begin(); it != macroMap.end(); ++it) {
		delete(it->second);
	}
}

std::string DefineMap::getDefine(const std::string & name) const {
	checkMacroName(name);
	
	StdDefMap::const_iterator stdIt = stdDefMap.find(name);
	if (stdIt != stdDefMap.end()) return ((&preprocessorContext)->*stdIt->second)();
	
	DefMap::const_iterator it = defMap.find(name);
	assert(it != defMap.end());
	return it->second;
}

DefineMap::Macro *DefineMap::getMacro(const std::string & name) const {
	MacroMap::const_iterator it = macroMap.find(name);
	assert(it != macroMap.end());
	return it->second;
}

bool DefineMap::isDefined(const std::string & name) const {
	checkMacroName(name);
	
	return defineDefined(name) || macroDefined(name);
}

bool DefineMap::stdDefDefined(const std::string & name) const {
	checkMacroName(name);
	
	return stdDefMap.find(name) != stdDefMap.end();
}

bool DefineMap::defineDefined(const std::string & name) const {
	checkMacroName(name);
	
	StdDefMap::const_iterator stdIt = stdDefMap.find(name);
	DefMap::const_iterator it = defMap.find(name);
	
	return stdIt != stdDefMap.end() || it != defMap.end();
}

bool DefineMap::macroDefined(const std::string & name) const {
	checkMacroName(name);
	
	MacroMap::const_iterator it = macroMap.find(name);
	return it != macroMap.end();
}

void DefineMap::define(const std::string & name, const std::string & value) {
	checkMacroName(name);
	
	if (stdDefDefined(name)) return;
	
	if (isDefined(name)) undef(name);
	defMap[name] = value;
}

void DefineMap::defineMacro(const std::string & name, Macro *macro) {
	checkMacroName(name);
	
	if (stdDefDefined(name)) return;
	
	if (isDefined(name)) undef(name);
	macroMap[name] = macro;
}

void DefineMap::undef(const std::string & name) {
	checkMacroName(name);
	assert(isDefined(name));
	
	DefMap::iterator it = defMap.find(name);
	if (it != defMap.end()) defMap.erase(it);
	else {
		MacroMap::iterator mIt = macroMap.find(name);
		macroMap.erase(mIt);
	}
}

#ifdef NDEBUG

// check disabled
inline static void checkMacroName(const std::string & name) {}

#else

#include <parser/Regex.h>

static Regex macroNameRegex("\\w(\\w|\\d)*\\(?");

inline static void checkMacroName(const std::string & name) {
	assert(macroNameRegex.matches(name));
}

#endif
