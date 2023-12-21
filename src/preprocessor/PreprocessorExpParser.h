#ifndef PREPROCESSOR_EXPRESSION_H
#define PREPROCESSOR_EXPRESSION_H

#include "Number.h"

#include <parser/ParserTable.h>
#include <parser/ParsingTree.h>
#include <parser/Pointer.h>
#include <parser/ScannerAutomata.h>

class DefineMap;
class Preprocessor;

class PreprocessorExpParser {
	public:
		PreprocessorExpParser(const Preprocessor *preproc, const DefineMap & defMap);
		
		bool parseExp(Input *exp) const;
		
	private:
		typedef ParsingTree::Node Node;
		typedef ParsingTree::NonTerminal NonTerminal;
		typedef ParsingTree::Token Token;
		
		Number evaluateExpression(NonTerminal *nt) const;
		Number evaluatePrimaryExp(NonTerminal *nt) const;
		Number evaluateUnaryExp(NonTerminal *nt) const;
		Number evaluateUnaryOperator(NonTerminal *nt, Number num) const;
		Number evaluateMultiplicativeExp(NonTerminal *nt) const;
		Number evaluateAdditiveExp(NonTerminal *nt) const;
		Number evaluateShiftExp(NonTerminal *nt) const;
		Number evaluateRelationExp(NonTerminal *nt) const;
		Number evaluateEqualityExp(NonTerminal *nt) const;
		Number evaluateAndExp(NonTerminal *nt) const;
		Number evaluateExclusiveOrExp(NonTerminal *nt) const;
		Number evaluateInclusiveOrExp(NonTerminal *nt) const;
		Number evaluateLogicalAndExp(NonTerminal *nt) const;
		Number evaluateLogicalOrExp(NonTerminal *nt) const;
		Number evaluateConditionalExp(NonTerminal *nt) const;
		
		Pointer<ScannerAutomata> scannerAutomata;
		Pointer<ParserTable> parserTable;
		
		const DefineMap & defineMap;
};

#endif
