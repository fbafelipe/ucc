#ifndef SWITCH_STMT_H
#define SWITCH_STMT_H

#include "Number.h"

#include <map>
#include <vector>

class SwitchStmt {
	public:
		class Case {
			public:
				Case();
				Case(unsigned int off, const Number & val);
				~Case();
				
				unsigned int getOffset() const;
				void setOffset(unsigned int off);
				
				const Number & getValue() const;
				void setNumber(const Number & val);
				
			private:
				unsigned int offset;
				Number value;
		};
		typedef std::vector<Case *> CaseList;
		typedef std::map<Number, Case *> CaseMap;
		
		SwitchStmt();
		~SwitchStmt();
		
		void addCase(Case *c);
		Case *getCase(const Number & val) const;
		Case *getCase(int val) const;
		const CaseList & getCases() const;
		unsigned int getCasesCount() const;
		
		bool isSequential() const;
		void getBounds(int *min, int *max) const;
		
		unsigned int getDefaultOffset() const;
		void setDefaultOffset(unsigned int off);
		
	private:
		CaseList cases;
		CaseMap caseMap;
		
		// if not present it's the offset of the entire switch
		unsigned int defaultOffset;
};

#endif
