#include "compiler/CParser.h"

#include "compiler/CompilerContext.h"
#include "compiler/CompilerDefs.h"
#include "vm/ArithmeticInstruction.h"
#include "vm/BranchInstruction.h"
#include "vm/CallInstruction.h"
#include "vm/JumpRegisterInstruction.h"
#include "vm/LabeledInstruction.h"
#include "vm/LoadInstruction.h"
#include "vm/NopInstruction.h"
#include "vm/NotInstruction.h"
#include "vm/SetInstruction.h"
#include "vm/StoreInstruction.h"
#include "CParserBuffer.h"

#include <parser/ParserError.h>

#include <cassert>
#include <cstdlib>
#include <list>

/*
 * <STATEMENT> ::= <LABELED_STATEMENT>
 *		| <COMPOUND_STATEMENT>
 *		| <EXPRESSION_STATEMENT>
 *		| <SELECTION_STATEMENT>
 *		| <ITERATION_STATEMENT>
 *		| <JUMP_STATEMENT>
 *		;
 */
void CParser::parseStatement(NonTerminal *nt, unsigned int scopeFlags) {
	assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_STATEMENT);
	
	switch (nt->getNonTerminalRule()) {
		case 0: // <STATEMENT> ::= <LABELED_STATEMENT>
			parseLabeledStatement(nt->getNonTerminalAt(0));
			break;
		case 1: // <STATEMENT> ::= <COMPOUND_STATEMENT>
			parseCompoundStatement(nt->getNonTerminalAt(0), scopeFlags);
			break;
		case 2: // <STATEMENT> ::= <EXPRESSION_STATEMENT>
			parseExpressionStatement(nt->getNonTerminalAt(0));
			break;
		case 3: // <STATEMENT> ::= <SELECTION_STATEMENT>
			parseSelectionStatement(nt->getNonTerminalAt(0), scopeFlags);
			break;
		case 4: // <STATEMENT> ::= <ITERATION_STATEMENT>
			parseIterationStatement(nt->getNonTerminalAt(0));
			break;
		case 5: // <STATEMENT> ::= <JUMP_STATEMENT>
			parseJumpStatement(nt->getNonTerminalAt(0));
			break;
		default:
			abort();
	}
}

/*
 * The method parseLabeledStatement is meant to be called
 * outside a switch, inside a switch call parseSwitchLabeledStatement
 * instead.
 * 
 * <LABELED_STATEMENT> ::= IDENTIFIER COLUMN <STATEMENT>
 *		| CASE <CONSTANT_EXPRESSION> COLUMN <STATEMENT>
 *		| DEFAULT COLUMN <STATEMENT>
 *		;
 */
void CParser::parseLabeledStatement(NonTerminal *nt) {
	assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_LABELED_STATEMENT);
	
	switch (nt->getNonTerminalRule()) {
		case 0: // <LABELED_STATEMENT> ::= IDENTIFIER COLUMN <STATEMENT>
			addInstruction(new LabeledInstruction(LABEL_PREFIX + nt->getTokenAt(0)->getToken(),
					new NopInstruction()));
			parseStatement(nt->getNonTerminalAt(2));
			break;
		case 1: // <LABELED_STATEMENT> ::= CASE <CONSTANT_EXPRESSION> COLUMN <STATEMENT>
			throw ParserError(nt->getInputLocation(), "case outside a switch.");
			break;
		case 2: // <LABELED_STATEMENT> ::= DEFAULT COLUMN <STATEMENT>
			throw ParserError(nt->getInputLocation(), "default case outside a switch.");
			break;
		default:
			abort();
	}
}

/*
 * <COMPOUND_STATEMENT> ::= BEGIN END
 *		| BEGIN <STATEMENT_LIST> END
 *		| BEGIN <DECLARATION_LIST> END
 *		| BEGIN <DECLARATION_LIST> <STATEMENT_LIST> END
 *		;
 */
Pointer<Scope> CParser::parseCompoundStatement(NonTerminal *nt, unsigned int scopeFlags) {
	assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_COMPOUND_STATEMENT);
	
	Pointer<Scope> scope = context.beginScope();
	scope->setScopeFlags(scopeFlags);
	
	switch (nt->getNonTerminalRule()) {
		case 0: // <COMPOUND_STATEMENT> ::= BEGIN END
			// do nothing
			break;
		case 1: // <COMPOUND_STATEMENT> ::= BEGIN <STATEMENT_LIST> END
			parseStatementList(nt->getNonTerminalAt(1));
			break;
		case 2: // <COMPOUND_STATEMENT> ::= BEGIN <DECLARATION_LIST> END
		{
			DeclarationList declList;
			parseDeclarationList(nt->getNonTerminalAt(1), declList);
			declareLocalVariables(declList);
			break;
		}
		case 3: // <COMPOUND_STATEMENT> ::= BEGIN <DECLARATION_LIST> <STATEMENT_LIST> END
		{
			DeclarationList declList;
			parseDeclarationList(nt->getNonTerminalAt(1), declList);
			declareLocalVariables(declList);
			
			parseStatementList(nt->getNonTerminalAt(2));
			
			break;
		}
		default:
			abort();
	}
	
	context.endScope();
	
	return scope;
}

/*
 * <EXPRESSION_STATEMENT> ::= INST_END
 *		| <EXPRESSION> INST_END
 *		;
 */
void CParser::parseExpressionStatement(NonTerminal *nt) {
	assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_EXPRESSION_STATEMENT);
	
	if (nt->getNonTerminalRule() == 0) { // <EXPRESSION_STATEMENT> ::= INST_END
		// do nothing
	}
	else { // <EXPRESSION_STATEMENT> ::=  <EXPRESSION> INST_END
		assert(nt->getNonTerminalRule() == 1);
		
		ExpResult exp = parseExp(nt->getNonTerminalAt(0));
		deallocateExpResult(exp);
	}
}

/*
 * <SELECTION_STATEMENT> ::= IF P_OPEN <EXPRESSION> P_CLOSE <STATEMENT>
 *		| IF P_OPEN <EXPRESSION> P_CLOSE <STATEMENT> ELSE <STATEMENT>
 *		| SWITCH P_OPEN <EXPRESSION> P_CLOSE <STATEMENT>
 *		;
 */
void CParser::parseSelectionStatement(NonTerminal *nt, unsigned int scopeFlags) {
	assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_SELECTION_STATEMENT);
	
	switch (nt->getNonTerminalRule()) {
		case 0: // <SELECTION_STATEMENT> ::= IF P_OPEN <EXPRESSION> P_CLOSE <STATEMENT>
		{
			ExpResult exp = parseExp(nt->getNonTerminalAt(2));
			if (!exp.getType()->fitRegister()) {
				throw ParserError(nt->getNonTerminalAt(2)->getInputLocation(), "Invalid expression.");
			}
			Register val = exp.getValue(context);
			deallocateExpResult(exp);
			
			addInstruction(new NotInstruction(val, val));
			
			BranchInstruction *branch = new BranchInstruction(val, 0);
			addInstruction(branch);
			deallocateRegister(val);
			unsigned int jump = context.getInstructions().size();
			
			parseStatement(nt->getNonTerminalAt(4));
			
			jump = context.getInstructions().size() - jump;
			branch->setTarget(jump);
			
			break;
		}
		case 1: // <SELECTION_STATEMENT> ::= IF P_OPEN <EXPRESSION> P_CLOSE <STATEMENT> ELSE <STATEMENT>
		{
			ExpResult exp = parseExp(nt->getNonTerminalAt(2));
			if (!exp.getType()->fitRegister()) {
				throw ParserError(nt->getNonTerminalAt(2)->getInputLocation(), "Invalid expression.");
			}
			Register val = exp.getValue(context);
			deallocateExpResult(exp);
			
			BranchInstruction *branch = new BranchInstruction(val, 0);
			addInstruction(branch);
			deallocateRegister(val);
			unsigned int jump = context.getInstructions().size();
			
			// else part
			parseStatement(nt->getNonTerminalAt(6));
			JumpInstruction *jmp = new JumpInstruction(0);
			addInstruction(jmp);
			
			jump = context.getInstructions().size() - jump;
			branch->setTarget(jump);
			jump = context.getInstructions().size();
			
			// if part
			parseStatement(nt->getNonTerminalAt(4));
			
			jump = context.getInstructions().size() - jump;
			jmp->setTarget(jump);
			
			break;
		}
		case 2: // <SELECTION_STATEMENT> ::= SWITCH P_OPEN <EXPRESSION> P_CLOSE <STATEMENT>
			parseSwitchStatement(nt, scopeFlags);
			break;
		default:
			abort();
	}
}

/*
 * <SELECTION_STATEMENT> ::= SWITCH P_OPEN <EXPRESSION> P_CLOSE <STATEMENT>
 */
void CParser::parseSwitchStatement(NonTerminal *nt, unsigned int scopeFlags) {
	assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_SELECTION_STATEMENT);
	assert(nt->getNonTerminalRule() == 2); // <SELECTION_STATEMENT> ::= SWITCH P_OPEN <EXPRESSION> P_CLOSE <STATEMENT>
	
	ExpResult exp = parseExp(nt->getNonTerminalAt(2));
	
	Pointer<SwitchStmt> switchStmt = parseSwitch(nt->getNonTerminalAt(4));
	
	if (switchStmt->isSequential()) {
		int min;
		int max;
		switchStmt->getBounds(&min, &max);
		
		Register val = exp.getValue(context);
		
		Register bound = allocatePRRegister();
		Register cmp = allocatePRRegister();
		
		// check if the expression is greater than max
		addInstruction(new SetInstruction(bound, max));
		addInstruction(new LessCmpInstruction(cmp, bound, val));
		
		BranchInstruction *defaultBrch1 = new BranchInstruction(cmp, 0);
		addInstruction(defaultBrch1);
		unsigned int defaultBrch1Off = context.getInstructions().size();
		
		deallocateRegister(cmp);
		
		// check if the expression is less than min
		addInstruction(new SetInstruction(bound, min));
		addInstruction(new LessCmpInstruction(cmp, val, bound));
		
		BranchInstruction *defaultBrch2 = new BranchInstruction(cmp, 0);
		addInstruction(defaultBrch2);
		unsigned int defaultBrch2Off = context.getInstructions().size();
		
		// jump to $PC + val - min + 2
		addInstruction(new SubInstruction(val, val, bound));
		addInstruction(new SetInstruction(cmp, 1));
		addInstruction(new AddInstruction(val, val, cmp));
		addInstruction(new AddInstruction(val, REG_PC, val));
		addInstruction(new JumpRegisterInstruction(val));
		
		// instructions that need to be redirected to the respective case (ordered)
		std::vector<JumpInstruction *> sequentialJumps;
		
		// add one jump for each case
		for (int i = min; i <= max; ++i) {
			JumpInstruction *jmp = new JumpInstruction(0);
			addInstruction(jmp);
			sequentialJumps.push_back(jmp);
		}
		
		deallocateRegister(val);
		deallocateRegister(bound);
		
		defaultBrch1Off = context.getInstructions().size() - defaultBrch1Off;
		defaultBrch2Off = context.getInstructions().size() - defaultBrch2Off;
		
		parseSwitchStmt(nt->getNonTerminalAt(4), *switchStmt, scopeFlags);
		
		// set the default offsets
		defaultBrch1->setTarget(switchStmt->getDefaultOffset() + defaultBrch1Off);
		defaultBrch2->setTarget(switchStmt->getDefaultOffset() + defaultBrch2Off);
		
		// set the jump offsets
		for (unsigned int i = 0; i < sequentialJumps.size(); ++i) {
			int caseVal = min + (int)i;
			assert(caseVal <= max);
			
			unsigned int offset = switchStmt->getCase(caseVal)->getOffset();
			
			// before the switch stmt begin, we stil have some jumps
			assert((int)offset + (int)max - (int)caseVal >= 0);
			offset += (int)max - (int)caseVal;
			
			sequentialJumps[i]->setTarget(offset);
		}
	}
	else {
		// TODO else-if chain
		abort();
	}
	
	deallocateExpResult(exp);
}

/*
 * <STATEMENT_LIST> ::= <STATEMENT>
 *		| <STATEMENT_LIST> <STATEMENT>
 *		;
 * 
 * <STATEMENT> ::= <LABELED_STATEMENT>
 *		| <COMPOUND_STATEMENT>
 *		| <EXPRESSION_STATEMENT>
 *		| <SELECTION_STATEMENT>
 *		| <ITERATION_STATEMENT>
 *		| <JUMP_STATEMENT>
 *		;
 * 
 * <COMPOUND_STATEMENT> ::= BEGIN END
 *		| BEGIN <STATEMENT_LIST> END
 *		| BEGIN <DECLARATION_LIST> END
 *		| BEGIN <DECLARATION_LIST> <STATEMENT_LIST> END
 *		;
 */
Pointer<SwitchStmt> CParser::parseSwitch(NonTerminal *nt) {
	assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_STATEMENT);
	
	if (nt->getNonTerminalRule() != 1) {// <STATEMENT> ::= <COMPOUND_STATEMENT>
		throw ParserError(nt->getInputLocation(), "Invalid switch statement.");
	}
	nt = nt->getNonTerminalAt(0);
	
	NonTerminal *stmtList = NULL;
	
	switch (nt->getNonTerminalRule()) {
		case 0: // <COMPOUND_STATEMENT> ::= BEGIN END
			// ignore
			break;
		case 1: // <COMPOUND_STATEMENT> ::= BEGIN <STATEMENT_LIST> END
			stmtList = nt->getNonTerminalAt(1);
			break;
		case 2:// <COMPOUND_STATEMENT> ::= BEGIN <DECLARATION_LIST> END
			// ignore
			break;
		case 3: // <COMPOUND_STATEMENT> ::= BEGIN <DECLARATION_LIST> <STATEMENT_LIST> END
			stmtList = nt->getNonTerminalAt(2);
			break;
		default:
			abort();
	}
	
	assert(stmtList->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_STATEMENT_LIST);
	
	// put the statements in a list
	std::list<NonTerminal *> ntList;
	while (stmtList) { // <STATEMENT_LIST> ::= <STATEMENT>
		if (stmtList->getNonTerminalRule() == 0) {
			ntList.push_front(stmtList->getNonTerminalAt(0));
			stmtList = NULL;
		}
		else { // <STATEMENT_LIST> ::= <STATEMENT_LIST> <STATEMENT>
			assert(stmtList->getNonTerminalRule() == 1);
			
			ntList.push_front(stmtList->getNonTerminalAt(1));
			stmtList = stmtList->getNonTerminalAt(0);
		}
	}
	
	Pointer<SwitchStmt> switchStmt = new SwitchStmt();
	
	for (std::list<NonTerminal *>::const_iterator it = ntList.begin(); it != ntList.end(); ++it) {
		nt = *it;
		
		assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_STATEMENT);
		
		if (nt->getNonTerminalRule() == 0) { // <STATEMENT> ::= <LABELED_STATEMENT>
			getSwitchLabeledStmtCase(nt->getNonTerminalAt(0), *switchStmt);
		}
	}
	
	return switchStmt;
}

/*
 * <STATEMENT_LIST> ::= <STATEMENT>
 *		| <STATEMENT_LIST> <STATEMENT>
 *		;
 * 
 * <STATEMENT> ::= <LABELED_STATEMENT>
 *		| <COMPOUND_STATEMENT>
 *		| <EXPRESSION_STATEMENT>
 *		| <SELECTION_STATEMENT>
 *		| <ITERATION_STATEMENT>
 *		| <JUMP_STATEMENT>
 *		;
 * 
 * <COMPOUND_STATEMENT> ::= BEGIN END
 *		| BEGIN <STATEMENT_LIST> END
 *		| BEGIN <DECLARATION_LIST> END
 *		| BEGIN <DECLARATION_LIST> <STATEMENT_LIST> END
 *		;
 */
void CParser::parseSwitchStmt(NonTerminal *nt, SwitchStmt & switchStmt, unsigned int scopeFlags) {
	assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_STATEMENT);
	
	if (nt->getNonTerminalRule() != 1) {// <STATEMENT> ::= <COMPOUND_STATEMENT>
		throw ParserError(nt->getInputLocation(), "Invalid switch statement.");
	}
	nt = nt->getNonTerminalAt(0);
	
	Pointer<Scope> scope = context.beginScope();
	scope->setScopeFlags(scopeFlags | Scope::CAN_BREAK);
	
	NonTerminal *stmtList = NULL;
	
	switch (nt->getNonTerminalRule()) {
		case 0: // <COMPOUND_STATEMENT> ::= BEGIN END
			// ignore
			break;
		case 1: // <COMPOUND_STATEMENT> ::= BEGIN <STATEMENT_LIST> END
			stmtList = nt->getNonTerminalAt(1);
			break;
		case 2:// <COMPOUND_STATEMENT> ::= BEGIN <DECLARATION_LIST> END
		{
			DeclarationList declList;
			parseDeclarationList(nt->getNonTerminalAt(1), declList);
			declareLocalVariables(declList);
			
			break;
		}
		case 3: // <COMPOUND_STATEMENT> ::= BEGIN <DECLARATION_LIST> <STATEMENT_LIST> END
		{
			DeclarationList declList;
			parseDeclarationList(nt->getNonTerminalAt(1), declList);
			declareLocalVariables(declList);
			
			stmtList = nt->getNonTerminalAt(2);
			
			break;
		}
		default:
			abort();
	}
	
	assert(stmtList->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_STATEMENT_LIST);
	
	// put the statements in a list
	std::list<NonTerminal *> ntList;
	while (stmtList) { // <STATEMENT_LIST> ::= <STATEMENT>
		if (stmtList->getNonTerminalRule() == 0) {
			ntList.push_front(stmtList->getNonTerminalAt(0));
			stmtList = NULL;
		}
		else { // <STATEMENT_LIST> ::= <STATEMENT_LIST> <STATEMENT>
			assert(stmtList->getNonTerminalRule() == 1);
			
			ntList.push_front(stmtList->getNonTerminalAt(1));
			stmtList = stmtList->getNonTerminalAt(0);
		}
	}
	
	for (std::list<NonTerminal *>::const_iterator it = ntList.begin(); it != ntList.end(); ++it) {
		nt = *it;
		
		assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_STATEMENT);
		
		if (nt->getNonTerminalRule() == 0) { // <STATEMENT> ::= <LABELED_STATEMENT>
			parseSwitchLabeledStatement(nt->getNonTerminalAt(0), switchStmt);
		}
		else parseStatement(nt);
	}
	
	if (switchStmt.getDefaultOffset() == 0) {
		switchStmt.setDefaultOffset(context.getInstructions().size()
				- context.getCurrentScope()->getInstructionsBegin());
	}
	
	context.endScope();
}

/*
 * <LABELED_STATEMENT> ::= IDENTIFIER COLUMN <STATEMENT>
 *		| CASE <CONSTANT_EXPRESSION> COLUMN <STATEMENT>
 *		| DEFAULT COLUMN <STATEMENT>
 *		;
 */
void CParser::getSwitchLabeledStmtCase(NonTerminal *nt, SwitchStmt & switchStmt) {
	assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_LABELED_STATEMENT);
	
	switch (nt->getNonTerminalRule()) {
		case 0: // <LABELED_STATEMENT> ::= IDENTIFIER COLUMN <STATEMENT>
			// ignore
			break;
		case 1: // <LABELED_STATEMENT> ::= CASE <CONSTANT_EXPRESSION> COLUMN <STATEMENT>
		{
			Number value = parseConstantExp(nt->getNonTerminalAt(1));;
			
			SwitchStmt::Case *c = new SwitchStmt::Case(0, value);
			switchStmt.addCase(c);
			
			break;
		}
		case 2: // <LABELED_STATEMENT> ::= DEFAULT COLUMN <STATEMENT>
			// ignore
			break;
		default:
			abort();
	}
}

/*
 * <LABELED_STATEMENT> ::= IDENTIFIER COLUMN <STATEMENT>
 *		| CASE <CONSTANT_EXPRESSION> COLUMN <STATEMENT>
 *		| DEFAULT COLUMN <STATEMENT>
 *		;
 */
void CParser::parseSwitchLabeledStatement(NonTerminal *nt, SwitchStmt & switchStmt) {
	assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_LABELED_STATEMENT);
	
	switch (nt->getNonTerminalRule()) {
		case 0: // <LABELED_STATEMENT> ::= IDENTIFIER COLUMN <STATEMENT>
			parseLabeledStatement(nt);
			break;
		case 1: // <LABELED_STATEMENT> ::= CASE <CONSTANT_EXPRESSION> COLUMN <STATEMENT>
		{
			Number value = parseConstantExp(nt->getNonTerminalAt(1));
			
			SwitchStmt::Case *c = switchStmt.getCase(value);
			assert(context.getInstructions().size() >= context.getCurrentScope()->getInstructionsBegin());
			c->setOffset(context.getInstructions().size() - context.getCurrentScope()->getInstructionsBegin());
			
			parseStatement(nt->getNonTerminalAt(3));
			break;
		}
		case 2: // <LABELED_STATEMENT> ::= DEFAULT COLUMN <STATEMENT>
			switchStmt.setDefaultOffset(context.getInstructions().size()
					- context.getCurrentScope()->getInstructionsBegin());
			
			parseStatement(nt->getNonTerminalAt(2));
			break;
		default:
			abort();
	}
}

/*
 *  <EXPRESSION_STATEMENT> ::= INST_END
 *		| <EXPRESSION> INST_END
 *		;
 * 
 * <ITERATION_STATEMENT> ::= WHILE P_OPEN <EXPRESSION> P_CLOSE <STATEMENT>
 *		| DO <STATEMENT> WHILE P_OPEN <EXPRESSION> P_CLOSE INST_END
 *		| FOR P_OPEN <EXPRESSION_STATEMENT> <EXPRESSION_STATEMENT> P_CLOSE <STATEMENT>
 *		| FOR P_OPEN <EXPRESSION_STATEMENT> <EXPRESSION_STATEMENT> <EXPRESSION> P_CLOSE <STATEMENT>
 *		;
 */
void CParser::parseIterationStatement(NonTerminal *nt) {
	assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_ITERATION_STATEMENT);
	
	switch (nt->getNonTerminalRule()) {
		case 0: // <ITERATION_STATEMENT> ::= WHILE P_OPEN <EXPRESSION> P_CLOSE <STATEMENT>
		{
			int whileBegin = context.getInstructions().size();
			
			ExpResult exp = parseExp(nt->getNonTerminalAt(2));
			if (!exp.getType()->fitRegister()) {
				throw ParserError(nt->getNonTerminalAt(2)->getInputLocation(), "Invalid expression.");
			}
			Register val = exp.getValue(context);
			deallocateExpResult(exp);
			
			addInstruction(new NotInstruction(val, val));
			
			BranchInstruction *branch = new BranchInstruction(val, 0);
			addInstruction(branch);
			deallocateRegister(val);
			unsigned int jump = context.getInstructions().size();
			
			parseStatement(nt->getNonTerminalAt(4), Scope::CAN_BREAK | Scope::CAN_CONTINUE);
			
			// back to whileBegin
			// the -1 is needed for: the jump instruction itself (the size is got before the instruction is added)
			addInstruction(new JumpInstruction(whileBegin - (int)context.getInstructions().size() - 1));
			
			jump = context.getInstructions().size() - jump;
			branch->setTarget(jump);
			
			break;
		}
		case 1: // <ITERATION_STATEMENT> ::= DO <STATEMENT> WHILE P_OPEN <EXPRESSION> P_CLOSE INST_END
		{
			int whileBegin = context.getInstructions().size();
			
			parseStatement(nt->getNonTerminalAt(1), Scope::CAN_BREAK | Scope::CAN_CONTINUE);
			
			ExpResult exp = parseExp(nt->getNonTerminalAt(4));
			if (!exp.getType()->fitRegister()) {
				throw ParserError(nt->getNonTerminalAt(4)->getInputLocation(), "Invalid expression.");
			}
			Register val = exp.getValue(context);
			deallocateExpResult(exp);
			
			// back to whileBegin
			// the -1 is needed for: the branch instruction itself (the size is got before the instruction is added)
			addInstruction(new BranchInstruction(val, whileBegin - (int)context.getInstructions().size() - 1));
			deallocateRegister(val);
			
			break;
		}
		case 2: // <ITERATION_STATEMENT> ::= FOR P_OPEN <EXPRESSION_STATEMENT> <EXPRESSION_STATEMENT> P_CLOSE <STATEMENT>
			parseForStmt(nt->getNonTerminalAt(2), nt->getNonTerminalAt(3), NULL, nt->getNonTerminalAt(5));
			break;
		case 3: // <ITERATION_STATEMENT> ::= FOR P_OPEN <EXPRESSION_STATEMENT> <EXPRESSION_STATEMENT> <EXPRESSION> P_CLOSE <STATEMENT>
			parseForStmt(nt->getNonTerminalAt(2), nt->getNonTerminalAt(3), nt->getNonTerminalAt(4), nt->getNonTerminalAt(6));
			break;
		default:
			abort();
	}
}

void CParser::parseForStmt(NonTerminal *init, NonTerminal *expStmt, NonTerminal *inc, NonTerminal *stmt) {
	assert(init->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_EXPRESSION_STATEMENT);
	assert(expStmt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_EXPRESSION_STATEMENT);
	assert(!inc || inc->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_EXPRESSION);
	assert(stmt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_STATEMENT);
	
	// initialization
	parseExpressionStatement(init);
	
	// jump the inc in the first iteration
	// NOTE: the inc must be in before the statement, or the continue directive won't work
	JumpInstruction *incJmp = new JumpInstruction(0);
	addInstruction(incJmp);
	
	int forBegin = context.getInstructions().size(); 
	
	// create an extra scope, so the continue directive won't skip the inc
	Pointer<Scope> scopeWithInc = context.beginScope();
	scopeWithInc->setScopeFlags(Scope::CAN_CONTINUE | Scope::CAN_BREAK);
	
	if (inc) deallocateExpResult(parseExp(inc));
	
	incJmp->setTarget(context.getInstructions().size() - forBegin);
	
	BranchInstruction *branch = NULL;
	unsigned int jump = 0;
	
	assert(expStmt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_EXPRESSION_STATEMENT);
	if (expStmt->getNonTerminalRule() == 0) { // <EXPRESSION_STATEMENT> ::= INST_END
		// always true, do nothing
	}
	else { // <EXPRESSION_STATEMENT> ::= <EXPRESSION> INST_END
		assert(expStmt->getNonTerminalRule() == 1);
		
		ExpResult exp = parseExp(expStmt->getNonTerminalAt(0));
		if (!exp.getType()->fitRegister()) {
			throw ParserError(stmt->getInputLocation(), "Invalid expression.");
		}
		Register val = exp.getValue(context);
		deallocateExpResult(exp);
		
		addInstruction(new NotInstruction(val, val));
		
		branch = new BranchInstruction(val, 0);
		addInstruction(branch);
		deallocateRegister(val);
		
		jump = context.getInstructions().size();
	}
	
	// this statement cannot has no flag CAN_BREAK and CAN_CONTINUE, those are in the external scope
	parseStatement(stmt);
	
	// back to forBegin
	// the -1 is needed for: the jump instruction itself (the size is got before the instruction is added)
	addInstruction(new JumpInstruction(forBegin - (int)context.getInstructions().size() - 1));
	
	if (branch) {
		jump = context.getInstructions().size() - jump;
		branch->setTarget(jump);
	}
	
	// end of scopeWithInc
	context.endScope();
}

/*
 * <JUMP_STATEMENT> ::= GOTO IDENTIFIER INST_END
 *		| CONTINUE INST_END
 *		| BREAK INST_END
 *		| RETURN INST_END
 *		| RETURN <EXPRESSION> INST_END
 *		;
 */
void CParser::parseJumpStatement(NonTerminal *nt) {
	assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_JUMP_STATEMENT);
	
	switch (nt->getNonTerminalRule()) {
		case 0: // <JUMP_STATEMENT> ::= GOTO IDENTIFIER INST_END
		{
			// TODO check for declarations when entering/exiting scope
			
			addInstruction(new CallInstruction(LABEL_PREFIX + nt->getTokenAt(1)->getToken()));
			break;
		}
		case 1: // <JUMP_STATEMENT> ::= CONTINUE INST_END
		{
			Pointer<Scope> scope = context.getScopeWithFlag(Scope::CAN_CONTINUE);
			
			if (!scope) throw ParserError(nt->getInputLocation(), "Continue outside for/while statement");
			
			unsigned int stackMem = context.getAccumulatedStack(scope);
			
			// deallocate the stack
			if (stackMem > 0) {
				Register reg = allocatePRRegister();
				addInstruction(new SetInstruction(reg, stackMem));
				addInstruction(new AddInstruction(REG_SP, REG_SP, reg));
				deallocateRegister(reg);
			}
			
			// create a jump to the begin of the scope
			addInstruction(scope->createJumpInstScopeBegin(context));
			
			break;
		}
		case 2: // <JUMP_STATEMENT> ::= BREAK INST_END
		{
			Pointer<Scope> scope = context.getScopeWithFlag(Scope::CAN_BREAK);
			
			if (!scope) throw ParserError(nt->getInputLocation(), "Break outside for/while/switch statement");
			
			unsigned int stackMem = context.getAccumulatedStack(scope);
			
			// deallocate the stack
			if (stackMem > 0) {
				Register reg = allocatePRRegister();
				addInstruction(new SetInstruction(reg, stackMem));
				addInstruction(new AddInstruction(REG_SP, REG_SP, reg));
				deallocateRegister(reg);
			}
			
			// create a jump to the end of the scope
			addInstruction(scope->createJumpInstScopeEnd(context));
			
			break;
		}
		case 3: // <JUMP_STATEMENT> ::= RETURN INST_END
		{
			const Pointer<Function> func = context.getCurrentFunction();
			unsigned int retSize = func->getReturnValueSize();
			
			// deallocate the stack used for local variables
			// cannot use deallocateStack becouse it would mess with the
			// base sp offset of this function
			Register val = allocatePRRegister();
			addInstruction(new SetInstruction(val, func->getStackBaseOffset() - retSize));
			addInstruction(new AddInstruction(REG_SP, REG_SP, val));
			
			deallocateRegister(val);
			
			// now the return addr should be on the top of the stack
			// let's jump to it
			Register addr = allocatePRRegister();
			addInstruction(new LoadInstruction(addr, REG_SP, REGISTER_SIZE, retSize));
			addInstruction(new JumpRegisterInstruction(addr));
			deallocateRegister(addr);
			
			break;
		}
		case 4: // <JUMP_STATEMENT> ::= RETURN <EXPRESSION> INST_END
		{
			ExpResult exp = parseExp(nt->getNonTerminalAt(1));
			
			const Pointer<Function> func = context.getCurrentFunction();
			unsigned int retSize = func->getReturnValueSize();
			if (retSize == 0) throw ParserError(nt->getInputLocation(), "Returning a value from a void function.");
			
			// set the return value
			Register val = exp.getValue(context);
			addInstruction(new StoreInstruction(val, REG_SP, retSize, func->getReturnValueSPOffset()));
			
			// deallocate the stack used for local variables
			// cannot use deallocateStack becouse it would mess with the
			// base sp offset of this function
			addInstruction(new SetInstruction(val, func->getStackBaseOffset() - retSize));
			addInstruction(new AddInstruction(REG_SP, REG_SP, val));
			
			deallocateRegister(val);
			
			// not the stack should have only the return value and the return addr
			// let's jump to the return addr
			Register addr = allocatePRRegister();
			addInstruction(new LoadInstruction(addr, REG_SP, REGISTER_SIZE, retSize));
			addInstruction(new JumpRegisterInstruction(addr));
			deallocateRegister(addr);
			
			if (exp.getResultType() == ExpResult::STACKED) {
				context.getCurrentFunction()->decrementStackBaseOffset(exp.getType()->getSize());
			}
			
			break;
		}
		default:
			abort();
	}
}

/*
 * <STATEMENT_LIST> ::= <STATEMENT>
 *		| <STATEMENT_LIST> <STATEMENT>
 *		;
 */
void CParser::parseStatementList(NonTerminal *nt) {
	assert(nt->getNonTerminalId() == CPARSERBUFFER_NONTERMINAL_STATEMENT_LIST);
	
	if (nt->getNonTerminalRule() == 0) { // <STATEMENT_LIST> ::= <STATEMENT>
		parseStatement(nt->getNonTerminalAt(0));
	}
	else { // <STATEMENT_LIST> ::= <STATEMENT_LIST> <STATEMENT>
		assert(nt->getNonTerminalRule() == 1);
		
		parseStatementList(nt->getNonTerminalAt(0));
		parseStatement(nt->getNonTerminalAt(1));
	}
}
