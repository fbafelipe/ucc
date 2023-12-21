#ifndef INPUT_WRAPPER_H
#define INPUT_WRAPPER_H

#include <parser/InputLocation.h>

#include <functional>
#include <map>
#include <string>

class Input;

class InputWrapper {
	public:
		InputWrapper(Input *in);
		~InputWrapper();
		
		void setLocation(const std::string & name, unsigned int line);
		
		InputLocation convertLocation(const InputLocation & loc) const;
		InputLocation currentLocation() const;
		
	private:
		class WrapPoint {
			public:
				WrapPoint();
				WrapPoint(const InputLocation & f, const InputLocation & t);
				
				const InputLocation & getFrom() const;
				const InputLocation & getTo() const;
				
			private:
				InputLocation from;
				InputLocation to;
		};
		// the map is reverse
		typedef std::map<unsigned int, WrapPoint, std::greater<unsigned int> > WrapMap;
		
		Input *input;
		
		WrapMap wraps;
};

#endif
