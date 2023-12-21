#include "preprocessor/InputWrapper.h"

#include <parser/Input.h>

#include <cassert>

/*****************************************************************************
 * InputWrapper::WrapPoint
 *****************************************************************************/
InputWrapper::WrapPoint::WrapPoint() {}

InputWrapper::WrapPoint::WrapPoint(const InputLocation & f,
		const InputLocation & t) : from(f), to(t) {}

const InputLocation & InputWrapper::WrapPoint::getFrom() const {
	return from;
}

const InputLocation & InputWrapper::WrapPoint::getTo() const {
	return to;
}

/*****************************************************************************
 * InputWrapper
 *****************************************************************************/
InputWrapper::InputWrapper(Input *in) : input(in) {
	assert(input);
}

InputWrapper::~InputWrapper() {}
void InputWrapper::setLocation(const std::string & name, unsigned int line) {
	wraps[input->getInputLine()] = WrapPoint(input->getCurrentLocation(), InputLocation(name, line, 0));
}

InputLocation InputWrapper::convertLocation(const InputLocation & loc) const {
	WrapMap::const_iterator it = wraps.lower_bound(loc.getLine());
	
	// no wrap was set yet, the location don't need any change
	if (it == wraps.end()) return loc;
	
	const WrapPoint & w = it->second;
	
	const InputLocation & from = w.getFrom();
	const InputLocation & to = w.getTo();
	
	return InputLocation(to.getName(),
			to.getLine() + loc.getLine() - from.getLine(),
			loc.getColumn());
}

InputLocation InputWrapper::currentLocation() const {
	return convertLocation(input->getCurrentLocation());
}
