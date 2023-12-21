#ifndef STATIC_MEMORY_H
#define STATIC_MEMORY_H

#include <parser/Pointer.h>

#include <vector>

class StaticMemory {
	public:
		typedef std::vector<unsigned char> Memory;
		
		StaticMemory();
		
		unsigned int allocate(unsigned int size);
		void initialize(unsigned int pos, const void *data, unsigned int size);
		
		const Memory & getMemory() const;
		
	private:
		Memory memory;
};

typedef std::vector<Pointer<StaticMemory> > StaticMemoryList;

#endif
