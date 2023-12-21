#ifndef DEFINE_MAP_H
#define DEFINE_MAP_H

#include <map>
#include <string>
#include <vector>

class PreprocessorContext;

class DefineMap {
	public:
		typedef std::vector<std::string> ArgumentList;
		
		class MacroToken {
			public:
				MacroToken();
				virtual ~MacroToken();
				
				virtual std::string expandToken(const ArgumentList & args) const = 0;
		};
		typedef std::vector<MacroToken *> MacroTokenList;
		
		class MacroVariable : public MacroToken {
			public:
				MacroVariable(unsigned int varIndex);
				virtual ~MacroVariable();
				
				virtual std::string expandToken(const ArgumentList & args) const;
				
			private:
				unsigned int variableIndex;
		};
		
		class MacroConstant : public MacroToken {
			public:
				MacroConstant(const std::string & c);
				virtual ~MacroConstant();
				
				virtual std::string expandToken(const ArgumentList & args) const;
				
			private:
				std::string constant;
		};
		
		class Macro {
			public:
				Macro(unsigned int numVars);
				~Macro();
				
				void addVariable(unsigned int varIndex);
				void addConstant(const std::string & c);
				
				unsigned int getVariablesCount() const;
				
				std::string expandMacro(const ArgumentList & args) const;
				
			private:
				unsigned int numVariables;
				
				MacroTokenList tokens;
		};
		typedef std::map<std::string, Macro *> MacroMap;
		
		DefineMap(const PreprocessorContext & pc);
		~DefineMap();
		
		bool isDefined(const std::string & name) const;
		bool stdDefDefined(const std::string & name) const;
		bool defineDefined(const std::string & name) const;
		bool macroDefined(const std::string & name) const;
		
		std::string getDefine(const std::string & name) const;
		Macro *getMacro(const std::string & name) const;
		
		void define(const std::string & name, const std::string & value);
		void defineMacro(const std::string & name, Macro *macro);
		
		void undef(const std::string & name);
		
	private:
		typedef std::map<std::string, std::string> DefMap;
		
		// Madness? THIS IS MEMBER FUNCTION POINTER MAP!!!
		typedef std::map<std::string, std::string (PreprocessorContext::*)() const> StdDefMap;
		
		const PreprocessorContext & preprocessorContext;
		
		StdDefMap stdDefMap;
		DefMap defMap;
		MacroMap macroMap;
};

#endif
