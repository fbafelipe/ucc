#include "compiler/CParser.h"

#include "compiler/ArrayDeclarator.h"
#include "compiler/ArrayType.h"
#include "compiler/Declaration.h"
#include "compiler/Declarator.h"
#include "compiler/DeclaratorBase.h"
#include "compiler/FunctionDeclarator.h"
#include "compiler/FunctionType.h"
#include "compiler/PrimitiveType.h"
#include "compiler/PointerType.h"
#include "compiler/TypeToken.h"
#include "CParserBuffer.h"

#include <cstdlib>

/*
 * <EXTERNAL_DECLARATION> ::= <FUNCTION_DEFINITION>
 *		| <DECLARATION>
 *		;
 */
void CParser::parseExternalDeclaration(NonTerminal *nt) {
	assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_EXTERNAL_DECLARATION);
	
	if (nt->getNonTerminalRule() == 0) {
		parseFunctionDefinition(nt->getNonTerminalAt(0));
	}
	else {
		assert(nt->getNonTerminalRule() == 1);
		
		Pointer<Declaration> decl = parseDeclaration(nt->getNonTerminalAt(0));
		declareGlobal(decl);
	}
}

/*
 * <DECLARATION> ::= <DECLARATION_SPECIFIERS> INST_END
 *		| <DECLARATION_SPECIFIERS> <INIT_DECLARATOR_LIST> INST_END
 *		;
 */
Pointer<Declaration> CParser::parseDeclaration(NonTerminal *nt) {
	assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_DECLARATION);
	
	Pointer<Declaration> decl = new Declaration();
	
	if (nt->getNonTerminalRule() == 0) { // <DECLARATION> ::= <DECLARATION_SPECIFIERS> INST_END
		parseDeclarationSpecifiers(nt->getNonTerminalAt(0), *decl);
	}
	else { // <DECLARATION> ::= <DECLARATION_SPECIFIERS> <INIT_DECLARATOR_LIST> INST_END
		assert(nt->getNonTerminalRule() == 1);
		
		parseDeclarationSpecifiers(nt->getNonTerminalAt(0), *decl);
		parseInitDeclaratorList(nt->getNonTerminalAt(1), *decl);
	}
	
	assert(decl);
	
	return decl;
}

/*
 * <DECLARATION_SPECIFIERS> ::= <STORAGE_CLASS_SPECIFIER>
 *		| <STORAGE_CLASS_SPECIFIER> <DECLARATION_SPECIFIERS>
 *		| <TYPE_SPECIFIER>
 *		| <TYPE_SPECIFIER> <DECLARATION_SPECIFIERS>
 *		| <TYPE_QUALIFIER>
 *		| <TYPE_QUALIFIER> <DECLARATION_SPECIFIERS>
 *		;
 */
void CParser::parseDeclarationSpecifiers(NonTerminal *nt, Declaration & decl) {
	assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_DECLARATION_SPECIFIERS);
	
	switch (nt->getNonTerminalRule()) {
		case 0: // <DECLARATION_SPECIFIERS> ::= <STORAGE_CLASS_SPECIFIER>
			parseStorageClassSpecifier(nt->getNonTerminalAt(0), decl);
			break;
		case 1: // <DECLARATION_SPECIFIERS> ::= <STORAGE_CLASS_SPECIFIER> <DECLARATION_SPECIFIERS>
			parseStorageClassSpecifier(nt->getNonTerminalAt(0), decl);
			parseDeclarationSpecifiers(nt->getNonTerminalAt(1), decl);
			break;
		case 2: // <DECLARATION_SPECIFIERS> ::= <TYPE_SPECIFIER>
			decl.setBaseType(parseTypeSpecifier(nt->getNonTerminalAt(0)));
			break;
		case 3: // <DECLARATION_SPECIFIERS> ::= <TYPE_SPECIFIER> <DECLARATION_SPECIFIERS>
			decl.setBaseType(parseTypeSpecifier(nt->getNonTerminalAt(0)));
			parseDeclarationSpecifiers(nt->getNonTerminalAt(1), decl);
			break;
		case 4: // <DECLARATION_SPECIFIERS> ::= <TYPE_QUALIFIER>
		{
			Pointer<Type> type = decl.getBaseType();
			parseTypeQualifier(nt->getNonTerminalAt(0), type);
			decl.setBaseType(type);
			break;
		}
		case 5: // <DECLARATION_SPECIFIERS> ::= <TYPE_QUALIFIER> <DECLARATION_SPECIFIERS>
		{
			parseDeclarationSpecifiers(nt->getNonTerminalAt(1), decl);
			Pointer<Type> type = decl.getBaseType();
			parseTypeQualifier(nt->getNonTerminalAt(0), type);
			decl.setBaseType(type);
			break;
		}
		default:
			abort();
	}
}

/*
 * <STORAGE_CLASS_SPECIFIER> ::= TYPEDEF
 *		| EXTERN
 *		| STATIC
 *		| AUTO
 *		| REGISTER
 *		;
 */
void CParser::parseStorageClassSpecifier(NonTerminal *nt, Declaration & decl) {
	assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_STORAGE_CLASS_SPECIFIER);
	
	switch (nt->getNonTerminalRule()) {
		case 0: // <STORAGE_CLASS_SPECIFIER> ::= TYPEDEF
			decl.setTypedef(true);
			break;
		case 1: // <STORAGE_CLASS_SPECIFIER> ::= EXTERN
			decl.setExtern(true);
			break;
		case 2: // <STORAGE_CLASS_SPECIFIER> ::= STATIC
			decl.setStatic(true);
			break;
		case 3: // <STORAGE_CLASS_SPECIFIER> ::= AUTO
			// ignore
			break;
		case 4: // <STORAGE_CLASS_SPECIFIER> ::= REGISTER
			// ignore
			break;
		default:
			abort();
	}
}

/*
 * <INIT_DECLARATOR_LIST> ::= <INIT_DECLARATOR>
 *		| <INIT_DECLARATOR_LIST> COMMA <INIT_DECLARATOR>
 *		;
 */
void CParser::parseInitDeclaratorList(NonTerminal *nt, Declaration & decl) {
	assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_INIT_DECLARATOR_LIST);
	
	if (nt->getNonTerminalRule() == 0) { // <INIT_DECLARATOR_LIST> ::= <INIT_DECLARATOR>
		parseInitDeclarator(nt->getNonTerminalAt(0), decl);
	}
	else { // <INIT_DECLARATOR_LIST> ::= <INIT_DECLARATOR_LIST> COMMA <INIT_DECLARATOR>
		assert(nt->getNonTerminalRule() == 1);
		
		parseInitDeclaratorList(nt->getNonTerminalAt(0), decl);
		parseInitDeclarator(nt->getNonTerminalAt(2), decl);
	}
}

/*
 * <INIT_DECLARATOR> ::= <DECLARATOR>
 *		| <DECLARATOR> EQ <INITIALIZER>
 *		;
 */
void CParser::parseInitDeclarator(NonTerminal *nt, Declaration & decl) {
	assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_INIT_DECLARATOR);
	
	if (nt->getNonTerminalRule() == 0) { // <INIT_DECLARATOR> ::= <DECLARATOR>
		Pointer<Declarator> declarator = parseDeclarator(nt->getNonTerminalAt(0), decl.getBaseType());
		decl.addDeclarator(declarator);
	}
	else { // <INIT_DECLARATOR> ::= <DECLARATOR> EQ <INITIALIZER>
		assert(nt->getNonTerminalRule() == 1);
		
		Pointer<Declarator> declarator = parseDeclarator(nt->getNonTerminalAt(0), decl.getBaseType());
		declarator->setInitializer(nt->getNonTerminalAt(2));
		decl.addDeclarator(declarator);
	}
}

/*
 * <DECLARATOR> ::= <POINTER> <DIRECT_DECLARATOR>
 *		| <DIRECT_DECLARATOR>
 *		;
 */
Pointer<Declarator> CParser::parseDeclarator(NonTerminal *nt, const Pointer<Type> & baseType) {
	assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_DECLARATOR);
	
	Pointer<Declarator> declarator;
	
	if (nt->getNonTerminalRule() == 0) { // <DECLARATOR> ::= <POINTER> <DIRECT_DECLARATOR>
		declarator = parseDirectDeclarator(nt->getNonTerminalAt(1), baseType);
		Pointer<Type> type = declarator->getType();
		
		if (type.instanceOf<FunctionType>()) {
			// put the pointer in the return
			Pointer<FunctionType> func = type.staticCast<FunctionType>();
			Pointer<Type> ret = func->getReturnType();
			parsePointer(nt->getNonTerminalAt(0), ret);
			
			// the setType method from FunctionDeclarator will already
			// turn it into a function
			assert(declarator.instanceOf<FunctionDeclarator>());
			type = ret;
		}
		else if (type.instanceOf<ArrayType>()) {
			// put the pointer in the base type
			Pointer<ArrayType> arr = type.staticCast<ArrayType>();
			Pointer<Type> base = arr->dereference();
			parsePointer(nt->getNonTerminalAt(0), base);
			
			type = new ArrayType(base, arr->getCount());
		}
		else parsePointer(nt->getNonTerminalAt(0), type);
		
		declarator->setType(type);
	}
	else { // <DECLARATOR> ::= <DIRECT_DECLARATOR>
		assert(nt->getNonTerminalRule() == 1);
		
		declarator = parseDirectDeclarator(nt->getNonTerminalAt(0), baseType);
	}
	
	assert(declarator);
	
	return declarator;
}

/*
 * <DIRECT_DECLARATOR> ::= IDENTIFIER
 *		| P_OPEN <DECLARATOR> P_CLOSE
 *		| <DIRECT_DECLARATOR> B_OPEN <CONSTANT_EXPRESSION> B_CLOSE
 *		| <DIRECT_DECLARATOR> B_OPEN B_CLOSE
 *		| <DIRECT_DECLARATOR> P_OPEN <PARAMETER_TYPE_LIST> P_CLOSE
 *		| <DIRECT_DECLARATOR> P_OPEN <IDENTIFIER_LIST> P_CLOSE
 *		| <DIRECT_DECLARATOR> P_OPEN P_CLOSE
 *		;
 */
Pointer<Declarator> CParser::parseDirectDeclarator(NonTerminal *nt, const Pointer<Type> & baseType) {
	assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_DIRECT_DECLARATOR);
	
	Pointer<Declarator> declarator;
	
	switch (nt->getNonTerminalRule()) {
		case 0: // <DIRECT_DECLARATOR> ::= IDENTIFIER
			declarator = new DeclaratorBase(baseType);
			declarator->setName(nt->getTokenAt(0)->getToken());
			break;
		case 1: // <DIRECT_DECLARATOR> ::= P_OPEN <DECLARATOR> P_CLOSE
		{
			declarator = parseDeclarator(nt->getNonTerminalAt(1), baseType);
			
			// function pointer have an extra '*'
			// let's dereference it
			Pointer<Type> type = declarator->getType();
			
			assert(type.instanceOf<PointerType>());
			declarator->setType(type.staticCast<PointerType>()->dereference());
			
			break;
		}
		case 2: // <DIRECT_DECLARATOR> ::= <DIRECT_DECLARATOR> B_OPEN <CONSTANT_EXPRESSION> B_CLOSE
			declarator = parseDirectDeclarator(nt->getNonTerminalAt(0), baseType);
			declarator = new ArrayDeclarator(declarator,
					parseConstantExp(nt->getNonTerminalAt(2)).intValue());
			break;
		case 3: // <DIRECT_DECLARATOR> ::= <DIRECT_DECLARATOR> B_OPEN B_CLOSE
			declarator = parseDirectDeclarator(nt->getNonTerminalAt(0), baseType);
			declarator->setType(new ArrayType(declarator->getType()));
			break;
		case 4: // <DIRECT_DECLARATOR> ::=  <DIRECT_DECLARATOR> P_OPEN <PARAMETER_TYPE_LIST> P_CLOSE
		{
			declarator = parseDirectDeclarator(nt->getNonTerminalAt(0), baseType);
			
			DeclaratorList parameterList;
			bool ellipsis = false;
			parseParameterTypeList(nt->getNonTerminalAt(2), parameterList, ellipsis);
			
			declarator = new FunctionDeclarator(declarator, parameterList, ellipsis);
			
			break;
		}
		case 5: // <DIRECT_DECLARATOR> ::= <DIRECT_DECLARATOR> P_OPEN <IDENTIFIER_LIST> P_CLOSE
		{
			declarator = parseDirectDeclarator(nt->getNonTerminalAt(0), baseType);
			
			IdentifierList identifierList;
			parseIdentifierList(nt->getNonTerminalAt(2), identifierList);
			
			DeclaratorList parameterList;
			Pointer<Type> t = new PrimitiveType<int>();
			for (IdentifierList::const_iterator it = identifierList.begin(); it != identifierList.end(); ++it) {
				Pointer<Declarator> d = new DeclaratorBase(t);
				d->setName(*it);
				parameterList.push_back(d);
			}
			
			declarator = new FunctionDeclarator(declarator, parameterList);
			break;
		}
		case 6: // <DIRECT_DECLARATOR> ::= <DIRECT_DECLARATOR> P_OPEN P_CLOSE
		{
			declarator = parseDirectDeclarator(nt->getNonTerminalAt(0), baseType);
			DeclaratorList parameterList;
			
			Pointer<FunctionDeclarator> funcDecl = new FunctionDeclarator(declarator, parameterList);
			funcDecl->setUndefinedParameters(true);
			declarator = funcDecl;
		}
			break;
		default:
			abort();
	}
	
	assert(declarator);
	
	return declarator;
}


/*
 * <ABSTRACT_DECLARATOR> ::= <POINTER>
 *		| <DIRECT_ABSTRACT_DECLARATOR>
 *		| <POINTER> <DIRECT_ABSTRACT_DECLARATOR>
 *		;
 */
Pointer<Declarator> CParser::parseAbstractDeclarator(NonTerminal *nt, const Pointer<Type> & baseType) {
	assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_ABSTRACT_DECLARATOR);
	
	Pointer<Declarator> declarator;
	
	switch (nt->getNonTerminalRule()) {
		case 0: // <ABSTRACT_DECLARATOR> ::= <POINTER>
		{
			Pointer<Type> type = baseType;
			parsePointer(nt->getNonTerminalAt(0), type);
			declarator = new DeclaratorBase(type);
			break;
		}
		case 1: // <ABSTRACT_DECLARATOR> ::= <DIRECT_ABSTRACT_DECLARATOR>
			declarator = parseDirectAbstractDeclarator(nt->getNonTerminalAt(0), baseType);
			break;
		case 2: // <ABSTRACT_DECLARATOR> ::= <POINTER> <DIRECT_ABSTRACT_DECLARATOR>
		{
			declarator = parseDirectAbstractDeclarator(nt->getNonTerminalAt(1), baseType);
			Pointer<Type> type = declarator->getType();
			parsePointer(nt->getNonTerminalAt(0), type);
			declarator->setType(type);
			break;
		}
		default:
			abort();
	}
	
	assert(declarator);
	
	return declarator;
}

/*
 * <DIRECT_ABSTRACT_DECLARATOR> ::= P_OPEN <ABSTRACT_DECLARATOR> P_CLOSE
 *		| B_OPEN B_CLOSE
 *		| B_OPEN <CONSTANT_EXPRESSION> B_CLOSE
 *		| <DIRECT_ABSTRACT_DECLARATOR> B_OPEN B_CLOSE
 *		| <DIRECT_ABSTRACT_DECLARATOR> B_OPEN <CONSTANT_EXPRESSION> B_CLOSE
 *		| P_OPEN P_CLOSE
 *		| P_OPEN <PARAMETER_TYPE_LIST> P_CLOSE
 *		| <DIRECT_ABSTRACT_DECLARATOR> P_OPEN P_CLOSE
 *		| <DIRECT_ABSTRACT_DECLARATOR> P_OPEN <PARAMETER_TYPE_LIST> P_CLOSE
 *		;
 */
Pointer<Declarator> CParser::parseDirectAbstractDeclarator(NonTerminal *nt, const Pointer<Type> & baseType) {
	assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_DIRECT_ABSTRACT_DECLARATOR);
	
	Pointer<Declarator> declarator;
	
	switch (nt->getNonTerminalRule()) {
		case 0: // <DIRECT_ABSTRACT_DECLARATOR> ::= P_OPEN <ABSTRACT_DECLARATOR> P_CLOSE
			declarator = parseAbstractDeclarator(nt->getNonTerminalAt(1), baseType);
			break;
		case 1: // <DIRECT_ABSTRACT_DECLARATOR> ::= B_OPEN B_CLOSE
			declarator = new ArrayDeclarator(new DeclaratorBase(baseType));
			break;
		case 2: // <DIRECT_ABSTRACT_DECLARATOR> ::= B_OPEN <CONSTANT_EXPRESSION> B_CLOSE
			declarator = new ArrayDeclarator(new DeclaratorBase(baseType),
					parseConstantExp(nt->getNonTerminalAt(1)).intValue());
			break;
		case 3: // <DIRECT_ABSTRACT_DECLARATOR> ::= <DIRECT_ABSTRACT_DECLARATOR> B_OPEN B_CLOSE
			declarator = parseDirectAbstractDeclarator(nt->getNonTerminalAt(0), baseType);
			declarator = new ArrayDeclarator(declarator);
			break;
		case 4: // <DIRECT_ABSTRACT_DECLARATOR> ::= <DIRECT_ABSTRACT_DECLARATOR> B_OPEN <CONSTANT_EXPRESSION> B_CLOSE
			declarator = parseDirectAbstractDeclarator(nt->getNonTerminalAt(0), baseType);
			declarator = new ArrayDeclarator(declarator,
					parseConstantExp(nt->getNonTerminalAt(2)).intValue());
			break;
		case 5: // <DIRECT_ABSTRACT_DECLARATOR> ::= P_OPEN P_CLOSE
			break;
			// TODO
			abort();
		case 6: // <DIRECT_ABSTRACT_DECLARATOR> ::= P_OPEN <PARAMETER_TYPE_LIST> P_CLOSE
			// TODO
			abort();
			break;
		case 7: // <DIRECT_ABSTRACT_DECLARATOR> ::= <DIRECT_ABSTRACT_DECLARATOR> P_OPEN P_CLOSE
			// TODO
			abort();
			break;
		case 8: // <DIRECT_ABSTRACT_DECLARATOR> ::= <DIRECT_ABSTRACT_DECLARATOR> P_OPEN <PARAMETER_TYPE_LIST> P_CLOSE
			// TODO
			abort();
			break;
		default:
			abort();
	}
	
	assert(declarator);
	
	return declarator;
}

/*
 * <POINTER> ::= MUL
 *		| MUL <TYPE_QUALIFIER_LIST>
 *		| MUL <POINTER>
 *		| MUL <TYPE_QUALIFIER_LIST> <POINTER>
 *		;
 */
void CParser::parsePointer(NonTerminal *nt, Pointer<Type> & type) {
	assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_POINTER);
	
	assert(type);
	
	switch (nt->getNonTerminalRule()) {
		case 0: // <POINTER> ::= MUL
			type = new PointerType(type);
			break;
		case 1: // <POINTER> ::= MUL <TYPE_QUALIFIER_LIST>
			type = new PointerType(type);
			parseTypeQualifierList(nt->getNonTerminalAt(1), type);
			break;
		case 2: // <POINTER> ::= MUL <POINTER>
			type = new PointerType(type);
			parsePointer(nt->getNonTerminalAt(1), type);
			break;
		case 3: // <POINTER> ::= MUL <TYPE_QUALIFIER_LIST> <POINTER>
			type = new PointerType(type);
			parseTypeQualifierList(nt->getNonTerminalAt(1), type);
			parsePointer(nt->getNonTerminalAt(2), type);
			break;
		default:
			abort();
	}
}

/*
 * <TYPE_QUALIFIER_LIST> ::= <TYPE_QUALIFIER>
 *		| <TYPE_QUALIFIER_LIST> <TYPE_QUALIFIER>
 *		;
 */
void CParser::parseTypeQualifierList(NonTerminal *nt, Pointer<Type> & type) {
	assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_TYPE_QUALIFIER_LIST);
	
	if (nt->getNonTerminalRule() == 0) { // <TYPE_QUALIFIER_LIST> ::= <TYPE_QUALIFIER>
		
	}
	else { // <TYPE_QUALIFIER_LIST> ::= <TYPE_QUALIFIER_LIST> <TYPE_QUALIFIER>
		assert(nt->getNonTerminalRule() == 1);
		
		parseTypeQualifierList(nt->getNonTerminalAt(0), type);
		parseTypeQualifier(nt->getNonTerminalAt(1), type);
	}
}

/*
 * <TYPE_QUALIFIER> ::= CONST
 *		| VOLATILE
 *		;
 */
void CParser::parseTypeQualifier(NonTerminal *nt, Pointer<Type> & type) {
	assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_TYPE_QUALIFIER);
	
	assert(type);
	
	if (nt->getNonTerminalRule() == 0) { // <TYPE_QUALIFIER> ::= CONST
		type->setConstant(true);
	}
	else { // <TYPE_QUALIFIER> ::= VOLATILE
		assert(nt->getNonTerminalRule() == 1);
		
		type->setVolatile(true);
	}
}

/*
 * <PARAMETER_TYPE_LIST> ::= <PARAMETER_LIST>
 *		| <PARAMETER_LIST> COMMA ELLIPSIS
 *		;
 */
void CParser::parseParameterTypeList(NonTerminal *nt, DeclaratorList & parameterList, bool & ellipsis) {
	assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_PARAMETER_TYPE_LIST);
	
	ellipsis = false;
	
	if (nt->getNonTerminalRule() == 0) { // <PARAMETER_TYPE_LIST> ::= <PARAMETER_LIST>
		parseParameterList(nt->getNonTerminalAt(0), parameterList);
	}
	else { // <PARAMETER_TYPE_LIST> ::= <PARAMETER_LIST> COMMA ELLIPSIS
		assert(nt->getNonTerminalRule() == 1);
		
		parseParameterList(nt->getNonTerminalAt(0), parameterList);
		ellipsis = true;
	}
}

/*
 * <PARAMETER_LIST> ::= <PARAMETER_DECLARATION>
 *		| <PARAMETER_LIST> COMMA <PARAMETER_DECLARATION>
 *		;
 */
void CParser::parseParameterList(NonTerminal *nt, DeclaratorList & parameterList) {
	assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_PARAMETER_LIST);
	
	if (nt->getNonTerminalRule() == 0) { // <PARAMETER_LIST> ::= <PARAMETER_DECLARATION>
		Pointer<Declarator> decl = parseParameterDeclaration(nt->getNonTerminalAt(0));
		if (!decl->getType()->isVoid()) {
			parameterList.push_back(parseParameterDeclaration(nt->getNonTerminalAt(0)));
		}
	}
	else { // <PARAMETER_TYPE_LIST> ::= <PARAMETER_LIST> COMMA <PARAMETER_DECLARATION>
		assert(nt->getNonTerminalRule() == 1);
		
		parseParameterList(nt->getNonTerminalAt(0), parameterList);
		parameterList.push_back(parseParameterDeclaration(nt->getNonTerminalAt(2)));
	}
}

/*
 * <PARAMETER_DECLARATION> ::= <DECLARATION_SPECIFIERS> <DECLARATOR>
 *		| <DECLARATION_SPECIFIERS> <ABSTRACT_DECLARATOR>
 *		| <DECLARATION_SPECIFIERS>
 *		;
 */
Pointer<Declarator> CParser::parseParameterDeclaration(NonTerminal *nt) {
	assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_PARAMETER_DECLARATION);
	
	Declaration decl;
	Pointer<Declarator> declarator;
	
	switch (nt->getNonTerminalRule()) {
		case 0: // <PARAMETER_DECLARATION> ::= <DECLARATION_SPECIFIERS> <DECLARATOR>
		{
			parseDeclarationSpecifiers(nt->getNonTerminalAt(0), decl);
			declarator = parseDeclarator(nt->getNonTerminalAt(1), decl.getBaseType());
			decl.addDeclarator(declarator);
			
			break;
		}
		case 1: // <PARAMETER_DECLARATION> ::= <DECLARATION_SPECIFIERS> <ABSTRACT_DECLARATOR>
		{
			parseDeclarationSpecifiers(nt->getNonTerminalAt(0), decl);
			declarator = parseAbstractDeclarator(nt->getNonTerminalAt(1), decl.getBaseType());
			decl.addDeclarator(declarator);
			break;
		}
		case 2: // <PARAMETER_DECLARATION> ::= <DECLARATION_SPECIFIERS>
			parseDeclarationSpecifiers(nt->getNonTerminalAt(0), decl);
			
			declarator = new DeclaratorBase(decl.getBaseType());
			decl.addDeclarator(declarator);
			
			break;
		default:
			abort();
	}
	
	assert(declarator);
	
	return declarator;
}

/*
 * <DECLARATION_LIST> ::= <DECLARATION>
 *		| <DECLARATION_LIST> <DECLARATION>
 *		;
 */
void CParser::parseDeclarationList(NonTerminal *nt, DeclarationList & declarationList) {
	assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_DECLARATION_LIST);
	
	if (nt->getNonTerminalRule() == 0) { // <DECLARATION_LIST> ::= <DECLARATION>
		declarationList.push_back(parseDeclaration(nt->getNonTerminalAt(0)));
	}
	else { // <DECLARATION_LIST> ::= <DECLARATION_LIST> <DECLARATION>
		assert(nt->getNonTerminalRule() == 1);
		
		parseDeclarationList(nt->getNonTerminalAt(0), declarationList);
		declarationList.push_back(parseDeclaration(nt->getNonTerminalAt(1)));
	}
}

/*
 * <TYPE_SPECIFIER> ::= VOID
 *		| CHAR
 *		| SHORT
 *		| INT
 *		| LONG
 *		| FLOAT
 *		| DOUBLE
 *		| SIGNED
 *		| UNSIGNED
 *		| <STRUCT_OR_UNION_SPECIFIER>
 *		| <ENUM_SPECIFIER>
 *		| TYPE_NAME
 *		;
 */
Pointer<Type> CParser::parseTypeSpecifier(NonTerminal *nt) {
	assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_TYPE_SPECIFIER);
	
	Pointer<Type> type;
	
	switch (nt->getNonTerminalRule()) {
		case 0: // <TYPE_SPECIFIER> ::= VOID
			type = new PrimitiveType<void>();
			break;
		case 1: // <TYPE_SPECIFIER> ::= CHAR
			type = new PrimitiveType<char>();
			break;
		case 2: // <TYPE_SPECIFIER> ::= SHORT
			type = new PrimitiveType<short>();
			break;
		case 3: // <TYPE_SPECIFIER> ::= INT
			type = new PrimitiveType<int>();
			break;
		case 4: // <TYPE_SPECIFIER> ::= LONG
			type = new PrimitiveType<long int>();
			break;
		case 5: // <TYPE_SPECIFIER> ::= FLOAT
			type = new PrimitiveType<float>();
			break;
		case 6: // <TYPE_SPECIFIER> ::= DOUBLE
			type = new PrimitiveType<double>();
			break;
		case 7: // <TYPE_SPECIFIER> ::= SIGNED
			type = new PrimitiveType<signed int>();
			break;
		case 8: // <TYPE_SPECIFIER> ::= UNSIGNED
			type = new PrimitiveType<unsigned int>();
			break;
		case 9: // <TYPE_SPECIFIER> ::= <STRUCT_OR_UNION_SPECIFIER>
			// TODO
			abort();
			break;
		case 10: // <TYPE_SPECIFIER> ::= <ENUM_SPECIFIER>
			// TODO
			abort();
			break;
		case 11: // <TYPE_SPECIFIER> ::= TYPE_NAME
			assert(dynamic_cast<TypeToken *>(nt->getTokenAt(0)));
			type = static_cast<TypeToken *>(nt->getTokenAt(0))->getType();
			break;
		default:
			abort();
	}
	
	assert(type);
	
	return type;
}
