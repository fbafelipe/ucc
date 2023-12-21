#include "compiler/CParser.h"

#include "compiler/ArrayType.h"
#include "compiler/Compiler.h"
#include "compiler/CompilerContext.h"
#include "compiler/CScanner.h"
#include "compiler/Declaration.h"
#include "compiler/FunctionDeclarator.h"
#include "compiler/FunctionType.h"
#include "compiler/GlobalSymbolTable.h"
#include "compiler/PointerType.h"
#include "compiler/PrimitiveType.h"
#include "vm/ArithmeticInstruction.h"
#include "vm/LabeledInstruction.h"
#include "vm/LoadInstruction.h"
#include "vm/NopInstruction.h"
#include "vm/Program.h"
#include "vm/SetInstruction.h"
#include "vm/StoreInstruction.h"
#include "CParserBuffer.h"

#include <parser/Input.h>
#include <parser/Parser.h>
#include <parser/Regex.h>
#include <parser/Scanner.h>

#include <cassert>
#include <cstdlib>
#include <list>

CParser::CParser(CompilerContext & ctx) : context(ctx) {}

Program *CParser::parse(Input *input) {
	const Compiler *compiler = context.getCompiler();
	scanner = new CScanner(compiler->getScannerAutomata(), input);
	Parser *parser = new Parser(compiler->getParserTable(), scanner);
	
	parser->setParserAction(this);
	
	Node *root = parser->parse();
	parseTranslationUnit((NonTerminal *)root);
	
	delete(parser);
	delete(root);
	
	assert(context.getStartFunction() == context.getCurrentFunction());
	
	CompilerContext::InstructionList instructions = context.getInstructions();
	context.consumeInstructions();
	
	return new Program(context.getStaticMemory()->getMemory(), instructions);
}

/*
 * <DECLARATION> ::= <DECLARATION_SPECIFIERS> INST_END
 *		| <DECLARATION_SPECIFIERS> <INIT_DECLARATOR_LIST> INST_END
 *		;
 */
void CParser::recognized(NonTerminal *nt) {
	if (nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_DECLARATION) {
		Pointer<Declaration> decl = parseDeclaration(nt);
		TypedefManager & typeManager = scanner->getTypedefManager();
		
		if (decl->isTypeDef()) {
			const DeclaratorList & declarators = decl->getDeclarators();
			for (DeclaratorList::const_iterator it = declarators.begin();
					it != declarators.end(); ++it) {
				typeManager.typeDef((*it)->getName(), (*it)->getType());
			}
		}
	}
}

void CParser::addInstruction(Instruction *inst) {
	context.addInstruction(inst);
}

Register CParser::allocatePRRegister() {
	return context.allocatePRRegister();
}

Register CParser::allocateFPRegister() {
	return context.allocateFPRegister();
}

// deallocate a register for the current function
void CParser::deallocateRegister(Register reg) {
	context.deallocateRegister(reg);
}

void CParser::deallocatePRRegister(Register reg) {
	context.deallocatePRRegister(reg);
}

void CParser::deallocateFPRegister(Register reg) {
	context.deallocateFPRegister(reg);
}

Register CParser::allocateConstant(Number value, bool relocable) {
	return context.allocateConstant(value, relocable);
}

Register CParser::allocateConstant(RegisterInt value) {
	return context.allocateConstant(value);
}

Register CParser::allocateConstant(int value) {
	return context.allocateConstant((RegisterInt)value);
}

Register CParser::allocateConstant(unsigned int value) {
	return context.allocateConstant((RegisterInt)value);
}

Register CParser::allocateConstant(RegisterFloat value) {
	return context.allocateConstant(value);
}

Register CParser::allocateString(const std::string & str) {
	return context.allocateString(str);
}

void CParser::stackPush(Register reg, unsigned int size) {
	context.stackPush(reg, size);
}

void CParser::stackPop(Register reg, unsigned int size) {
	context.stackPop(reg, size);
}

/*
 * <TRANSLATION_UNIT> ::= <TRANSLATION_UNIT> <EXTERNAL_DECLARATION>
 *		| <EXTERNAL_DECLARATION>
 *		;
 */
void CParser::parseTranslationUnit(NonTerminal *nt) {
	assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_TRANSLATION_UNIT);
	
	std::list<NonTerminal *> externalDeclarations;
	
	while (nt->getNonTerminalRule() == 0) {
		externalDeclarations.push_front(nt->getNonTerminalAt(1));
		
		nt = nt->getNonTerminalAt(0);
		assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_TRANSLATION_UNIT);
	}
	
	assert(nt->getNonTerminalRule() == 1);
	externalDeclarations.push_front(nt->getNonTerminalAt(0));
	
	for (std::list<NonTerminal *>::const_iterator it = externalDeclarations.begin();
			it != externalDeclarations.end(); ++it) {
		parseExternalDeclaration(*it);
	}
}

/*
 * <FUNCTION_DEFINITION> ::= <DECLARATION_SPECIFIERS> <DECLARATOR> <DECLARATION_LIST> <COMPOUND_STATEMENT>
 *		| <DECLARATION_SPECIFIERS> <DECLARATOR> <COMPOUND_STATEMENT>
 *		| <DECLARATOR> <DECLARATION_LIST> <COMPOUND_STATEMENT>
 *		| <DECLARATOR> <COMPOUND_STATEMENT>
 *		;
 */
void CParser::parseFunctionDefinition(NonTerminal *nt) {
	assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_FUNCTION_DEFINITION);
	
	NonTerminal *functionBody = NULL;
	
	Declaration decl;
	Pointer<Declarator> declarator;
	
	DeclarationList declarationList;
	bool hasDeclarationList = false;;
	
	switch (nt->getNonTerminalRule()) {
		case 0: // <FUNCTION_DEFINITION> ::= <DECLARATION_SPECIFIERS> <DECLARATOR> <DECLARATION_LIST> <COMPOUND_STATEMENT>
			parseDeclarationSpecifiers(nt->getNonTerminalAt(0), decl);
			declarator = parseDeclarator(nt->getNonTerminalAt(1), decl.getBaseType());
			decl.addDeclarator(declarator);
			parseDeclarationList(nt->getNonTerminalAt(2), declarationList);
			hasDeclarationList = true;
			functionBody = nt->getNonTerminalAt(3);
			break;
		case 1: // <FUNCTION_DEFINITION> ::= <DECLARATION_SPECIFIERS> <DECLARATOR> <COMPOUND_STATEMENT>
			parseDeclarationSpecifiers(nt->getNonTerminalAt(0), decl);
			declarator = parseDeclarator(nt->getNonTerminalAt(1), decl.getBaseType());
			decl.addDeclarator(declarator);
			functionBody = nt->getNonTerminalAt(2);
			break;
		case 2: // <FUNCTION_DEFINITION> ::= <DECLARATOR> <DECLARATION_LIST> <COMPOUND_STATEMENT>
			declarator = parseDeclarator(nt->getNonTerminalAt(0), decl.getBaseType());
			decl.addDeclarator(declarator);
			parseDeclarationList(nt->getNonTerminalAt(1), declarationList);
			hasDeclarationList = true;
			functionBody = nt->getNonTerminalAt(2);
			break;
		case 3: // <FUNCTION_DEFINITION> ::= <DECLARATOR> <COMPOUND_STATEMENT>
			declarator = parseDeclarator(nt->getNonTerminalAt(0), decl.getBaseType());
			decl.addDeclarator(declarator);
			functionBody = nt->getNonTerminalAt(1);
			break;
		default:
			abort();
	}
	
	assert(functionBody && functionBody->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_COMPOUND_STATEMENT);
	assert(decl.getDeclarators().size() == 1);
	assert(declarator);
	
	Pointer<FunctionDeclarator> funcDecl = declarator.dynamicCast<FunctionDeclarator>();
	
	if (!funcDecl) {
		throw ParserError(nt->getInputLocation(), "Invalid function definition.");
	}
	
	GlobalSymbolTable *globalSyms = context.getSymbolManager().getGlobalSymbolTable();
	if (!globalSyms->hasFunction(declarator->getName())) declareFunction(funcDecl);
	else {
		const Pointer<Function> & func = globalSyms->getFunction(declarator->getName());
		
		assert(funcDecl->getType().instanceOf<FunctionType>());
		
		// check if the implementation is compatible with the declaration
		const Pointer<FunctionType> & declType = func->getType();
		Pointer<FunctionType> implType = funcDecl->getType().staticCast<FunctionType>();
		
		if (*declType->getReturnType() != *implType->getReturnType()) {
			throw ParserError(nt->getInputLocation(), "Function return type mismatch declaration.");
		}
		
		if (implType->isUndefined()) {
			// no arguments
			TypeList emptyTypeList;
			implType->setTypeList(emptyTypeList);
			implType->setUndefined(false);
		}
		
		if (!declType->isUndefined()) {
			const TypeList & declParams = declType->getTypeList();
			const TypeList & implParams = implType->getTypeList();
			
			if (declParams.size() != implParams.size()) {
				throw ParserError(nt->getInputLocation(), "Function parameters mismatch declaration.");
			}
			
			TypeList::const_iterator declIt = declParams.begin();
			TypeList::const_iterator implIt = implParams.begin();
			for (; declIt != declParams.end(); ++declIt, ++implIt) {
				if (**declIt != **implIt) {
					throw ParserError(nt->getInputLocation(), "Function parameters mismatch declaration.");
				}
			}
		}
		else {
			const TypeList & typeList = implType->getTypeList();
			declType->setTypeList(typeList);
			declType->setUndefined(false);
		}
	}
	
	Pointer<Type> returnType = funcDecl->getReturnType();
	
	const Pointer<Function> & func = context.beginFunction(declarator->getName());
	
	if (func->wasImplemented()) {
		throw ParserError(nt->getInputLocation(),
			std::string("Redefining function ") + declarator->getName());
	}
	
	addInstruction(new LabeledInstruction(func->getName(), new NopInstruction()));
	
	unsigned int returnSize = returnType->getSize();
	// allocate space on stack to the return value
	if (returnSize > 0) context.allocateStack(returnSize);
	
	// create a scope for the function
	// this will help to manage parameters deallocation
	context.beginScope();
	
	// allocate stack space for the parameters
	if (hasDeclarationList) allocateParameters(funcDecl->getParameters(), declarationList);
	else allocateParameters(funcDecl->getParameters());
	
	parseCompoundStatement(functionBody);
	
	context.endScope();
	
	// add the default return
	unsigned int offset = func->getStackBaseOffset();
	addInstruction(new LoadInstruction(REG_PC, REG_SP, REGISTER_SIZE, offset));
	
	context.endFunction();
}

void CParser::allocateParameters(const DeclaratorList & declaratorList, const DeclarationList & declList) {
	int baseOffset = -2 * REGISTER_SIZE; // skip the return addr and argc
	
	unsigned int params = declaratorList.size();
	unsigned int declarationParams = 0;
	
	for (DeclarationList::const_iterator it = declList.begin(); it != declList.end(); ++it) {
		const DeclaratorList & dList = (*it)->getDeclarators();
		
		declarationParams += dList.size();
		allocateParameters(dList, baseOffset);
	}
	
	if (params != declarationParams) throw ParserError("Parameters mismatch.");
}

void CParser::allocateParameters(const DeclaratorList & declList) {
	int baseOffset = -2 * REGISTER_SIZE; // skip the return addr
	
	allocateParameters(declList, baseOffset);
}

void CParser::allocateParameters(const DeclaratorList & declList, int & baseOffset) {
	for (DeclaratorList::const_iterator it = declList.begin(); it != declList.end(); ++it) {
		allocateParameter(**it, baseOffset);
	}
}

void CParser::allocateParameter(const Declarator & decl, int & baseOffset) {
	// the parameters are before the stack base
	assert(baseOffset < 0);
	
	Pointer<Type> type = decl.getType();
	unsigned int typeSize = type->getSize();
	if (type.instanceOf<ArrayType>()) {
		type = new PointerType(type.staticCast<ArrayType>()->dereference());
		typeSize = type->getSize();
	}
	assert(typeSize);
	
	const Pointer<Function> & func = context.getCurrentFunction();
	context.getCurrentScope()->allocateStack(typeSize);
	
	context.allocateStack(typeSize);
	
	int pos = func->getStackBaseOffset();
	
	Pointer<Variable> var = new Variable(type, Variable::LOCAL, pos);
	context.getSymbolManager().addVariable(decl.getName(), var);
	
	// copy the value to the parameter
	int baseOff = func->getStackBaseOffset();
	Register val = allocatePRRegister();
	addInstruction(new LoadInstruction(val, REG_SP, typeSize, baseOff - baseOffset));
	addInstruction(new StoreInstruction(val, REG_SP, typeSize, baseOff - pos));
	deallocateRegister(val);
	
	baseOffset -= typeSize; // move to the next variable
}

/*
 * <TYPE_NAME> ::= <SPECIFIER_QUALIFIER_LIST>
 *		| <SPECIFIER_QUALIFIER_LIST> <ABSTRACT_DECLARATOR>
 *		;
 */
Pointer<Type> CParser::parseTypeName(NonTerminal *nt) {
	assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_TYPE_NAME);
	
	Pointer<Type> type;
	
	if (nt->getNonTerminalRule() == 0) { // <TYPE_NAME> ::= <SPECIFIER_QUALIFIER_LIST>
		parseSpecifierQualifierList(nt->getNonTerminalAt(0), type);
	}
	else { // <TYPE_NAME> ::= <SPECIFIER_QUALIFIER_LIST> <ABSTRACT_DECLARATOR>
		assert(nt->getNonTerminalRule() == 1);
		
		parseSpecifierQualifierList(nt->getNonTerminalAt(0), type);
		Pointer<Declarator> decl = parseAbstractDeclarator(nt->getNonTerminalAt(1), type);
		
		type = decl->getType();
	}
	
	assert(type);
	
	return type;
}

/*
 * <SPECIFIER_QUALIFIER_LIST> ::= <TYPE_SPECIFIER> <SPECIFIER_QUALIFIER_LIST>
 *		| <TYPE_SPECIFIER>
 *		| <TYPE_QUALIFIER> <SPECIFIER_QUALIFIER_LIST>
 *		| <TYPE_QUALIFIER>
 *		;
 */
void CParser::parseSpecifierQualifierList(NonTerminal *nt, Pointer<Type> & type) {
	assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_SPECIFIER_QUALIFIER_LIST);
	
	switch (nt->getNonTerminalRule()) {
		case 0: // <SPECIFIER_QUALIFIER_LIST> ::= <TYPE_SPECIFIER> <SPECIFIER_QUALIFIER_LIST>
			if (type) throw ParserError(nt->getInputLocation(), "More than one data types in specifier list.");
			type = parseTypeSpecifier(nt->getNonTerminalAt(0));
			parseSpecifierQualifierList(nt->getNonTerminalAt(1), type);
			break;
		case 1: // <SPECIFIER_QUALIFIER_LIST> ::= <TYPE_SPECIFIER>
			type = parseTypeSpecifier(nt->getNonTerminalAt(0));
			break;
		case 2: // <SPECIFIER_QUALIFIER_LIST> ::= <TYPE_QUALIFIER> <SPECIFIER_QUALIFIER_LIST>
			parseSpecifierQualifierList(nt->getNonTerminalAt(1), type);
			parseTypeQualifier(nt->getNonTerminalAt(0), type);
			break;
		case 3: // <SPECIFIER_QUALIFIER_LIST> ::= <TYPE_QUALIFIER>
			type = new PrimitiveType<int>();
			parseTypeQualifier(nt->getNonTerminalAt(0), type);
			break;
		default:
			abort();
	}
	
	assert(type);
}

Pointer<Type> CParser::parseTypeFromConstant(Token *tok) {
	static Regex intRegex = Regex("(\\d)+");
	static Regex uintRegex = Regex("(.)+(u|U)");
	static Regex longintRegex = Regex("(.)+l");
	static Regex longlongintRegex = Regex("(.)+L");

	static Regex floatRegex = Regex(".*f");
	static Regex doubleRegex = Regex("(.*F)|(.*\\..*l?)");
	static Regex longdoubleRegex = Regex("(.*[eE\\.].*L)");
	
	static Regex charRegex = Regex("\'[^\']*\'");
	static Regex stringRegex = Regex("\"[^\"]*\"");
	
	const std::string t = tok->getToken();
	
	if (intRegex.matches(t)) return new PrimitiveType<int>();
	if (uintRegex.matches(t)) return new PrimitiveType<unsigned int>();
	if (longintRegex.matches(t)) return new PrimitiveType<long int>();
	if (longlongintRegex.matches(t)) return new PrimitiveType<long long int>();
	
	if (floatRegex.matches(t)) return new PrimitiveType<float>();
	if (doubleRegex.matches(t)) return new PrimitiveType<double>();
	if (longdoubleRegex.matches(t)) return new PrimitiveType<long double>();
	
	if (charRegex.matches(t)) return new PrimitiveType<char>();
	if (stringRegex.matches(t)) return new PointerType(new PrimitiveType<char>());
	
	// unreachable
	abort();
}

/*
 * <IDENTIFIER_LIST> ::= IDENTIFIER
 *		| <IDENTIFIER_LIST> COMMA IDENTIFIER
 *		;
 */
void CParser::parseIdentifierList(NonTerminal *nt, IdentifierList & identifierList) {
	assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_IDENTIFIER_LIST);
	
	if (nt->getNonTerminalRule() == 0) { // <IDENTIFIER_LIST> ::= IDENTIFIER
		identifierList.push_back(nt->getTokenAt(0)->getToken());
	}
	else { // <IDENTIFIER_LIST> ::= <IDENTIFIER_LIST> COMMA IDENTIFIER
		assert(nt->getNonTerminalRule() == 1);
		
		parseIdentifierList(nt->getNonTerminalAt(0), identifierList);
		identifierList.push_back(nt->getTokenAt(1)->getToken());
	}
}

void CParser::declareLocalVariables(const DeclarationList & declarationList) {
	for (DeclarationList::const_iterator it = declarationList.begin(); it != declarationList.end(); ++it) {
		if ((*it)->isStatic()) declareLocalStaticVariable(*it);
		else declareLocalVariable(*it);
	}
}

void CParser::declareLocalVariable(const Pointer<Declaration> & decl) {
	DeclaratorList declarators = decl->getDeclarators();
	
	for (DeclaratorList::const_iterator it = declarators.begin(); it != declarators.end(); ++it) {
		const Pointer<Type> type = (*it)->getType();
		unsigned int typeSize = type->getSize();
		if (type.instanceOf<ArrayType>()) {
			Pointer<ArrayType> arrayType = type.staticCast<ArrayType>();
			if (!arrayType->isCountDefined()) throw ParserError(std::string("array ") + (*it)->getName() + " has no name.");
			typeSize = arrayType->getCount() * arrayType->dereference()->getSize();
		}
		assert(typeSize);
		
		const Pointer<Function> & func = context.getCurrentFunction();
		context.getCurrentScope()->allocateStack(typeSize);
		
		context.allocateStack(typeSize);
		
		unsigned int pos = func->getStackBaseOffset();
		
		Pointer<Variable> var = new Variable(type, Variable::LOCAL, pos);
		context.getSymbolManager().addVariable((*it)->getName(), var);
		
		if ((*it)->hasInitializer()) parseInitializer((*it)->getInitializer(), var);
	}
}

void CParser::declareLocalStaticVariable(const Pointer<Declaration> & decl) {
	// TODO
	abort();
}

void CParser::declareGlobalVariable(const Pointer<Declarator> & decl) {
	const Pointer<Type> type = decl->getType();
	unsigned int typeSize = type->getSize();
	assert(typeSize);
	
	unsigned int pos = context.getStaticMemory()->allocate(typeSize);
	
	Pointer<Variable> var = new Variable(type, Variable::GLOBAL, pos);
	context.getSymbolManager().getGlobalSymbolTable()->addVariable(decl->getName(), var);
	
	if (decl->hasInitializer()) parseInitializer(decl->getInitializer(), var);
}

void CParser::declareFunction(const Pointer<FunctionDeclarator> & decl) {
	assert(decl->getType().instanceOf<FunctionType>());
	
	Pointer<Function> function = new Function(decl->getName());
	
	function->setType(decl->getType().staticCast<FunctionType>());
	
	context.getSymbolManager().getGlobalSymbolTable()->addFunction(decl->getName(), function);
	
}

void CParser::declareGlobal(const Pointer<Declaration> & decl) {
	DeclaratorList declarators = decl->getDeclarators();
	
	for (DeclaratorList::const_iterator it = declarators.begin(); it != declarators.end(); ++it) {
		if (it->instanceOf<FunctionDeclarator>()) declareFunction(it->staticCast<FunctionDeclarator>());
		else declareGlobalVariable(*it);
	}
}

/*
 * <INITIALIZER> ::= <ASSIGNMENT_EXPRESSION>
 *		| BEGIN <INITIALIZER_LIST> END
 *		| BEGIN <INITIALIZER_LIST> COMMA END
 *		;
 */
void CParser::parseInitializer(NonTerminal *nt, const Pointer<Variable> & var) {
	assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_INITIALIZER);
	
	switch (nt->getNonTerminalRule()) {
		case 0: // <INITIALIZER> ::= <ASSIGNMENT_EXPRESSION>
		{
			ExpResult exp = parseAssignmentExp(nt->getNonTerminalAt(0));
			
			if (!var->getType()->fitRegister()) {
				throw ParserError(nt->getInputLocation(), "Invalid initialization.");
			}
			if (!exp.getType()->allowImplicitlyCastTo(var->getType())) {
				throw ParserError(nt->getInputLocation(), "Invalid implicitly cast.");
			}
			
			Register val = exp.getValue(context);
			Register addr = getVariableAddr(var);
			
			addInstruction(new StoreInstruction(val, addr, var->getType()->getSize(), 0));
			
			deallocateRegister(val);
			deallocateRegister(addr);
			
			deallocateExpResult(exp);
			
			break;
		}
		case 1: // <INITIALIZER> ::= BEGIN <INITIALIZER_LIST> END
		case 2: // <INITIALIZER> ::= BEGIN <INITIALIZER_LIST> COMMA END
		{
			Pointer<Type> type = var->getType();
			if (!type.instanceOf<ArrayType>()) {
				throw ParserError(nt->getInputLocation(), "Invalid initialization of non array type.");
			}
			type = type.staticCast<ArrayType>()->dereference();
			
			unsigned int pos = parseInitializerList(nt->getNonTerminalAt(1), type);
			Register val = allocateConstant(Number(Number::INT, (long long int)pos), true);
			Register addr = getVariableAddr(var);
			
			addInstruction(new StoreInstruction(val, addr, var->getType()->getSize(), 0));
			
			deallocateRegister(val);
			deallocateRegister(addr);
			
			break;
		}
		default:
			abort();
	}
}

/*
 * <INITIALIZER_LIST> ::= <INITIALIZER>
 *		| <INITIALIZER_LIST> COMMA <INITIALIZER>
 *		;
 */
unsigned int CParser::parseInitializerList(NonTerminal *nt, const Pointer<Type> & elementType) {
	assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_INITIALIZER_LIST);
	
	std::list<NonTerminal *> ntList;
	
	while (nt) {
		assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_INITIALIZER_LIST);
		
		if (nt->getNonTerminalRule() == 0) { // <INITIALIZER_LIST> ::= <INITIALIZER>
			ntList.push_front(nt->getNonTerminalAt(0));
			nt = NULL;
		}
		else { // <INITIALIZER_LIST> ::= <INITIALIZER_LIST> COMMA <INITIALIZER>
			assert(nt->getNonTerminalRule() == 1);
			
			ntList.push_front(nt->getNonTerminalAt(2));
			
			nt = nt->getNonTerminalAt(0);
		}
	}
	
	/*unsigned char *mem = new unsigned char[elementType->getSize() * ntList.size()];
	
	for (std::list<NonTerminal *>::const_iterator it = ntList.begin(); it != ntList.end(); ++it) {
		
	}*/
	
	// TODO
	abort();
}
