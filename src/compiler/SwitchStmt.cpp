#include "compiler/SwitchStmt.h"

#include <cassert>

/*****************************************************************************
 * SwitchStmt::Case
 *****************************************************************************/
SwitchStmt::Case::Case() : offset(0) {}

SwitchStmt::Case::Case(unsigned int off, const Number & val) : offset(off), value(val) {}

SwitchStmt::Case::~Case() {}

unsigned int SwitchStmt::Case::getOffset() const {
	return offset;
}

void SwitchStmt::Case::setOffset(unsigned int off) {
	offset = off;
}

const Number & SwitchStmt::Case::getValue() const {
	return value;
}

void SwitchStmt::Case::setNumber(const Number & val) {
	value = val;
}

/*****************************************************************************
 * SwitchStmt
 *****************************************************************************/
SwitchStmt::SwitchStmt() : defaultOffset(0) {}

SwitchStmt::~SwitchStmt() {
	for (CaseList::iterator it = cases.begin(); it != cases.end(); ++it) {
		delete(*it);
	}
}

void SwitchStmt::addCase(Case *c) {
	cases.push_back(c);
	caseMap[c->getValue()] = c;
}

SwitchStmt::Case *SwitchStmt::getCase(const Number & val) const {
	CaseMap::const_iterator it = caseMap.find(val);
	
	assert(it != caseMap.end());
	
	return it->second;
}

SwitchStmt::Case *SwitchStmt::getCase(int val) const {
	return getCase(Number(Number::INT, (long long int)val));
}

const SwitchStmt::CaseList & SwitchStmt::getCases() const {
	return cases;
}

unsigned int SwitchStmt::getCasesCount() const {
	return cases.size();
}

bool SwitchStmt::isSequential() const {
	if (caseMap.empty()) {
		// it is sequencial, but getBounds cannot be called with an empty switch
		return false;
	}
	
	CaseMap::const_iterator it = caseMap.begin();
	
	Number v = it->first;
	if (v.isFloat()) return false;
	
	for (++it; it != caseMap.end(); ++it) {
		Number num = it->first;
		
		if (num.isFloat()) return false;
		if (v.intValue() + 1 != num.intValue()) return false;
		
		v = it->first;
	}
	
	return true;
}

void SwitchStmt::getBounds(int *min, int *max) const {
	assert(isSequential());
	
	*min = caseMap.begin()->first.intValue();
	*max = caseMap.rbegin()->first.intValue();
	
	assert(*min <= *max);
}

unsigned int SwitchStmt::getDefaultOffset() const {
	return defaultOffset;
}

void SwitchStmt::setDefaultOffset(unsigned int off) {
	defaultOffset = off;
}
