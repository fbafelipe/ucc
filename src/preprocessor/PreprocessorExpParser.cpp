#include "preprocessor/PreprocessorExpParser.h"

#include "preprocessor/DefineMap.h"
#include "preprocessor/Preprocessor.h"
#include "preprocessor/PreprocessorExpScanner.h"
#include "Number.h"
#include "PreprocExpParserBuffer.h"
#include "UccUtils.h"

#include <parser/Parser.h>
#include <parser/ParserError.h>

#include <cassert>
#include <cstdlib>

/*****************************************************************************
 * PreprocessorExpParser
 *****************************************************************************/
PreprocessorExpParser::PreprocessorExpParser(const Preprocessor *preproc,
		const DefineMap & defMap) : defineMap(defMap) {
	
	scannerAutomata = preproc->getExpScannerAutomata();
	parserTable = preproc->getExpParserTable();
}

bool PreprocessorExpParser::parseExp(Input *exp) const {
	Scanner *scanner = new PreprocessorExpScanner(scannerAutomata, exp, defineMap);
	Parser *parser = new Parser(parserTable, scanner);
	
	Node *root = parser->parse();
	
	assert(root->getNodeType() == ParsingTree::NODE_NON_TERMINAL);
	bool result = evaluateExpression((NonTerminal *)root).boolValue();
	
	delete(root);
	delete(parser);
	
	return result;
}

/*****************************************************************************
 * Auxiliar functions
 *****************************************************************************/

/*
 * <EXPRESSION> ::= <LOGICAL_OR_EXPRESSION>;
 */
Number PreprocessorExpParser::evaluateExpression(NonTerminal *nt) const {
	assert(nt->getNonTerminalId() == PREPROCEXPPARSERBUFFER_NONTERMINAL_EXPRESSION);
	assert(nt->getNonTerminalRule() == 0);
	
	return evaluateConditionalExp(nt->getNonTerminalAt(0));
}

/*
 * <PRIMARY_EXPRESSION> ::= CONSTANT
 *		| P_OPEN <EXPRESSION> P_CLOSE
 *		| DEFINED P_OPEN IDENTIFIER P_CLOSE
 *		| DEFINED_M IDENTIFIER P_CLOSE
 *		| DEFINED IDENTIFIER
 *		| IDENTIFIER
 *		;
 */
Number PreprocessorExpParser::evaluatePrimaryExp(NonTerminal *nt) const {
	assert(nt->getNonTerminalId() == PREPROCEXPPARSERBUFFER_NONTERMINAL_PRIMARY_EXPRESSION);
	
	Number result;
	
	switch (nt->getNonTerminalRule()) {
		case 0: // <PRIMARY_EXPRESSION> ::= CONSTANT
			result = evaluateConstant(nt->getTokenAt(0));
			break;
		case 1: // <PRIMARY_EXPRESSION> ::= P_OPEN <EXPRESSION> P_CLOSE
			result = evaluateExpression(nt->getNonTerminalAt(1));
			break;
		case 2: // <PRIMARY_EXPRESSION> ::= DEFINED P_OPEN IDENTIFIER P_CLOSE
			result = Number(defineMap.isDefined(nt->getTokenAt(2)->getToken()));
			break;
		case 3:// <PRIMARY_EXPRESSION> ::= DEFINED_M IDENTIFIER P_CLOSE
			result = Number(defineMap.isDefined(nt->getTokenAt(1)->getToken()));
			break;
		case 4: // <PRIMARY_EXPRESSION> ::= DEFINED IDENTIFIER
			result = Number(defineMap.isDefined(nt->getTokenAt(1)->getToken()));
			break;
		case 5:
			throw ParserError(nt->getInputLocation(), std::string("\"") + nt->getTokenAt(0)->getToken()
					+ "\" undefined."); 
			break;
		default:
			abort();
	}
	
	return result;
}

/*
 * <UNARY_EXPRESSION> ::= <PRIMARY_EXPRESSION>
 *		| <UNARY_OPERATOR> <PRIMARY_EXPRESSION>
 *		;
 */
Number PreprocessorExpParser::evaluateUnaryExp(NonTerminal *nt) const {
	assert(nt->getNonTerminalId() == PREPROCEXPPARSERBUFFER_NONTERMINAL_UNARY_EXPRESSION);
	
	Number result;
	
	if (nt->getNonTerminalRule() == 0) {
		result = evaluatePrimaryExp(nt->getNonTerminalAt(0));
	}
	else {
		assert(nt->getNonTerminalRule() == 1);
		
		result = evaluatePrimaryExp(nt->getNonTerminalAt(1));
		result = evaluateUnaryOperator(nt->getNonTerminalAt(0), result);
	}
	
	return result;
}

/*
 * <UNARY_OPERATOR> ::= PLUS_SIG
 *		| LESS_SIG
 *		| NEG
 *		| NOT
 *		;
 */
Number PreprocessorExpParser::evaluateUnaryOperator(NonTerminal *nt, Number num) const {
	assert(nt->getNonTerminalId() == PREPROCEXPPARSERBUFFER_NONTERMINAL_UNARY_OPERATOR);
	
	Number result;
	
	switch (nt->getNonTerminalRule()) {
		case 0: // <UNARY_OPERATOR> ::= PLUS_SIG
			result = num;
			break;
		case 1: // <UNARY_OPERATOR> ::= LESS_SIG
			result = -num;
			break;
		case 2: // <UNARY_OPERATOR> ::= NEG
			if (num.isFloat()) throw ParserError(nt->getInputLocation(), "Invalid unary operand.");
			result = ~num;;
			break;
		case 3: // <UNARY_OPERATOR> ::= NOT
			result = !num;
			break;
		default:
			abort();
	}
	
	return result;
}

/*
 * <MULTIPLICATIVE_EXPRESSION> ::= <UNARY_EXPRESSION>
 *		| <MULTIPLICATIVE_EXPRESSION> MUL <UNARY_EXPRESSION>
 *		| <MULTIPLICATIVE_EXPRESSION> DIV <UNARY_EXPRESSION>
 *		| <MULTIPLICATIVE_EXPRESSION> MOD <UNARY_EXPRESSION>
 *		;
 */
Number PreprocessorExpParser::evaluateMultiplicativeExp(NonTerminal *nt) const {
	assert(nt->getNonTerminalId() == PREPROCEXPPARSERBUFFER_NONTERMINAL_MULTIPLICATIVE_EXPRESSION);
	
	Number result;
	Number a, b;
	
	switch (nt->getNonTerminalRule()) {
		case 0: // <MULTIPLICATIVE_EXPRESSION> ::= <UNARY_EXPRESSION>
			result = evaluateUnaryExp(nt->getNonTerminalAt(0));
			break;
		case 1: // <MULTIPLICATIVE_EXPRESSION> ::=  <MULTIPLICATIVE_EXPRESSION> MUL <UNARY_EXPRESSION>
			a = evaluateMultiplicativeExp(nt->getNonTerminalAt(2));
			b = evaluateUnaryExp(nt->getNonTerminalAt(0));
			result = a * b;
			break;
		case 2: // <MULTIPLICATIVE_EXPRESSION> ::= <MULTIPLICATIVE_EXPRESSION> DIV <UNARY_EXPRESSION>
			a = evaluateMultiplicativeExp(nt->getNonTerminalAt(2));
			b = evaluateUnaryExp(nt->getNonTerminalAt(0));
			result = a / b;
			break;
		case 3: // <MULTIPLICATIVE_EXPRESSION> ::= <MULTIPLICATIVE_EXPRESSION> MOD <UNARY_EXPRESSION>
			a = evaluateMultiplicativeExp(nt->getNonTerminalAt(2));
			b = evaluateUnaryExp(nt->getNonTerminalAt(0));
			if (b.isFloat()) throw ParserError(nt->getInputLocation(), "Invalid operand.");
			result = a % b;
			break;
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
Number PreprocessorExpParser::evaluateAdditiveExp(NonTerminal *nt) const {
	assert(nt->getNonTerminalId() == PREPROCEXPPARSERBUFFER_NONTERMINAL_ADDITIVE_EXPRESSION);
	
	Number result;
	Number a, b;
	
	switch (nt->getNonTerminalRule()) {
		case 0: // <ADDITIVE_EXPRESSION> ::= <MULTIPLICATIVE_EXPRESSION>
			result = evaluateMultiplicativeExp(nt->getNonTerminalAt(0));
			break;
		case 1: // <ADDITIVE_EXPRESSION> ::= <ADDITIVE_EXPRESSION> PLUS_SIG <MULTIPLICATIVE_EXPRESSION>
			a = evaluateAdditiveExp(nt->getNonTerminalAt(0));
			b = evaluateMultiplicativeExp(nt->getNonTerminalAt(2));
			result = a + b;
			break;
		case 2: // <ADDITIVE_EXPRESSION> ::= <ADDITIVE_EXPRESSION> LESS_SIG <MULTIPLICATIVE_EXPRESSION>
			a = evaluateAdditiveExp(nt->getNonTerminalAt(0));
			b = evaluateMultiplicativeExp(nt->getNonTerminalAt(2));
			result = a - b;
			break;
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
Number PreprocessorExpParser::evaluateShiftExp(NonTerminal *nt) const {
	assert(nt->getNonTerminalId() == PREPROCEXPPARSERBUFFER_NONTERMINAL_SHIFT_EXPRESSION);
	
	Number result;
	Number a, b;
	
	switch (nt->getNonTerminalRule()) {
		case 0: // <SHIFT_EXPRESSION> ::= <ADDITIVE_EXPRESSION>
			result = evaluateAdditiveExp(nt->getNonTerminalAt(0));
			break;
		case 1: // <SHIFT_EXPRESSION> ::= <SHIFT_EXPRESSION> LEFT_OP <ADDITIVE_EXPRESSION>
			a = evaluateShiftExp(nt->getNonTerminalAt(0));
			b = evaluateAdditiveExp(nt->getNonTerminalAt(2));
			if (b.isFloat()) throw ParserError(nt->getInputLocation(), "Invalid operand.");
			result = a << b.intValue();
			break;
		case 2: // <SHIFT_EXPRESSION> ::= <SHIFT_EXPRESSION> RIGHT_OP <ADDITIVE_EXPRESSION>
			a = evaluateShiftExp(nt->getNonTerminalAt(0));
			b = evaluateAdditiveExp(nt->getNonTerminalAt(2));
			if (b.isFloat()) throw ParserError(nt->getInputLocation(), "Invalid operand.");
			result = a >> b.intValue();
			break;
		default:
			abort();
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
Number PreprocessorExpParser::evaluateRelationExp(NonTerminal *nt) const {
	assert(nt->getNonTerminalId() == PREPROCEXPPARSERBUFFER_NONTERMINAL_RELATIONAL_EXPRESSION);
	
	Number result;
	Number a, b;
	
	switch (nt->getNonTerminalRule()) {
		case 0: // <RELATIONAL_EXPRESSION> ::= <SHIFT_EXPRESSION>
			result = evaluateShiftExp(nt->getNonTerminalAt(0));
			break;
		case 1: // <RELATIONAL_EXPRESSION> ::= <RELATIONAL_EXPRESSION> LESS <SHIFT_EXPRESSION>
			a = evaluateRelationExp(nt->getNonTerminalAt(0));
			b = evaluateShiftExp(nt->getNonTerminalAt(2));
			result = Number(a < b);
			break;
		case 2: // <RELATIONAL_EXPRESSION> ::= <RELATIONAL_EXPRESSION> GREATER <SHIFT_EXPRESSION>
			a = evaluateRelationExp(nt->getNonTerminalAt(0));
			b = evaluateShiftExp(nt->getNonTerminalAt(2));
			result = Number(a > b);
			break;
		case 3: // <RELATIONAL_EXPRESSION> ::= <RELATIONAL_EXPRESSION> LE_OP <SHIFT_EXPRESSION>
			a = evaluateRelationExp(nt->getNonTerminalAt(0));
			b = evaluateShiftExp(nt->getNonTerminalAt(2));
			result = Number(a <= b);
			break;
		case 4: // <RELATIONAL_EXPRESSION> ::= <RELATIONAL_EXPRESSION> LE_OP <SHIFT_EXPRESSION>
			a = evaluateRelationExp(nt->getNonTerminalAt(0));
			b = evaluateShiftExp(nt->getNonTerminalAt(2));
			result = Number(a >= b);
			break;
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
Number PreprocessorExpParser::evaluateEqualityExp(NonTerminal *nt) const {
	assert(nt->getNonTerminalId() == PREPROCEXPPARSERBUFFER_NONTERMINAL_EQUALITY_EXPRESSION);
	
	Number result;
	Number a, b;
	
	switch (nt->getNonTerminalRule()) {
		case 0: // <EQUALITY_EXPRESSION> ::= <RELATIONAL_EXPRESSION>
			result = evaluateRelationExp(nt->getNonTerminalAt(0));
			break;
		case 1: // <EQUALITY_EXPRESSION> ::= <EQUALITY_EXPRESSION> EQ_OP <RELATIONAL_EXPRESSION>
			a = evaluateEqualityExp(nt->getNonTerminalAt(0));
			b = evaluateRelationExp(nt->getNonTerminalAt(2));
			result = Number(a == b);
			break;
		case 2: // <EQUALITY_EXPRESSION> ::= <EQUALITY_EXPRESSION> NE_OP <RELATIONAL_EXPRESSION>
			a = evaluateEqualityExp(nt->getNonTerminalAt(0));
			b = evaluateRelationExp(nt->getNonTerminalAt(2));
			result = Number(a != b);
			break;
		default:
			abort();
	}
	
	return result;
}

/*
 * <AND_EXPRESSION> ::= <EQUALITY_EXPRESSION>
 *		| <AND_EXPRESSION> AND <EQUALITY_EXPRESSION>
 *		;
 */
Number PreprocessorExpParser::evaluateAndExp(NonTerminal *nt) const {
	assert(nt->getNonTerminalId() == PREPROCEXPPARSERBUFFER_NONTERMINAL_AND_EXPRESSION);
	
	Number result;
	
	if (nt->getNonTerminalRule() == 0) {
		result = evaluateEqualityExp(nt->getNonTerminalAt(0));
	}
	else {
		assert(nt->getNonTerminalRule() == 1);
		
		Number a = evaluateAndExp(nt->getNonTerminalAt(0));
		Number b = evaluateEqualityExp(nt->getNonTerminalAt(2));
		if (a.isFloat() || b.isFloat()) { 
			throw ParserError(nt->getInputLocation(), "use of invalid operand.");
		}
		result = a & b;
	}
	
	return result;
}

/*
 *<EXCLUSIVE_OR_EXPRESSION> ::= <AND_EXPRESSION>
 *		| <EXCLUSIVE_OR_EXPRESSION> XOR <AND_EXPRESSION>
 *		;
 */
Number PreprocessorExpParser::evaluateExclusiveOrExp(NonTerminal *nt) const {
	assert(nt->getNonTerminalId() == PREPROCEXPPARSERBUFFER_NONTERMINAL_EXCLUSIVE_OR_EXPRESSION);
	
	Number result;
	
	if (nt->getNonTerminalRule() == 0) {
		result = evaluateAndExp(nt->getNonTerminalAt(0));
	}
	else {
		assert(nt->getNonTerminalRule() == 1);
		
		Number a = evaluateExclusiveOrExp(nt->getNonTerminalAt(0));
		Number b = evaluateAndExp(nt->getNonTerminalAt(2));
		if (a.isFloat() || b.isFloat()) { 
			throw ParserError(nt->getInputLocation(), "use of invalid operand.");
		}
		result = a ^ b;
	}
	
	return result;
}

/*
 * <INCLUSIVE_OR_EXPRESSION> ::= <EXCLUSIVE_OR_EXPRESSION>
 *		| <INCLUSIVE_OR_EXPRESSION> OR <EXCLUSIVE_OR_EXPRESSION>
 *		;
 */
Number PreprocessorExpParser::evaluateInclusiveOrExp(NonTerminal *nt) const {
	assert(nt->getNonTerminalId() == PREPROCEXPPARSERBUFFER_NONTERMINAL_INCLUSIVE_OR_EXPRESSION);
	
	Number result;
	
	if (nt->getNonTerminalRule() == 0) {
		result = evaluateExclusiveOrExp(nt->getNonTerminalAt(0));
	}
	else {
		assert(nt->getNonTerminalRule() == 1);
		
		Number a = evaluateInclusiveOrExp(nt->getNonTerminalAt(0));
		Number b = evaluateExclusiveOrExp(nt->getNonTerminalAt(2));
		if (a.isFloat() || b.isFloat()) { 
			throw ParserError(nt->getInputLocation(), "use of invalid operand.");
		}
		result = a | b;
	}
	
	return result;
}

/*
 * <LOGICAL_AND_EXPRESSION> ::= <INCLUSIVE_OR_EXPRESSION>
 *		| <LOGICAL_AND_EXPRESSION> AND_OP <INCLUSIVE_OR_EXPRESSION>
 *		;
 */
Number PreprocessorExpParser::evaluateLogicalAndExp(NonTerminal *nt) const {
	assert(nt->getNonTerminalId() == PREPROCEXPPARSERBUFFER_NONTERMINAL_LOGICAL_AND_EXPRESSION);
	
	Number result;
	
	if (nt->getNonTerminalRule() == 0) {
		result = evaluateInclusiveOrExp(nt->getNonTerminalAt(0));
	}
	else {
		assert(nt->getNonTerminalRule() == 1);
		
		Number a = evaluateLogicalAndExp(nt->getNonTerminalAt(0));
		if (!a.boolValue()) result = Number(false);
		else {
			Number b = evaluateInclusiveOrExp(nt->getNonTerminalAt(2));
			result = Number(b.boolValue());
		}
	}
	
	return result;
}

/*
 * <LOGICAL_OR_EXPRESSION> ::= <LOGICAL_AND_EXPRESSION>
 *		| <LOGICAL_OR_EXPRESSION> OR_OP <LOGICAL_AND_EXPRESSION>
 *		;
 */
Number PreprocessorExpParser::evaluateLogicalOrExp(NonTerminal *nt) const {
	assert(nt->getNonTerminalId() == PREPROCEXPPARSERBUFFER_NONTERMINAL_LOGICAL_OR_EXPRESSION);
	
	Number result;
	
	if (nt->getNonTerminalRule() == 0) {
		result = evaluateLogicalAndExp(nt->getNonTerminalAt(0));
	}
	else {
		assert(nt->getNonTerminalRule() == 1);
		
		Number a = evaluateLogicalOrExp(nt->getNonTerminalAt(0));
		if (a.boolValue()) result = Number(true);
		else {
			Number b = evaluateLogicalAndExp(nt->getNonTerminalAt(2));
			result = Number(b.boolValue());
		}
	}
	
	return result;
}

/*
 * <CONDITIONAL_EXPRESSION> ::= <LOGICAL_OR_EXPRESSION>
 *		| <LOGICAL_OR_EXPRESSION> QUESTION <EXPRESSION> COLUMN <CONDITIONAL_EXPRESSION>
 *		;
 */
Number PreprocessorExpParser::evaluateConditionalExp(NonTerminal *nt) const {
	assert(nt->getNonTerminalId() == PREPROCEXPPARSERBUFFER_NONTERMINAL_CONDITIONAL_EXPRESSION);
	
	Number result;
	
	if (nt->getNonTerminalRule() == 0) {
		result = evaluateLogicalOrExp(nt->getNonTerminalAt(0));
	}
	else {
		assert(nt->getNonTerminalRule() == 1);
		
		Number condition = evaluateLogicalOrExp(nt->getNonTerminalAt(0));
		
		if (condition.boolValue()) result = evaluateExpression(nt->getNonTerminalAt(2));
		else result = evaluateConditionalExp(nt->getNonTerminalAt(4));
	}
	
	return result;
}
