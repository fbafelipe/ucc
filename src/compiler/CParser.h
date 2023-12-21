#ifndef CPARSER_H
#define CPARSER_H

#include "compiler/Declaration.h"
#include "compiler/Declarator.h"
#include "compiler/ExpResult.h"
#include "compiler/FunctionDeclarator.h"
#include "compiler/Scope.h"
#include "compiler/SwitchStmt.h"
#include "compiler/Variable.h"
#include "vm/RegisterUtils.h"
#include "Number.h"

#include <parser/ParserAction.h>
#include <parser/ParsingTree.h>
#include <parser/Pointer.h>

#include <string>
#include <vector>

class CompilerContext;
class CScanner;
class Declarator;
class Input;
class Instruction;
class Program;

typedef std::vector<std::string> IdentifierList;

class CParser : private ParserAction {
	public:
		typedef ParsingTree::Node Node;
		typedef ParsingTree::NodeList NodeList;
		typedef ParsingTree::NonTerminal NonTerminal;
		typedef ParsingTree::Token Token;
		
		CParser(CompilerContext & ctx);
		
		Program *parse(Input *input);
		
	private:
		void recognized(NonTerminal *nt);
		
		// add an instruction to the current scope
		void addInstruction(Instruction *inst);
		
		// allocate a register for the current function
		Register allocatePRRegister();
		Register allocateFPRegister();
		
		// deallocate a register for the current function
		void deallocateRegister(Register reg);
		void deallocatePRRegister(Register reg);
		void deallocateFPRegister(Register reg);
		
		void parseTranslationUnit(NonTerminal *nt);
		void parseFunctionDefinition(NonTerminal *nt);
		
		void allocateParameters(const DeclaratorList & declaratorList, const DeclarationList & declList);
		void allocateParameters(const DeclaratorList & declList);
		void allocateParameters(const DeclaratorList & declList, int & baseOffset);
		void allocateParameter(const Declarator & decl, int & baseOffset);
		
		// statements
		void parseStatement(NonTerminal *nt, unsigned int scopeFlags = 0);
		void parseLabeledStatement(NonTerminal *nt);
		Pointer<Scope> parseCompoundStatement(NonTerminal *nt, unsigned int scopeFlags = 0);
		void parseExpressionStatement(NonTerminal *nt);
		void parseSelectionStatement(NonTerminal *nt, unsigned int scopeFlags = 0);
		void parseSwitchStatement(NonTerminal *nt, unsigned int scopeFlags = 0);
		Pointer<SwitchStmt> parseSwitch(NonTerminal *nt);
		void parseSwitchStmt(NonTerminal *nt, SwitchStmt & switchStmt, unsigned int scopeFlags = 0);
		void getSwitchLabeledStmtCase(NonTerminal *nt, SwitchStmt & switchStmt);
		void parseSwitchLabeledStatement(NonTerminal *nt, SwitchStmt & switchStmt);
		void parseIterationStatement(NonTerminal *nt);
		void parseForStmt(NonTerminal *init, NonTerminal *expStmt, NonTerminal *inc, NonTerminal *stmt);
		void parseJumpStatement(NonTerminal *nt);
		void parseStatementList(NonTerminal *nt);
		
		// declarations
		void parseExternalDeclaration(NonTerminal *nt);
		Pointer<Declaration> parseDeclaration(NonTerminal *nt);
		void parseDeclarationSpecifiers(NonTerminal *nt, Declaration & decl);
		void parseStorageClassSpecifier(NonTerminal *nt, Declaration & decl);
		void parseInitDeclaratorList(NonTerminal *nt, Declaration & decl);
		void parseInitDeclarator(NonTerminal *nt, Declaration & decl);
		Pointer<Declarator> parseDeclarator(NonTerminal *nt, const Pointer<Type> & baseType);
		Pointer<Declarator> parseDirectDeclarator(NonTerminal *nt, const Pointer<Type> & baseType);
		Pointer<Declarator> parseAbstractDeclarator(NonTerminal *nt, const Pointer<Type> & baseType);
		Pointer<Declarator> parseDirectAbstractDeclarator(NonTerminal *nt, const Pointer<Type> & baseType);
		void parsePointer(NonTerminal *nt, Pointer<Type> & type);
		void parseTypeQualifierList(NonTerminal *nt, Pointer<Type> & type);
		void parseTypeQualifier(NonTerminal *nt, Pointer<Type> & type);
		void parseParameterTypeList(NonTerminal *nt, DeclaratorList & parameterList, bool & ellipsis);
		void parseParameterList(NonTerminal *nt, DeclaratorList & parameterList);
		void parseDeclarationList(NonTerminal *nt, DeclarationList & declarationList);
		Pointer<Declarator> parseParameterDeclaration(NonTerminal *nt);
		Pointer<Type> parseTypeSpecifier(NonTerminal *nt);
		
		void storeInVariable(Register reg, const Pointer<Variable> & var);
		void getVariableAddr(Register reg, const Pointer<Variable> & var);
		Register getVariableAddr(const Pointer<Variable> & var);
		
		// expression
		// return the register with the result
		void checkVoidExp(const ExpResult & exp, ParsingTree::Node *node); // check if a used result is void
		unsigned int parseArgumentExpList(NonTerminal *nt, const Pointer<Type> & type);
		ExpResult parsePrimaryExpIdentifier(Token *token);
		ExpResult parsePrimaryExp(NonTerminal *nt);
		ExpResult parsePostfixExp(NonTerminal *nt);
		ExpResult parseUnaryExp(NonTerminal *nt);
		void parseUnaryOperator(NonTerminal *nt, ExpResult & exp);
		Pointer<Type> parseTypeFromConstant(Token *tok);
		ExpResult parseCastExp(NonTerminal *nt);
		ExpResult parseMultiplicativeExp(NonTerminal *nt);
		ExpResult parseAdditiveExp(NonTerminal *nt);
		ExpResult parseShiftExp(NonTerminal *nt);
		ExpResult parseRelationalExp(NonTerminal *nt);
		ExpResult parseEqualityExp(NonTerminal *nt);
		ExpResult parseAndExp(NonTerminal *nt);
		ExpResult parseExclusiveOrExp(NonTerminal *nt);
		ExpResult parseInclusiveOrExp(NonTerminal *nt);
		ExpResult parseLogicalAndExp(NonTerminal *nt);
		ExpResult parseLogicalOrExp(NonTerminal *nt);
		ExpResult parseConditionalExp(NonTerminal *nt);
		ExpResult parseAssignmentExp(NonTerminal *nt);
		void parseAssigmentOperator(NonTerminal *nt, ExpResult & result, ExpResult& value);
		ExpResult parseExp(NonTerminal *nt);
		void deallocateExpResult(const ExpResult & exp);
		Number parseConstantExp(NonTerminal *nt);
		
		// type
		Pointer<Type> parseTypeName(NonTerminal *nt);
		void parseSpecifierQualifierList(NonTerminal *nt, Pointer<Type> & type);
		
		void parseIdentifierList(NonTerminal *nt, IdentifierList & identifierList);
		
		Register allocateConstant(Number value, bool relocable = false);
		Register allocateConstant(RegisterInt value);
		Register allocateConstant(int value);
		Register allocateConstant(unsigned int value);
		Register allocateConstant(RegisterFloat value);
		
		Register allocateString(const std::string & str);
		
		void declareLocalVariables(const DeclarationList & declarationList);
		void declareLocalVariable(const Pointer<Declaration> & decl);
		void declareLocalStaticVariable(const Pointer<Declaration> & decl);
		void declareGlobalVariable(const Pointer<Declarator> & decl);
		void declareFunction(const Pointer<FunctionDeclarator> & decl);
		void declareGlobal(const Pointer<Declaration> & decl);
		
		void parseInitializer(NonTerminal *nt, const Pointer<Variable> & var);
		unsigned int parseInitializerList(NonTerminal *nt, const Pointer<Type> & elementType);
		
		void stackPush(Register reg, unsigned int size = REGISTER_SIZE);
		void stackPop(Register reg, unsigned int size = REGISTER_SIZE);
		
		CompilerContext & context;
		
		CScanner *scanner;
};

#endif
