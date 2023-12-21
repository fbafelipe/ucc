#include "compiler/CParser.h"

#include "compiler/ArrayType.h"
#include "compiler/CompilerContext.h"
#include "compiler/FunctionType.h"
#include "compiler/GlobalSymbolTable.h"
#include "compiler/PointerType.h"
#include "compiler/PrimitiveType.h"
#include "compiler/PrimitiveTypeBase.h"
#include "compiler/Type.h"
#include "vm/ArithmeticInstruction.h"
#include "vm/BranchInstruction.h"
#include "vm/CopyInstruction.h"
#include "vm/JumpRegisterInstruction.h"
#include "vm/LoadAddrInstruction.h"
#include "vm/LoadInstruction.h"
#include "vm/NotInstruction.h"
#include "vm/SetInstruction.h"
#include "vm/StoreInstruction.h"
#include "CParserBuffer.h"
#include "UccUtils.h"

#include <parser/ParserError.h>

#include <cassert>
#include <cstdlib>

void CParser::checkVoidExp(const ExpResult & exp, ParsingTree::Node *node) {
	if (exp.getType() == ExpResult::VOID) {
		throw ParserError(node->getInputLocation(), "Void return not ignored as it should be.");
	}
}

void CParser::storeInVariable(Register reg, const Pointer<Variable> & var) {
	assert(var);
	
	switch (var->getVariableType()) {
		case Variable::GLOBAL:
			addInstruction(new StoreInstruction(reg, REG_GP, var->getType()->getSize(), var->getPosition()));
			break;
		case Variable::LOCAL:
		{
			unsigned int offset = context.getCurrentFunction()->getStackBaseOffset() - var->getPosition();
			addInstruction(new StoreInstruction(reg, REG_SP, var->getType()->getSize(), offset));
			break;
		}
		default:
			abort();
	}
}

void CParser::getVariableAddr(Register reg, const Pointer<Variable> & var) {
	switch (var->getVariableType()) {
		case Variable::GLOBAL:
		{
			SetInstruction *setInst = new SetInstruction(reg, var->getPosition());
			setInst->setRelocable(true);
			addInstruction(setInst);
			addInstruction(new AddInstruction(reg, REG_GP, reg));
			break;
		}
		case Variable::LOCAL:
		{
			assert(context.getCurrentFunction()->getStackBaseOffset() >= var->getPosition());
			unsigned int offset = context.getCurrentFunction()->getStackBaseOffset() - var->getPosition();
			addInstruction(new SetInstruction(reg, offset));
			addInstruction(new AddInstruction(reg, REG_SP, reg));
			
			break;
		}
		default:
			abort();
	}
}

Register CParser::getVariableAddr(const Pointer<Variable> & var) {
	Register reg = allocatePRRegister();
	getVariableAddr(reg, var);
	return reg;
}

ExpResult CParser::parsePrimaryExpIdentifier(Token *token) {
	assert(token->getTokenTypeId() == CPARSERBUFFER_TOKEN_IDENTIFIER);
	
	ExpResult result(ExpResult::STACKED);
	
	const std::string & name = token->getToken();
	
	const SymbolManager & sManager = context.getSymbolManager();
	GlobalSymbolTable *globalSyms = sManager.getGlobalSymbolTable();
	
	if (sManager.hasVariable(name)) {
		const Pointer<Variable> & var = sManager.getVariable(name);
		
		Register reg = getVariableAddr(var);
		stackPush(reg);
		deallocateRegister(reg);
		
		result.setType(var->getType());
		result.setOffsetFromBase(context);
		result.setNeedDealocate(true);
		
		// for arrays, it's address should be stored,
		// so it's not dynamic
		result.setDynamic(!var->getType().instanceOf<ArrayType>());
	}
	else if (globalSyms->hasFunction(name)) {
		const Pointer<Function> & func = globalSyms->getFunction(name);
		
		Register addr = allocatePRRegister();
		addInstruction(new LoadAddrInstruction(addr, func->getName()));
		stackPush(addr);
		deallocateRegister(addr);
		
		result.setType(func->getType());
		result.setOffsetFromBase(context);
		result.setNeedDealocate(true);
	}
	else {
		throw ParserError(token->getInputLocation(),
				std::string("Use of undefined identifier \"") + name + "\"");
	}
	
	return result;
}

/*
 * Return the amount of stack memory was used.
 * 
 * <ARGUMENT_EXPRESSION_LIST> ::= <ASSIGNMENT_EXPRESSION>
 *		| <ARGUMENT_EXPRESSION_LIST> COMMA <ASSIGNMENT_EXPRESSION>
 *		;
 */
unsigned int CParser::parseArgumentExpList(NonTerminal *nt, const Pointer<Type> & type) {
	assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_ARGUMENT_EXPRESSION_LIST);
	assert(type.dynamicCast<FunctionType>());
	
	Pointer<FunctionType> functionType = type.staticCast<FunctionType>();
	const TypeList & typeList = functionType->getTypeList();
	bool ellipsis = functionType->hasEllipsis();
	
	// list with the expression of the arguments
	// in the order that they will be pushed (reverse order from declared)
	std::vector<NonTerminal *> nonTerminals;
	
	while (nt) {
		assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_ARGUMENT_EXPRESSION_LIST);
		
		if (nt->getNonTerminalRule() == 0) { // <ARGUMENT_EXPRESSION_LIST> ::= <ASSIGNMENT_EXPRESSION>
			nonTerminals.push_back(nt->getNonTerminalAt(0));
			nt = NULL;
		}
		else { // <ARGUMENT_EXPRESSION_LIST> ::= <ARGUMENT_EXPRESSION_LIST> COMMA <ASSIGNMENT_EXPRESSION>
			assert(nt->getNonTerminalRule() == 1);
			
			nonTerminals.push_back(nt->getNonTerminalAt(2));
			nt =  nt->getNonTerminalAt(0);
		}
	}
	
	if (nonTerminals.size() < typeList.size()) {
		throw ParserError(nt->getInputLocation(), "Too few arguments.");
	}
	if (!ellipsis && nonTerminals.size() > typeList.size()) {
		throw ParserError(nt->getInputLocation(), "Too many arguments.");
	}
	
	assert(nonTerminals.size() >= typeList.size());
	unsigned int ellipsisArguments = nonTerminals.size() - typeList.size();
	assert(ellipsis || ellipsisArguments == 0);
	
	std::vector<NonTerminal *>::const_iterator ntIt = nonTerminals.begin();
	
	unsigned int mem = 0;
	
	for (unsigned int i = 0; i < ellipsisArguments; ++i, ++ntIt) {
		// no need to cast the type
		ExpResult exp = parseAssignmentExp(*ntIt);
		checkVoidExp(exp, *ntIt);
		
		unsigned int size = exp.getType()->getSize();
		
		// TODO make this work with structs
		Register val = exp.getValue(context);
		
		if (exp.getResultType() == ExpResult::CONSTANT) {
			stackPush(val, size);
		}
		else if (exp.isDynamic()) {
			deallocateExpResult(exp);
			stackPush(val, size);
		}
		
		deallocateRegister(val);
		
		mem += size;
	}
	
	for (TypeList::const_reverse_iterator it = typeList.rbegin(); it != typeList.rend(); ++it, ++ntIt) {
		ExpResult exp = parseAssignmentExp(*ntIt);
		checkVoidExp(exp, *ntIt);
		
		if (!exp.getType()->allowImplicitlyCastTo(*it)) {
			throw ParserError((*ntIt)->getInputLocation(), "Invalid implicitly cast.");
		}
		
		unsigned int size = (*it)->getSize();
		
		Register val = exp.getValue(context);
		
		if (exp.getType()->isFloatingPoint() ^ (*it)->isFloatingPoint()) {
			Register castVal;
			if ((*it)->isFloatingPoint()) castVal = allocateFPRegister();
			else castVal = allocatePRRegister();
			
			addInstruction(new AddInstruction(castVal, val, REG_ZERO));
			deallocateRegister(val);
			
			deallocateExpResult(exp);
			stackPush(castVal, size);
			
			deallocateRegister(castVal);
		}
		else {
			if (exp.getResultType() == ExpResult::CONSTANT) {
				stackPush(val, size);
			}
			else if ((*it)->getSize() != size || exp.isDynamic()) {
				deallocateExpResult(exp);
				stackPush(val, size);
			}
			
			deallocateRegister(val);
		}
		
		
		mem += size;
	}
	
	// push the argument count
	Register reg = allocateConstant((RegisterInt)nonTerminals.size());
	stackPush(reg, REGISTER_SIZE);
	deallocateRegister(reg);
	mem += REGISTER_SIZE;
	
	return mem;
}

/*
 * <PRIMARY_EXPRESSION> ::= IDENTIFIER
 *		| CONSTANT
 *		| STRING_LITERAL
 *		| P_OPEN <EXPRESSION> P_CLOSE
 *		;
 */
ExpResult CParser::parsePrimaryExp(NonTerminal *nt) {
	assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_PRIMARY_EXPRESSION);
	
	ExpResult result;
	
	switch (nt->getNonTerminalRule()) {
		case 0: // <PRIMARY_EXPRESSION> ::= IDENTIFIER
			result = parsePrimaryExpIdentifier(nt->getTokenAt(0));
			break;
		case 1: // <PRIMARY_EXPRESSION> ::= CONSTANT
			result.setResultType(ExpResult::CONSTANT);
			result.setType(parseTypeFromConstant(nt->getTokenAt(0)));
			result.setConstant(evaluateConstant(nt->getTokenAt(0)));
			break;
		case 2: // <PRIMARY_EXPRESSION> ::= STRING_LITERAL
		{
			Register pos = allocateString(getStringWithoutScapes(cleanString(nt->getTokenAt(0)->getToken())));
			stackPush(pos);
			deallocateRegister(pos);
			
			result.setResultType(ExpResult::STACKED);
			result.setType(new PointerType(new PrimitiveType<char>()));
			result.setOffsetFromBase(context);
			result.setNeedDealocate(true);
			break;
		}
		case 3: // <PRIMARY_EXPRESSION> ::= P_OPEN <EXPRESSION> P_CLOSE
			result = parseExp(nt->getNonTerminalAt(1));
			break;
		default:
			abort();
	}
	
	return result;
}

/*
 * <POSTFIX_EXPRESSION> ::= <PRIMARY_EXPRESSION>
 *		| <POSTFIX_EXPRESSION> B_OPEN <EXPRESSION> B_CLOSE
 *		| <POSTFIX_EXPRESSION> P_OPEN P_CLOSE
 *		| <POSTFIX_EXPRESSION> P_OPEN <ARGUMENT_EXPRESSION_LIST> P_CLOSE
 *		| <POSTFIX_EXPRESSION> DOT IDENTIFIER
 *		| <POSTFIX_EXPRESSION> PTR_OP IDENTIFIER
 *		| <POSTFIX_EXPRESSION> INC_OP
 *		| <POSTFIX_EXPRESSION> DEC_OP
 *		;
 */
ExpResult CParser::parsePostfixExp(NonTerminal *nt) {
	assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_POSTFIX_EXPRESSION);
	
	ExpResult result;
	
	switch (nt->getNonTerminalRule()) {
		case 0: // <POSTFIX_EXPRESSION> ::= <PRIMARY_EXPRESSION>
			result = parsePrimaryExp(nt->getNonTerminalAt(0));
			break;
		case 1: // <POSTFIX_EXPRESSION> ::= <POSTFIX_EXPRESSION> B_OPEN <EXPRESSION> B_CLOSE
		{
			ExpResult base = parsePostfixExp(nt->getNonTerminalAt(0));
			ExpResult offset = parseExp(nt->getNonTerminalAt(2));
			
			checkVoidExp(base, nt->getNonTerminalAt(0));
			checkVoidExp(offset, nt->getNonTerminalAt(2));
			
			Pointer<Type> type = base.getType();
			if (type->getTypeEnum() != Type::POINTER && type->getTypeEnum() != Type::ARRAY) {
				ExpResult aux = base;
				base = offset;
				offset = aux;
			}
			
			type = base.getType();
			Pointer<Type> elementType;
			if (type->getTypeEnum() == Type::POINTER) elementType = type.staticCast<PointerType>()->dereference();
			else if (type->getTypeEnum() == Type::ARRAY) elementType = type.staticCast<ArrayType>()->dereference();
			else throw ParserError(nt->getInputLocation(), "Non pointer array access.");
			
			assert(elementType);
			if (elementType->getSize() == 0) throw ParserError(nt->getInputLocation(), "Cannot dereference void.");
			
			Register pos = base.getValue(context);
			Register off = offset.getValue(context);
			
			deallocateExpResult(offset);
			deallocateExpResult(base);
			
			// multiply the offset by the element size
			Register s = allocateConstant(elementType->getSize());
			addInstruction(new MulInstruction(off, off, s));
			deallocateRegister(s);
			
			addInstruction(new AddInstruction(pos, pos, off));
			
			stackPush(pos);
			
			deallocateRegister(pos);
			deallocateRegister(off);
			
			result.setResultType(ExpResult::STACKED);
			result.setType(elementType);
			result.setOffsetFromBase(context);
			result.setNeedDealocate(true);
			result.setDynamic(true);
			
			break;
		}
		case 2: // <POSTFIX_EXPRESSION> ::= <POSTFIX_EXPRESSION> P_OPEN P_CLOSE
		{
			result = parsePostfixExp(nt->getNonTerminalAt(0));
			if (!result.getType().instanceOf<FunctionType>()) {
				throw ParserError(nt->getInputLocation(), "Invalid function call");
			}
			Pointer<FunctionType> funcType = result.getType().staticCast<FunctionType>();
			
			// push the argument count
			stackPush(REG_ZERO);
			unsigned int stackMem = REGISTER_SIZE;
			
			// get the function addr
			Register funcAddr = result.getValue(context);
			
			// push the return addr
			Register reg = allocatePRRegister();
			
			// use some utility functions may be dangerous here
			// since we set the offset hardcoded
			
			// grow the stack for the return value
			context.allocateStack(REGISTER_SIZE);
			stackMem += REGISTER_SIZE;
			
			// jump 2 instructions: the push and the call
			// the offset is from the add, so it already will be skipped
			addInstruction(new SetInstruction(reg, 2));
			addInstruction(new AddInstruction(reg, REG_PC, reg));
			addInstruction(new StoreInstruction(reg, REG_SP, REGISTER_SIZE, 0));
			deallocatePRRegister(reg);
			
			addInstruction(new JumpRegisterInstruction(funcAddr));
			
			deallocatePRRegister(funcAddr);
			assert(context.getCurrentFunction()->allRegistersFree());
			
			// TODO make the return value works when its a struct
			reg = REG_NOTUSED;
			
			if (!funcType->getReturnType()->isVoid() > 0) {
				// get the return value
				reg = allocatePRRegister();
				Register r = allocatePRRegister();
				
				unsigned int retSize = funcType->getReturnType()->getSize();
				assert(retSize > 0);
				
				// here we can't use stackPop since it would mess with stack base,
				// since it was pushed by the callee
				addInstruction(new LoadInstruction(reg, REG_SP, retSize, 0));
				addInstruction(new SetInstruction(r, retSize));
				addInstruction(new AddInstruction(REG_SP, REG_SP, r));
				
				deallocatePRRegister(r);
				// reg will be used later, when pushing the result (after removing the arguments)
			}
			
			// deallocate arguments from the stack
			context.deallocateStack(stackMem);
			
			deallocateExpResult(result);
			
			if (funcType->getReturnType()->getSize() > 0) {
				stackPush(reg, funcType->getReturnType()->getSize());
				deallocateRegister(reg);
				
				result.setResultType(ExpResult::STACKED);
				result.setType(funcType->getReturnType());
				result.setOffsetFromBase(context);
				result.setNeedDealocate(true);
				result.setDynamic(false);
			}
			else result.setResultType(ExpResult::VOID);
			
			break;
		}
		case 3: // <POSTFIX_EXPRESSION> ::= <POSTFIX_EXPRESSION> P_OPEN <ARGUMENT_EXPRESSION_LIST> P_CLOSE
		{
			result = parsePostfixExp(nt->getNonTerminalAt(0));
			if (!result.getType().instanceOf<FunctionType>()) {
				throw ParserError(nt->getInputLocation(), "Invalid function call");
			}
			Pointer<FunctionType> funcType = result.getType().staticCast<FunctionType>();
			
			unsigned int stackMem = parseArgumentExpList(nt->getNonTerminalAt(2), funcType);
			
			// get the function addr
			Register funcAddr = result.getValue(context);
			
			// push the return addr
			Register reg = allocatePRRegister();
			
			// use some utility functions may be dangerous here
			// since we set the offset hardcoded
			
			// grow the stack for the return value
			context.allocateStack(REGISTER_SIZE);
			stackMem += REGISTER_SIZE;
			
			// jump 2 instructions: the push and the call
			// the offset is from the add, so it already will be skipped
			addInstruction(new SetInstruction(reg, 2));
			addInstruction(new AddInstruction(reg, REG_PC, reg));
			addInstruction(new StoreInstruction(reg, REG_SP, REGISTER_SIZE, 0));
			deallocatePRRegister(reg);
			
			addInstruction(new JumpRegisterInstruction(funcAddr));
			
			deallocatePRRegister(funcAddr);
			assert(context.getCurrentFunction()->allRegistersFree());
			
			// TODO make the return value works when its a struct
			reg = REG_NOTUSED;
			
			if (!funcType->getReturnType()->isVoid() > 0) {
				// get the return value
				reg = allocatePRRegister();
				Register r = allocatePRRegister();
				
				unsigned int retSize = funcType->getReturnType()->getSize();
				assert(retSize > 0);
				
				// here we can't use stackPop since it would mess with stack base,
				// since it was pushed by the callee
				addInstruction(new LoadInstruction(reg, REG_SP, retSize, 0));
				addInstruction(new SetInstruction(r, retSize));
				addInstruction(new AddInstruction(REG_SP, REG_SP, r));
				
				deallocatePRRegister(r);
				// reg will be used later, when pushing the result (after removing the arguments)
			}
			
			// deallocate arguments from the stack
			context.deallocateStack(stackMem);
			
			deallocateExpResult(result);
			
			if (funcType->getReturnType()->getSize() > 0) {
				stackPush(reg, funcType->getReturnType()->getSize());
				deallocateRegister(reg);
				
				result.setResultType(ExpResult::STACKED);
				result.setType(funcType->getReturnType());
				result.setOffsetFromBase(context);
				result.setNeedDealocate(true);
				result.setDynamic(false);
			}
			else result.setResultType(ExpResult::VOID);
			
			break;
		}
		case 4: // <POSTFIX_EXPRESSION> ::= <POSTFIX_EXPRESSION> DOT IDENTIFIER
			// TODO
			abort();
			break;
		case 5: // <POSTFIX_EXPRESSION> ::= <POSTFIX_EXPRESSION> PTR_OP IDENTIFIER
			// TODO
			abort();
			break;
		case 6: // <POSTFIX_EXPRESSION> ::= <POSTFIX_EXPRESSION> INC_OP
			result = parsePostfixExp(nt->getNonTerminalAt(0));
			checkVoidExp(result, nt);
			
			if (result.getResultType() == ExpResult::CONSTANT || result.isDynamic()) {
				unsigned int increment = result.getType()->getIncrement();
				if (!increment) throw ParserError(nt->getInputLocation(), "Invalid unary operand.");
				
				Register inc = allocateConstant(increment);
				Register val = result.getValue(context);
				
				addInstruction(new AddInstruction(inc, val, inc));
				
				Register pos = result.getPointer(context);
				addInstruction(new StoreInstruction(inc, pos, result.getType()->getSize(), 0));
				
				deallocateRegister(pos);
				deallocateRegister(inc);
				
				deallocateExpResult(result);
				
				stackPush(val, result.getType()->getSize());
				
				deallocateRegister(val);
				
				result.setOffsetFromBase(context);
				result.setNeedDealocate(true);
				result.setDynamic(false);
			}
			else throw ParserError(nt->getInputLocation(), "Invalid unary operand.");
			
			break;
		case 7: // <POSTFIX_EXPRESSION> ::= <POSTFIX_EXPRESSION> DEC_OP
			result = parsePostfixExp(nt->getNonTerminalAt(0));
			checkVoidExp(result, nt);
			
			if (result.getResultType() == ExpResult::CONSTANT || result.isDynamic()) {
				unsigned int increment = result.getType()->getIncrement();
				if (!increment) throw ParserError(nt->getInputLocation(), "Invalid unary operand.");
				
				Register inc = allocateConstant(increment);
				Register val = result.getValue(context);
				
				addInstruction(new SubInstruction(inc, val, inc));
				
				Register pos = result.getPointer(context);
				addInstruction(new StoreInstruction(inc, pos, result.getType()->getSize(), 0));
				
				deallocateRegister(pos);
				deallocateRegister(inc);
				
				deallocateExpResult(result);
				
				stackPush(val, result.getType()->getSize());
				
				deallocateRegister(val);
				
				result.setOffsetFromBase(context);
				result.setNeedDealocate(true);
				result.setDynamic(false);
			}
			else throw ParserError(nt->getInputLocation(), "Invalid unary operand.");
			
			break;
		default:
			abort();
	}
	
	return result;
}

/*
 * <UNARY_EXPRESSION> ::= <POSTFIX_EXPRESSION>
 *		| INC_OP <UNARY_EXPRESSION>
 *		| DEC_OP <UNARY_EXPRESSION>
 *		| <UNARY_OPERATOR> <CAST_EXPRESSION>
 *		| SIZEOF <UNARY_EXPRESSION>
 *		| SIZEOF P_OPEN <TYPE_NAME> P_CLOSE
 *		;
 */
ExpResult CParser::parseUnaryExp(NonTerminal *nt) {
	assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_UNARY_EXPRESSION);
	
	ExpResult result;
	
	switch (nt->getNonTerminalRule()) {
		case 0: // <UNARY_EXPRESSION> ::= <POSTFIX_EXPRESSION>
			result = parsePostfixExp(nt->getNonTerminalAt(0));
			break;
		case 1: // <UNARY_EXPRESSION> ::=  INC_OP <UNARY_EXPRESSION>
			result = parseUnaryExp(nt->getNonTerminalAt(1));
			checkVoidExp(result, nt->getNonTerminalAt(1));
			
			if (result.getResultType() != ExpResult::CONSTANT) {
				unsigned int increment = result.getType()->getIncrement();
				if (!increment) throw ParserError(nt->getInputLocation(), "Invalid unary operator.");
				
				Register val = result.getValue(context);
				Register inc = allocatePRRegister();
				
				addInstruction(new SetInstruction(inc, increment));
				addInstruction(new AddInstruction(val, val, inc));
				deallocateRegister(inc);
				
				Register pos = result.getPointer(context);
				
				addInstruction(new StoreInstruction(val, pos, result.getType()->getSize(), 0));
				
				deallocateRegister(val);
				deallocateRegister(inc);
			}
			else throw ParserError(nt->getInputLocation(), "Cannot increment constant.");
			
			break;
		case 2: // <UNARY_EXPRESSION> ::= DEC_OP <UNARY_EXPRESSION>
			result = parseUnaryExp(nt->getNonTerminalAt(1));
			checkVoidExp(result, nt->getNonTerminalAt(1));
			
			if (result.getResultType() != ExpResult::CONSTANT) {
				unsigned int increment = result.getType()->getIncrement();
				if (!increment) throw ParserError(nt->getInputLocation(), "Invalid unary operator.");
				
				Register val = result.getValue(context);
				Register inc = allocatePRRegister();
				
				addInstruction(new SetInstruction(inc, increment));
				addInstruction(new SubInstruction(val, val, inc));
				deallocateRegister(inc);
				
				Register pos = result.getPointer(context);
				
				addInstruction(new StoreInstruction(val, pos, result.getType()->getSize(), 0));
				
				deallocateRegister(val);
				deallocateRegister(inc);
			}
			else throw ParserError(nt->getInputLocation(), "Cannot decrement constant.");
			
			break;
		case 3: // <UNARY_EXPRESSION> ::= <UNARY_OPERATOR> <CAST_EXPRESSION>
			result = parseCastExp(nt->getNonTerminalAt(1));
			parseUnaryOperator(nt->getNonTerminalAt(0), result);
			break;
		case 4: // <UNARY_EXPRESSION> ::= SIZEOF <UNARY_EXPRESSION>
		{
			result = parseCastExp(nt->getNonTerminalAt(1));
			unsigned int size = result.getType()->getSize();
			deallocateExpResult(result);
			
			result.setResultType(ExpResult::CONSTANT);
			result.setType(new PrimitiveType<unsigned int>());
			result.setConstant(size);
			
			break;
		}
		case 5: // <UNARY_EXPRESSION> ::= SIZEOF P_OPEN <TYPE_NAME> P_CLOSE
		{
			Pointer<Type> type = parseTypeName(nt->getNonTerminalAt(2));
			
			result.setResultType(ExpResult::CONSTANT);
			result.setType(new PrimitiveType<unsigned int>());
			result.setConstant(type->getSize());
			
			break;
		}
		default:
			abort();
	}
	
	return result;
}

/*
 * <UNARY_OPERATOR> ::= AND
 *		| MUL
 *		| PLUS_SIG
 *		| LESS_SIG
 *		| NEG
 *		| NOT
 *		;
 */
void CParser::parseUnaryOperator(NonTerminal *nt, ExpResult & exp) {
	assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_UNARY_OPERATOR);
	
	checkVoidExp(exp, nt);
	
	switch (nt->getNonTerminalRule()) {
		case 0: // <UNARY_OPERATOR> ::= AND
		{
			if (exp.getResultType() == ExpResult::CONSTANT) {
				throw ParserError(nt->getInputLocation(), "Cannot get address from constant.");
			}
			
			Pointer<Type> type = new PointerType(exp.getType());
			Register addr = exp.getPointer(context);
			
			if (type->getSize() == exp.getType()->getSize()) {
				addInstruction(new StoreInstruction(addr, addr, REGISTER_SIZE, 0));
			}
			else {
				deallocateExpResult(exp);
				stackPush(addr);
				exp.setOffsetFromBase(context);
			}
			
			deallocateRegister(addr);
			
			exp.setType(type);
			exp.setDynamic(false);
			
			break;
		}
		case 1: // <UNARY_OPERATOR> ::= MUL
		{
			if (exp.getResultType() == ExpResult::CONSTANT) {
				throw ParserError(nt->getInputLocation(), "Cannot dereference constant.");
			}
			
			Pointer<Type> type = exp.getType();
			if (type.dynamicCast<PointerType>()) {
				type = type.staticCast<PointerType>()->dereference();
			}
			else if (type.dynamicCast<ArrayType>()) {
				type = type.staticCast<ArrayType>()->dereference();
			}
			else throw ParserError(nt->getInputLocation(), "Cannot dereference non pointer.");
			
			if (type->getSize() == 0) throw ParserError(nt->getInputLocation(), "Cannot dereference void pointer.");
			
			if (!exp.isDynamic()) exp.setDynamic(true);
			else {
				if (type->fitRegister()) {
					// the value is the address
					Register pos = exp.getValue(context);
					
					if (type->getSize() == exp.getType()->getSize()) {
						unsigned int off = exp.getSPOffset(context);
						addInstruction(new StoreInstruction(pos, REG_SP, type->getSize(), off));
					}
					else {
						deallocateExpResult(exp);
						stackPush(pos);
						exp.setOffsetFromBase(context);
					}
					
					deallocateRegister(pos);
					exp.setDynamic(true);
				}
				else {
					// TODO structs
					abort();
				}
			}
			
			exp.setType(type);
			
			break;
		}
		case 2: // <UNARY_OPERATOR> ::= PLUS_SIG
			// ignore
			break;
		case 3: // <UNARY_OPERATOR> ::= LESS_SIG
			if (exp.getResultType() == ExpResult::CONSTANT) {
				exp.setConstant(-exp.getConstant());
			}
			else {
				Register pos = exp.getPointer(context);
				Register val = exp.getValue(context);
				
				addInstruction(new SubInstruction(val, REG_ZERO, val));
				addInstruction(new StoreInstruction(val, pos, exp.getType()->getSize(), 0));
				
				deallocateRegister(pos);
				deallocateRegister(val);
			}
			break;
		case 4: // <UNARY_OPERATOR> ::= NEG
			if (exp.getResultType() == ExpResult::CONSTANT) {
				exp.setConstant(~exp.getConstant());
			}
			else {
				Register val = exp.getValue(context);
				Register neg = allocatePRRegister();
				
				addInstruction(new SetInstruction(neg, -1)); // set all bits to 1
				addInstruction(new XorInstruction(val, val, neg));
				deallocateRegister(neg);
				
				Register pos = exp.getPointer(context);
				addInstruction(new StoreInstruction(val, pos, exp.getType()->getSize(), 0));
				
				deallocateRegister(pos);
				deallocateRegister(val);
			}
			break;
		case 5: // <UNARY_OPERATOR> ::= NOT
			if (exp.getResultType() == ExpResult::CONSTANT) {
				exp.setConstant(!exp.getConstant());
			}
			else {
				Register val = exp.getValue(context);
				Register pos = exp.getPointer(context);
				
				addInstruction(new NotInstruction(val, val));
				addInstruction(new StoreInstruction(val, pos, exp.getType()->getSize(), 0));
				
				deallocateRegister(pos);
				deallocateRegister(val);
			}
			break;
		default:
			abort();
	}
}

/*
 * <CAST_EXPRESSION> ::= <UNARY_EXPRESSION>
 *		| P_OPEN <TYPE_NAME> P_CLOSE <CAST_EXPRESSION>
 *		;
 */
ExpResult CParser::parseCastExp(NonTerminal *nt) {
	assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_CAST_EXPRESSION);
	
	ExpResult result;
	
	if (nt->getNonTerminalRule() == 0) { // <CAST_EXPRESSION> ::= <UNARY_EXPRESSION>
		result = parseUnaryExp(nt->getNonTerminalAt(0));
	}
	else { // <CAST_EXPRESSION> ::= P_OPEN <TYPE_NAME> P_CLOSE <CAST_EXPRESSION>
		assert(nt->getNonTerminalRule() == 1);
		
		Pointer<Type> type = parseTypeName(nt->getNonTerminalAt(1));
		result = parseCastExp(nt->getNonTerminalAt(3));
		
		if (!result.getType()->allowExplicitCastTo(type)) {
			throw ParserError(nt->getInputLocation(), "Invalid cast.");
		}
		
		if (result.getResultType() == ExpResult::CONSTANT) {
			const Number & val = result.getConstant();
			if (type->isFloatingPoint()) {
				result.setConstant(Number(Number::FLOAT, val.floatValue()));
			}
			else result.setConstant(Number(Number::INT, val.intValue()));
		}
		else {
			assert(result.getResultType() == ExpResult::STACKED);
			
			Register val = result.getValue(context);
			
			bool forceRestack = false;
			
			if (result.getType()->isFloatingPoint() ^ type->isFloatingPoint()) {
				Register target;
				if (type->isFloatingPoint()) target = allocateFPRegister();
				else target = allocatePRRegister();
				
				// arithmetic instructions treat double/integer casting
				addInstruction(new AddInstruction(target, val, REG_ZERO));
				
				deallocateRegister(val);
				val = target;
				
				// now we need to store the value in the stack, even if it has the same size
				forceRestack = true;
			}
			
			if (type->getSize() != result.getType()->getSize() || forceRestack) {
				deallocateExpResult(result);
				
				// TODO cast double/float
				stackPush(val, type->getSize());
				
				result.setOffsetFromBase(context);
				result.setNeedDealocate(true);
				result.setDynamic(false);
				
			}
			
			deallocateRegister(val);
		}
		
		result.setType(type);
	}
	
	return result;
}

/*
 * <MULTIPLICATIVE_EXPRESSION> ::= <CAST_EXPRESSION>
 *		| <MULTIPLICATIVE_EXPRESSION> MUL <CAST_EXPRESSION>
 *		| <MULTIPLICATIVE_EXPRESSION> DIV <CAST_EXPRESSION>
 *		| <MULTIPLICATIVE_EXPRESSION> MOD <CAST_EXPRESSION>
 *		;
 */
ExpResult CParser::parseMultiplicativeExp(NonTerminal *nt) {
	assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_MULTIPLICATIVE_EXPRESSION);
	
	ExpResult result;
	
	switch (nt->getNonTerminalRule()) {
		case 0: // <MULTIPLICATIVE_EXPRESSION> ::= <CAST_EXPRESSION>
			result = parseCastExp(nt->getNonTerminalAt(0));
			break;
		case 1: // <MULTIPLICATIVE_EXPRESSION> ::= <MULTIPLICATIVE_EXPRESSION> MUL <CAST_EXPRESSION>
		{
			result = parseMultiplicativeExp(nt->getNonTerminalAt(0));
			ExpResult value = parseCastExp(nt->getNonTerminalAt(2));
			
			if (result.getResultType() == ExpResult::CONSTANT
					&& value.getResultType() == ExpResult::CONSTANT) {
				result.setResultType(ExpResult::CONSTANT);
				result.setType(Type::getResultingType(result.getType(), value.getType()));
				result.setConstant(result.getConstant() * value.getConstant());
			}
			else {
				Register r = result.getValue(context);
				Register val = value.getValue(context);
				
				deallocateExpResult(result);
				deallocateExpResult(value);
				
				addInstruction(new MulInstruction(r, r, val));
				
				Pointer<Type> type = Type::getResultingType(result.getType(), value.getType());
				
				stackPush(r, type->getSize());
				
				deallocateRegister(r);
				deallocateRegister(val);
				
				result.setResultType(ExpResult::STACKED);
				result.setType(type);
				result.setOffsetFromBase(context);
				result.setNeedDealocate(true);
				result.setDynamic(false);
			}
			
			break;
		}
		case 2: // <MULTIPLICATIVE_EXPRESSION> ::= <MULTIPLICATIVE_EXPRESSION> DIV <CAST_EXPRESSION>
		{
			result = parseMultiplicativeExp(nt->getNonTerminalAt(0));
			ExpResult value = parseCastExp(nt->getNonTerminalAt(2));
			
			if (result.getResultType() == ExpResult::CONSTANT
					&& value.getResultType() == ExpResult::CONSTANT) {
				result.setResultType(ExpResult::CONSTANT);
				result.setType(Type::getResultingType(result.getType(), value.getType()));
				result.setConstant(result.getConstant() / value.getConstant());
			}
			else {
				Register r = result.getValue(context);
				Register val = value.getValue(context);
				
				deallocateExpResult(result);
				deallocateExpResult(value);
				
				addInstruction(new DivInstruction(r, r, val));
				
				Pointer<Type> type = Type::getResultingType(result.getType(), value.getType());
				
				stackPush(r, type->getSize());
				
				deallocateRegister(r);
				deallocateRegister(val);
				
				result.setResultType(ExpResult::STACKED);
				result.setType(type);
				result.setOffsetFromBase(context);
				result.setNeedDealocate(true);
				result.setDynamic(false);
			}
			
			break;
		}
		case 3: // <MULTIPLICATIVE_EXPRESSION> ::= <MULTIPLICATIVE_EXPRESSION> MOD <CAST_EXPRESSION>
		{
			result = parseMultiplicativeExp(nt->getNonTerminalAt(0));
			ExpResult value = parseCastExp(nt->getNonTerminalAt(2));
			
			if (result.getType()->isFloatingPoint() || value.getType()->isFloatingPoint()) {
				throw ParserError("Cannot use operator \'%\' for floating point.");
			}
			
			if (result.getResultType() == ExpResult::CONSTANT
					&& value.getResultType() == ExpResult::CONSTANT) {
				result.setResultType(ExpResult::CONSTANT);
				result.setType(Type::getResultingType(result.getType(), value.getType()));
				result.setConstant(result.getConstant() % value.getConstant());
			}
			else {
				Register r = result.getValue(context);
				Register val = value.getValue(context);
				
				assert(RegisterUtils::isProgrammerRegister(r));
				assert(RegisterUtils::isProgrammerRegister(val));
				
				deallocateExpResult(result);
				deallocateExpResult(value);
				
				addInstruction(new ModInstruction(r, r, val));
				
				Pointer<Type> type = Type::getResultingType(result.getType(), value.getType());
				
				stackPush(r, type->getSize());
				
				deallocateRegister(r);
				deallocateRegister(val);
				
				result.setResultType(ExpResult::STACKED);
				result.setType(type);
				result.setOffsetFromBase(context);
				result.setNeedDealocate(true);
				result.setDynamic(false);
			}
			
			break;
		}
		default:
			abort();
	}
	
	return result;
}

/*
 * <ADDITIVE_EXPRESSION> ::= <MULTIPLICATIVE_EXPRESSION>
 *		| <ADDITIVE_EXPRESSION> PLUS_SIG <MULTIPLICATIVE_EXPRESSION>
 *		| <ADDITIVE_EXPRESSION> LESS_SIG <MULTIPLICATIVE_EXPRESSION>
 *		;
 */
ExpResult CParser::parseAdditiveExp(NonTerminal *nt) {
	assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_ADDITIVE_EXPRESSION);
	
	ExpResult result;
	
	switch (nt->getNonTerminalRule()) {
		case 0: // <ADDITIVE_EXPRESSION> ::= <MULTIPLICATIVE_EXPRESSION>
			result = parseMultiplicativeExp(nt->getNonTerminalAt(0));
			break;
		case 1: // <ADDITIVE_EXPRESSION> ::= <ADDITIVE_EXPRESSION> PLUS_SIG <MULTIPLICATIVE_EXPRESSION>
		{
			result = parseAdditiveExp(nt->getNonTerminalAt(0));
			ExpResult value = parseMultiplicativeExp(nt->getNonTerminalAt(2));
			
			if (result.getResultType() == ExpResult::CONSTANT
					&& value.getResultType() == ExpResult::CONSTANT) {
				result.setResultType(ExpResult::CONSTANT);
				result.setType(Type::getResultingType(result.getType(), value.getType()));
				result.setConstant(result.getConstant() + value.getConstant());
			}
			else {
				Register r = result.getValue(context);
				Register val = value.getValue(context);
				
				deallocateExpResult(result);
				deallocateExpResult(value);
				
				addInstruction(new AddInstruction(r, r, val));
				
				Pointer<Type> type = Type::getResultingType(result.getType(), value.getType());
				
				stackPush(r, type->getSize());
				
				deallocateRegister(r);
				deallocateRegister(val);
				
				result.setResultType(ExpResult::STACKED);
				result.setType(type);
				result.setOffsetFromBase(context);
				result.setNeedDealocate(true);
				result.setDynamic(false);
			}
			
			break;
		}
		case 2: // <ADDITIVE_EXPRESSION> ::= <ADDITIVE_EXPRESSION> LESS_SIG <MULTIPLICATIVE_EXPRESSION>
		{
			result = parseAdditiveExp(nt->getNonTerminalAt(0));
			ExpResult value = parseMultiplicativeExp(nt->getNonTerminalAt(2));
			
			if (result.getResultType() == ExpResult::CONSTANT
					&& value.getResultType() == ExpResult::CONSTANT) {
				result.setResultType(ExpResult::CONSTANT);
				result.setType(Type::getResultingType(result.getType(), value.getType()));
				result.setConstant(result.getConstant() - value.getConstant());
			}
			else {
				Register r = result.getValue(context);
				Register val = value.getValue(context);
				
				deallocateExpResult(result);
				deallocateExpResult(value);
				
				addInstruction(new SubInstruction(r, r, val));
				
				Pointer<Type> type = Type::getResultingType(result.getType(), value.getType());
				
				stackPush(r, type->getSize());
				
				deallocateRegister(r);
				deallocateRegister(val);
				
				result.setResultType(ExpResult::STACKED);
				result.setType(type);
				result.setOffsetFromBase(context);
				result.setNeedDealocate(true);
				result.setDynamic(false);
			}
			
			break;
		}
		default:
			abort();
	}
	
	return result;
}

/*
 * <SHIFT_EXPRESSION> ::= <ADDITIVE_EXPRESSION>
 *		| <SHIFT_EXPRESSION> LEFT_OP <ADDITIVE_EXPRESSION>
 *		| <SHIFT_EXPRESSION> RIGHT_OP <ADDITIVE_EXPRESSION>
 *		;
 */
ExpResult CParser::parseShiftExp(NonTerminal *nt) {
	assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_SHIFT_EXPRESSION);
	
	ExpResult result;
	
	// <SHIFT_EXPRESSION> ::= <ADDITIVE_EXPRESSION>
	if (nt->getNonTerminalRule() == 0) return parseAdditiveExp(nt->getNonTerminalAt(0));
	
	ExpResult value = parseShiftExp(nt->getNonTerminalAt(0));
	ExpResult shift = parseAdditiveExp(nt->getNonTerminalAt(2));
	
	if (value.getResultType() == ExpResult::CONSTANT
			&& shift.getResultType() == ExpResult::CONSTANT) {
		
		const Number & val = value.getConstant();
		const Number & sh = shift.getConstant();
		
		if (val.isFloat() || sh.isFloat()) {
			throw ParserError(nt->getTokenAt(1)->getInputLocation(), "Cannot shift floating point value.");
		}
		
		result.setResultType(ExpResult::CONSTANT);
		result.setType(value.getType());
		
		if (nt->getNonTerminalRule() == 1) { // <SHIFT_EXPRESSION> ::= <SHIFT_EXPRESSION> LEFT_OP <ADDITIVE_EXPRESSION>
			result.setConstant(val << sh.intValue());
		}
		else { // <SHIFT_EXPRESSION> ::= <SHIFT_EXPRESSION> RIGHT_OP <ADDITIVE_EXPRESSION>
			result.setConstant(val >> sh.intValue());
		}
	}
	else {
		if (value.getType()->getTypeEnum() == Type::STRUCT
				|| value.getType()->getTypeEnum() == Type::UNION
				|| shift.getType()->getTypeEnum() == Type::STRUCT
				|| shift.getType()->getTypeEnum() == Type::UNION) {
			throw ParserError(nt->getInputLocation(), "Invalid binary operator.");
		}
		
		if (value.getType()->isFloatingPoint() || shift.getType()->isFloatingPoint()) {
			throw ParserError(nt->getInputLocation(), "Cannot shift floating point value.");
		}
		
		Register val = value.getValue(context);
		Register sh = shift.getValue(context);
		
		deallocateExpResult(value);
		deallocateExpResult(shift);
		
		if (nt->getNonTerminalRule() == 1) { // <SHIFT_EXPRESSION> ::= <SHIFT_EXPRESSION> LEFT_OP <ADDITIVE_EXPRESSION>
			addInstruction(new ShiftLeftInstruction(val, val, sh));
		}
		else { // <SHIFT_EXPRESSION> ::= <SHIFT_EXPRESSION> RIGHT_OP <ADDITIVE_EXPRESSION>
			addInstruction(new ShiftRightInstruction(val, val, sh));
		}
		
		Pointer<Type> type = value.getType();
		
		stackPush(val, type->getSize());
		
		deallocateRegister(val);
		deallocateRegister(sh);
		
		result.setResultType(ExpResult::STACKED);
		result.setType(type);
		result.setOffsetFromBase(context);
		result.setNeedDealocate(true);
	}
	
	return result;
}

/*
 * <RELATIONAL_EXPRESSION> ::= <SHIFT_EXPRESSION>
 *		| <RELATIONAL_EXPRESSION> LESS <SHIFT_EXPRESSION>
 *		| <RELATIONAL_EXPRESSION> GREATER <SHIFT_EXPRESSION>
 *		| <RELATIONAL_EXPRESSION> LE_OP <SHIFT_EXPRESSION>
 *		| <RELATIONAL_EXPRESSION> GE_OP <SHIFT_EXPRESSION>
 *		;
 */
ExpResult CParser::parseRelationalExp(NonTerminal *nt) {
	assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_RELATIONAL_EXPRESSION);
	
	ExpResult result;
	
	switch (nt->getNonTerminalRule()) {
		case 0: // <RELATIONAL_EXPRESSION> ::= <SHIFT_EXPRESSION>
			result = parseShiftExp(nt->getNonTerminalAt(0));
			break;
		case 1: // <RELATIONAL_EXPRESSION> ::= <RELATIONAL_EXPRESSION> LESS <SHIFT_EXPRESSION>
		case 2: // <RELATIONAL_EXPRESSION> ::= <RELATIONAL_EXPRESSION> GREATER <SHIFT_EXPRESSION>
		case 3: // <RELATIONAL_EXPRESSION> ::= <RELATIONAL_EXPRESSION> LE_OP <SHIFT_EXPRESSION>
		case 4: // <RELATIONAL_EXPRESSION> ::= <RELATIONAL_EXPRESSION> GE_OP <SHIFT_EXPRESSION>
		{
			ExpResult valueA = parseRelationalExp(nt->getNonTerminalAt(0));
			ExpResult valueB = parseShiftExp(nt->getNonTerminalAt(2));
			
			if (valueA.getResultType() == ExpResult::CONSTANT
					&& valueB.getResultType() == ExpResult::CONSTANT) {
				result.setResultType(ExpResult::CONSTANT);
				result.setType(new PrimitiveType<bool>());
				
				const Number & valA = valueA.getConstant();
				const Number & valB = valueB.getConstant();
				
				switch (nt->getNonTerminalRule()) {
					case 1: // <RELATIONAL_EXPRESSION> ::= <RELATIONAL_EXPRESSION> LESS <SHIFT_EXPRESSION>
						result.setConstant(valA < valB);
						break;
					case 2: // <RELATIONAL_EXPRESSION> ::= <RELATIONAL_EXPRESSION> GREATER <SHIFT_EXPRESSION>
						result.setConstant(valA > valB);
						break;
					case 3: // <RELATIONAL_EXPRESSION> ::= <RELATIONAL_EXPRESSION> LE_OP <SHIFT_EXPRESSION>
						result.setConstant(valA <= valB);
						break;
					case 4: // <RELATIONAL_EXPRESSION> ::= <RELATIONAL_EXPRESSION> GE_OP <SHIFT_EXPRESSION>
						result.setConstant(valA >= valB);
						break;
					default:
						abort();
				}
			}
			else {
				if (valueA.getType()->getTypeEnum() == Type::STRUCT
						|| valueA.getType()->getTypeEnum() == Type::UNION
						|| valueB.getType()->getTypeEnum() == Type::STRUCT
						|| valueB.getType()->getTypeEnum() == Type::UNION) {
					throw ParserError(nt->getTokenAt(1)->getInputLocation(), "Invalid binary operator.");
				}
				
				Register valA = valueA.getValue(context);
				Register valB = valueB.getValue(context);
				
				deallocateExpResult(valueA);
				deallocateExpResult(valueB);
				
				switch (nt->getNonTerminalRule()) {
					case 1: // <RELATIONAL_EXPRESSION> ::= <RELATIONAL_EXPRESSION> LESS <SHIFT_EXPRESSION>
						addInstruction(new LessCmpInstruction(valA, valA, valB));
						break;
					case 2: // <RELATIONAL_EXPRESSION> ::= <RELATIONAL_EXPRESSION> GREATER <SHIFT_EXPRESSION>
						addInstruction(new LessCmpInstruction(valA, valB, valA));
						break;
					case 3: // <RELATIONAL_EXPRESSION> ::= <RELATIONAL_EXPRESSION> LE_OP <SHIFT_EXPRESSION>
						addInstruction(new LessCmpInstruction(valA, valB, valA));
						addInstruction(new NotInstruction(valA, valA));
						break;
					case 4: // <RELATIONAL_EXPRESSION> ::= <RELATIONAL_EXPRESSION> GE_OP <SHIFT_EXPRESSION>
						addInstruction(new LessCmpInstruction(valA, valA, valB));
						addInstruction(new NotInstruction(valA, valA));
						break;
					default:
						abort();
				}
				
				Pointer<Type> type = new PrimitiveType<bool>();
				
				stackPush(valA, type->getSize());
				
				deallocateRegister(valA);
				deallocateRegister(valB);
				
				result.setResultType(ExpResult::STACKED);
				result.setType(type);
				result.setOffsetFromBase(context);
				result.setNeedDealocate(true);
			}
			
			break;
		}
		default:
			abort();
	}
	
	return result;
}

/*
 * <EQUALITY_EXPRESSION> ::= <RELATIONAL_EXPRESSION>
 *		| <EQUALITY_EXPRESSION> EQ_OP <RELATIONAL_EXPRESSION>
 *		| <EQUALITY_EXPRESSION> NE_OP <RELATIONAL_EXPRESSION>
 *		;
 */
ExpResult CParser::parseEqualityExp(NonTerminal *nt) {
	assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_EQUALITY_EXPRESSION);
	
	ExpResult result;
	
	switch (nt->getNonTerminalRule()) {
		case 0: // <EQUALITY_EXPRESSION> ::= <RELATIONAL_EXPRESSION>
			result = parseRelationalExp(nt->getNonTerminalAt(0));
			break;
		case 1: // <EQUALITY_EXPRESSION> ::= <EQUALITY_EXPRESSION> EQ_OP <RELATIONAL_EXPRESSION>
		{
			ExpResult valueA = parseEqualityExp(nt->getNonTerminalAt(0));
			ExpResult valueB = parseRelationalExp(nt->getNonTerminalAt(2));
			
			if (valueA.getResultType() == ExpResult::CONSTANT
					&& valueB.getResultType() == ExpResult::CONSTANT) {
				result.setResultType(ExpResult::CONSTANT);
				result.setType(new PrimitiveType<bool>());
				result.setConstant(Number(valueA.getConstant() == valueB.getConstant()));
			}
			else {
				// TODO compare structs
				Register valA = valueA.getValue(context);
				Register valB = valueB.getValue(context);
				
				deallocateExpResult(valueA);
				deallocateExpResult(valueB);
				
				addInstruction(new EqualCmpInstruction(valA, valA, valB));
				
				Pointer<Type> type = Type::getResultingType(valueA.getType(), valueB.getType());
				
				stackPush(valA, type->getSize());
				
				deallocateRegister(valA);
				deallocateRegister(valB);
				
				result.setResultType(ExpResult::STACKED);
				result.setType(type);
				result.setOffsetFromBase(context);
				result.setNeedDealocate(true);
			}
			
			break;
		}
		case 2: // <EQUALITY_EXPRESSION> ::= <EQUALITY_EXPRESSION> NE_OP <RELATIONAL_EXPRESSION>
		{
			ExpResult valueA = parseEqualityExp(nt->getNonTerminalAt(0));
			ExpResult valueB = parseRelationalExp(nt->getNonTerminalAt(2));
			
			if (valueA.getResultType() == ExpResult::CONSTANT
					&& valueB.getResultType() == ExpResult::CONSTANT) {
				result.setResultType(ExpResult::CONSTANT);
				result.setType(new PrimitiveType<bool>());
				result.setConstant(Number(valueA.getConstant() != valueB.getConstant()));
			}
			else {
				// TODO compare structs
				Register valA = valueA.getValue(context);
				Register valB = valueB.getValue(context);
				
				deallocateExpResult(valueA);
				deallocateExpResult(valueB);
				
				addInstruction(new NotEqualCmpInstruction(valA, valA, valB));
				
				Pointer<Type> type = Type::getResultingType(valueA.getType(), valueB.getType());
				
				stackPush(valA, type->getSize());
				
				deallocateRegister(valA);
				deallocateRegister(valB);
				
				result.setResultType(ExpResult::STACKED);
				result.setType(type);
				result.setOffsetFromBase(context);
				result.setNeedDealocate(true);
			}
			
			break;
		}
	}
	
	return result;
}

/*
 * <AND_EXPRESSION> ::= <EQUALITY_EXPRESSION>
 *		| <AND_EXPRESSION> AND <EQUALITY_EXPRESSION>
 *		;
 */
ExpResult CParser::parseAndExp(NonTerminal *nt) {
	assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_AND_EXPRESSION);
	
	ExpResult result;
	
	if (nt->getNonTerminalRule() == 0) { // <AND_EXPRESSION> ::= <EQUALITY_EXPRESSION>
		result = parseEqualityExp(nt->getNonTerminalAt(0));
	}
	else {
		// <AND_EXPRESSION> ::= <AND_EXPRESSION> AND <EQUALITY_EXPRESSION>
		assert(nt->getNonTerminalRule() == 1);
		
		ExpResult valueA = parseLogicalOrExp(nt->getNonTerminalAt(0));
		checkVoidExp(valueA, nt->getNonTerminalAt(0));
		ExpResult valueB = parseLogicalAndExp(nt->getNonTerminalAt(2));
		checkVoidExp(valueB, nt->getNonTerminalAt(2));
		
		if (valueA.getResultType() == ExpResult::CONSTANT
				&& valueB.getResultType() == ExpResult::CONSTANT) {
			result.setResultType(ExpResult::CONSTANT);
			result.setType(Type::getResultingType(valueA.getType(), valueB.getType()));
			result.setConstant(Number(valueA.getConstant() & valueB.getConstant()));
		}
		else {
			Register valA = valueA.getValue(context);
			Register valB = valueB.getValue(context);
			
			deallocateExpResult(valueA);
			deallocateExpResult(valueB);
			
			addInstruction(new AndInstruction(valA, valA, valB));
			
			Pointer<Type> type = Type::getResultingType(valueA.getType(), valueB.getType());
			
			stackPush(valA, type->getSize());
			
			deallocateRegister(valA);
			deallocateRegister(valB);
			
			result.setResultType(ExpResult::STACKED);
			result.setType(type);
			result.setOffsetFromBase(context);
			result.setNeedDealocate(true);
		}
	}
	
	return result;
}

/*
 * <EXCLUSIVE_OR_EXPRESSION> ::= <AND_EXPRESSION>
 *		| <EXCLUSIVE_OR_EXPRESSION> XOR <AND_EXPRESSION>
 *		;
 */
ExpResult CParser::parseExclusiveOrExp(NonTerminal *nt) {
	assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_EXCLUSIVE_OR_EXPRESSION);
	
	ExpResult result;
	
	if (nt->getNonTerminalRule() == 0) { // <EXCLUSIVE_OR_EXPRESSION> ::= <AND_EXPRESSION>
		result = parseAndExp(nt->getNonTerminalAt(0));
	}
	else {
		// <EXCLUSIVE_OR_EXPRESSION> ::= <EXCLUSIVE_OR_EXPRESSION> XOR <AND_EXPRESSION>
		assert(nt->getNonTerminalRule() == 1);
		
		ExpResult valueA = parseLogicalOrExp(nt->getNonTerminalAt(0));
		checkVoidExp(valueA, nt->getNonTerminalAt(0));
		ExpResult valueB = parseLogicalAndExp(nt->getNonTerminalAt(2));
		checkVoidExp(valueB, nt->getNonTerminalAt(2));
		
		if (valueA.getResultType() == ExpResult::CONSTANT
				&& valueB.getResultType() == ExpResult::CONSTANT) {
			result.setResultType(ExpResult::CONSTANT);
			result.setType(Type::getResultingType(valueA.getType(), valueB.getType()));
			result.setConstant(Number(valueA.getConstant() ^ valueB.getConstant()));
		}
		else {
			Register valA = valueA.getValue(context);
			Register valB = valueB.getValue(context);
			
			deallocateExpResult(valueA);
			deallocateExpResult(valueB);
			
			addInstruction(new XorInstruction(valA, valA, valB));
			
			Pointer<Type> type = Type::getResultingType(valueA.getType(), valueB.getType());
			
			stackPush(valA, type->getSize());
			
			deallocateRegister(valA);
			deallocateRegister(valB);
			
			result.setResultType(ExpResult::STACKED);
			result.setType(type);
			result.setOffsetFromBase(context);
			result.setNeedDealocate(true);
		}
	}
	
	return result;
}

/*
 * <INCLUSIVE_OR_EXPRESSION> ::= <EXCLUSIVE_OR_EXPRESSION>
 *		| <INCLUSIVE_OR_EXPRESSION> OR <EXCLUSIVE_OR_EXPRESSION>
 *		;
 */
ExpResult CParser::parseInclusiveOrExp(NonTerminal *nt) {
	assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_INCLUSIVE_OR_EXPRESSION);
	
	ExpResult result;
	
	if (nt->getNonTerminalRule() == 0) { // <INCLUSIVE_OR_EXPRESSION> ::= <EXCLUSIVE_OR_EXPRESSION>
		result = parseExclusiveOrExp(nt->getNonTerminalAt(0));
	}
	else {
		// <INCLUSIVE_OR_EXPRESSION> ::= <INCLUSIVE_OR_EXPRESSION> OR <EXCLUSIVE_OR_EXPRESSION>
		assert(nt->getNonTerminalRule() == 1);
		
		ExpResult valueA = parseLogicalOrExp(nt->getNonTerminalAt(0));
		checkVoidExp(valueA, nt->getNonTerminalAt(0));
		ExpResult valueB = parseLogicalAndExp(nt->getNonTerminalAt(2));
		checkVoidExp(valueB, nt->getNonTerminalAt(2));
		
		if (valueA.getResultType() == ExpResult::CONSTANT
				&& valueB.getResultType() == ExpResult::CONSTANT) {
			result.setResultType(ExpResult::CONSTANT);
			result.setType(Type::getResultingType(valueA.getType(), valueB.getType()));
			result.setConstant(Number(valueA.getConstant() | valueB.getConstant()));
		}
		else {
			Register valA = valueA.getValue(context);
			Register valB = valueB.getValue(context);
			
			deallocateExpResult(valueA);
			deallocateExpResult(valueB);
			
			addInstruction(new OrInstruction(valA, valA, valB));
			
			Pointer<Type> type = Type::getResultingType(valueA.getType(), valueB.getType());
			
			stackPush(valA, type->getSize());
			
			deallocateRegister(valA);
			deallocateRegister(valB);
			
			result.setResultType(ExpResult::STACKED);
			result.setType(type);
			result.setOffsetFromBase(context);
			result.setNeedDealocate(true);
		}
	}
	
	return result;
}

/*
 * <LOGICAL_AND_EXPRESSION> ::= <INCLUSIVE_OR_EXPRESSION>
 *		| <LOGICAL_AND_EXPRESSION> AND_OP <INCLUSIVE_OR_EXPRESSION>
 *		;
 */
ExpResult CParser::parseLogicalAndExp(NonTerminal *nt) {
	assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_LOGICAL_AND_EXPRESSION);
	
	ExpResult result;
	
	if (nt->getNonTerminalRule() == 0) { // <LOGICAL_AND_EXPRESSION> ::= <INCLUSIVE_OR_EXPRESSION>
		result = parseInclusiveOrExp(nt->getNonTerminalAt(0));
	}
	else {
		// <LOGICAL_AND_EXPRESSION> ::= <LOGICAL_AND_EXPRESSION> AND_OP <INCLUSIVE_OR_EXPRESSION>
		assert(nt->getNonTerminalRule() == 1);
		
		ExpResult valueA = parseLogicalAndExp(nt->getNonTerminalAt(0));
		checkVoidExp(valueA, nt->getNonTerminalAt(0));
		ExpResult valueB = parseInclusiveOrExp(nt->getNonTerminalAt(2));
		checkVoidExp(valueB, nt->getNonTerminalAt(2));
		
		if (valueA.getResultType() == ExpResult::CONSTANT
				&& valueB.getResultType() == ExpResult::CONSTANT) {
			result.setResultType(ExpResult::CONSTANT);
			result.setType(new PrimitiveType<bool>());
			result.setConstant(Number(valueA.getConstant().boolValue()
					&& valueB.getConstant().boolValue()));
		}
		else {
			Register valA = valueA.getValue(context);
			Register valB = valueB.getValue(context);
			
			deallocateExpResult(valueA);
			deallocateExpResult(valueB);
			
			addInstruction(new LogicalAndInstruction(valA, valA, valB));
			
			Pointer<Type> type = new PrimitiveType<bool>();
			
			stackPush(valA, type->getSize());
			
			deallocateRegister(valA);
			deallocateRegister(valB);
			
			result.setResultType(ExpResult::STACKED);
			result.setType(type);
			result.setOffsetFromBase(context);
			result.setNeedDealocate(true);
		}
	}
	
	return result;
}

/*
 * <LOGICAL_OR_EXPRESSION> ::= <LOGICAL_AND_EXPRESSION>
 *		| <LOGICAL_OR_EXPRESSION> OR_OP <LOGICAL_AND_EXPRESSION>
 *		;
 */
ExpResult CParser::parseLogicalOrExp(NonTerminal *nt) {
	assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_LOGICAL_OR_EXPRESSION);
	
	ExpResult result;
	
	if (nt->getNonTerminalRule() == 0) { // <LOGICAL_OR_EXPRESSION> ::= <LOGICAL_AND_EXPRESSION>
		result = parseLogicalAndExp(nt->getNonTerminalAt(0));
	}
	else {
		// <LOGICAL_OR_EXPRESSION> ::= <LOGICAL_OR_EXPRESSION> OR_OP <LOGICAL_AND_EXPRESSION>
		assert(nt->getNonTerminalRule() == 1);
		
		ExpResult valueA = parseLogicalOrExp(nt->getNonTerminalAt(0));
		checkVoidExp(valueA, nt->getNonTerminalAt(0));
		ExpResult valueB = parseLogicalAndExp(nt->getNonTerminalAt(2));
		checkVoidExp(valueB, nt->getNonTerminalAt(2));
		
		if (valueA.getResultType() == ExpResult::CONSTANT
				&& valueB.getResultType() == ExpResult::CONSTANT) {
			result.setResultType(ExpResult::CONSTANT);
			result.setType(new PrimitiveType<bool>());
			result.setConstant(Number(valueA.getConstant().boolValue()
					|| valueB.getConstant().boolValue()));
		}
		else {
			Register valA = valueA.getValue(context);
			Register valB = valueB.getValue(context);
			
			deallocateExpResult(valueA);
			deallocateExpResult(valueB);
			
			addInstruction(new LogicalOrInstruction(valA, valA, valB));
			
			Pointer<Type> type = new PrimitiveType<bool>();
			
			stackPush(valA, type->getSize());
			
			deallocateRegister(valA);
			deallocateRegister(valB);
			
			result.setResultType(ExpResult::STACKED);
			result.setType(type);
			result.setOffsetFromBase(context);
			result.setNeedDealocate(true);
		}
	}
	
	return result;
}

/*
 * <CONDITIONAL_EXPRESSION> ::= <LOGICAL_OR_EXPRESSION>
 *		| <LOGICAL_OR_EXPRESSION> QUESTION <EXPRESSION> COLUMN <CONDITIONAL_EXPRESSION>
 *		;
 */
ExpResult CParser::parseConditionalExp(NonTerminal *nt) {
	assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_CONDITIONAL_EXPRESSION);
	
	ExpResult result;
	
	if (nt->getNonTerminalRule() == 0) { // <CONDITIONAL_EXPRESSION> ::= <LOGICAL_OR_EXPRESSION>
		result = parseLogicalOrExp(nt->getNonTerminalAt(0));
	}
	else { // <CONDITIONAL_EXPRESSION> ::= <LOGICAL_OR_EXPRESSION> QUESTION <EXPRESSION> COLUMN <CONDITIONAL_EXPRESSION>
		assert(nt->getNonTerminalRule() == 1);
		
		result = parseLogicalOrExp(nt->getNonTerminalAt(0));
		checkVoidExp(result, nt);
		
		if (result.getResultType() == ExpResult::CONSTANT) {
			bool r;
			if (result.getResultType() == ExpResult::CONSTANT) r = result.getConstant().boolValue();
			else r = true;
			
			deallocateExpResult(result);
			
			if (r) result = parseExp(nt->getNonTerminalAt(2));
			else result = parseConditionalExp(nt->getNonTerminalAt(4));
		}
		else {
			if (result.getType()->getTypeEnum() == Type::STRUCT
					|| result.getType()->getTypeEnum() == Type::UNION) {
				throw ParserError(nt->getInputLocation(), "Expecting conditional expression.");
			}
			
			// TODO fix this for types with different sizes in exp1 or exp2
			// TODO fix this for structs
			
			Register val = result.getValue(context);
			deallocateExpResult(result);
			BranchInstruction *branch = new BranchInstruction(val, 0);
			addInstruction(branch);
			deallocateRegister(val);
			
			unsigned int instructions = context.getCurrentScope()->getInstructionCount();
			
			// generate first the second expression
			// so the branch will point to the first expression
			ExpResult exp2 = parseConditionalExp(nt->getNonTerminalAt(4));
			JumpInstruction *jump = new JumpInstruction(0);
			
			branch->setTarget(context.getCurrentScope()->getInstructionCount() - instructions);
			instructions = context.getCurrentScope()->getInstructionCount();
			
			ExpResult exp1 = parseExp(nt->getNonTerminalAt(2));
			
			jump->setTarget(context.getCurrentScope()->getInstructionCount() - instructions);
			
			// TODO fix this
			// here we are assuming that exp1 and exp2 have the same size
			result = exp1;
		}
	}
	
	return result;
}

/*
 * <ASSIGNMENT_EXPRESSION> ::= <CONDITIONAL_EXPRESSION>
 *		| <UNARY_EXPRESSION> <ASSIGNMENT_OPERATOR> <ASSIGNMENT_EXPRESSION>
 *		;
 */
ExpResult CParser::parseAssignmentExp(NonTerminal *nt) {
	assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_ASSIGNMENT_EXPRESSION);
	
	ExpResult result;
	
	if (nt->getNonTerminalRule() == 0) { // <ASSIGNMENT_EXPRESSION> ::= <CONDITIONAL_EXPRESSION>
		result = parseConditionalExp(nt->getNonTerminalAt(0));
	}
	else { // <ASSIGNMENT_EXPRESSION> ::= <UNARY_EXPRESSION> <ASSIGNMENT_OPERATOR> <ASSIGNMENT_EXPRESSION>
		assert(nt->getNonTerminalRule() == 1);
		
		result = parseUnaryExp(nt->getNonTerminalAt(0));
		
		ExpResult value = parseAssignmentExp(nt->getNonTerminalAt(2));
		
		parseAssigmentOperator(nt->getNonTerminalAt(1), result, value);
		
		deallocateExpResult(value);
	}
	
	return result;
}

/*
 * <ASSIGNMENT_OPERATOR> ::= EQ
 *		| MUL_ASSIGN
 *		| DIV_ASSIGN
 *		| MOD_ASSIGN
 *		| ADD_ASSIGN
 *		| SUB_ASSIGN
 *		| LEFT_ASSIGN
 *		| RIGHT_ASSIGN
 *		| AND_ASSIGN
 *		| XOR_ASSIGN
 *		| OR_ASSIGN
 *		;
 */
void CParser::parseAssigmentOperator(NonTerminal *nt, ExpResult & result, ExpResult & value) {
	assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_ASSIGNMENT_OPERATOR);
	
	checkVoidExp(result, nt);
	checkVoidExp(value, nt);
	
	if (result.getResultType() == ExpResult::CONSTANT) {
		throw ParserError(nt->getInputLocation(), "Invalid lvalue.");
	}
	
	assert(result.getResultType() == ExpResult::STACKED);
	
	if (!result.isDynamic()) {
		throw ParserError(nt->getInputLocation(), "Invalid lvalue.");
	}
	
	if (nt->getNonTerminalRule() == 0) { // <ASSIGNMENT_OPERATOR> ::= EQ
		Register pos = result.getPointer(context);
		Register val = value.getValue(context);
		
		addInstruction(new StoreInstruction(val, pos, result.getType()->getSize(), 0));
		
		deallocateRegister(pos);
		deallocateRegister(val);
		
		return;
	}
	
	Register lval = result.getValue(context);
	Register val = value.getValue(context);
	
	switch (nt->getNonTerminalRule()) {
		case 1: // <ASSIGNMENT_OPERATOR> ::= MUL_ASSIGN
			addInstruction(new MulInstruction(val, lval, val));
			break;
		case 2: // <ASSIGNMENT_OPERATOR> ::= DIV_ASSIGN
			addInstruction(new DivInstruction(val, lval, val));
			break;
		case 3: // <ASSIGNMENT_OPERATOR> ::= MOD_ASSIGN
			addInstruction(new ModInstruction(val, lval, val));
			break;
		case 4: // <ASSIGNMENT_OPERATOR> ::= ADD_ASSIGN
			addInstruction(new AddInstruction(val, lval, val));
			break;
		case 5: // <ASSIGNMENT_OPERATOR> ::= SUB_ASSIGN
			addInstruction(new SubInstruction(val, lval, val));
			break;
		case 6: // <ASSIGNMENT_OPERATOR> ::= LEFT_ASSIGN
			addInstruction(new ShiftLeftInstruction(val, lval, val));
			break;
		case 7: // <ASSIGNMENT_OPERATOR> ::= RIGHT_ASSIGN
			addInstruction(new ShiftRightInstruction(val, lval, val));
			break;
		case 8: // <ASSIGNMENT_OPERATOR> ::= AND_ASSIGN
			addInstruction(new AndInstruction(val, lval, val));
			break;
		case 9: // <ASSIGNMENT_OPERATOR> ::= XOR_ASSIGN
			addInstruction(new XorInstruction(val, lval, val));
			break;
		case 10: // <ASSIGNMENT_OPERATOR> ::= OR_ASSIGN
			addInstruction(new OrInstruction(val, lval, val));
			break;
		default:
			abort();
	}
	deallocateRegister(lval);
	
	Register pos = result.getPointer(context);
	
	addInstruction(new StoreInstruction(val, pos, result.getType()->getSize(), 0));
	
	deallocateRegister(pos);
	deallocateRegister(val);
}

/*
 * <EXPRESSION> ::= <ASSIGNMENT_EXPRESSION>
 *		| <EXPRESSION> COMMA <ASSIGNMENT_EXPRESSION>
 *		;
 */
ExpResult CParser::parseExp(NonTerminal *nt) {
	assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_EXPRESSION);
	
	ExpResult result;
	
	if (nt->getNonTerminalRule() == 0) {
		result = parseAssignmentExp(nt->getNonTerminalAt(0));
	}
	else {
		assert(nt->getNonTerminalRule() == 1);
		
		deallocateExpResult(parseExp(nt->getNonTerminalAt(0)));
		result = parseAssignmentExp(nt->getNonTerminalAt(2));
	}
	
	return result;
}

void CParser::deallocateExpResult(const ExpResult & exp) {
	if (exp.getResultType() == ExpResult::STACKED) {
		if (exp.isNeedDeallocate()) {
			context.deallocateStack(exp.getStackAllocSize());
		}
	}
}

/*
 * <CONSTANT_EXPRESSION> ::= <CONDITIONAL_EXPRESSION>;
 */
Number CParser::parseConstantExp(NonTerminal *nt) {
	assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_CONSTANT_EXPRESSION);
	assert(nt->getNonTerminalRule() == 0);
	
	ExpResult result = parseConditionalExp(nt->getNonTerminalAt(0));
	
	if (result.getResultType() != ExpResult::CONSTANT) {
		throw ParserError(nt->getInputLocation(), "Expecting constant expression.");
	}
	
	return result.getConstant();
}
