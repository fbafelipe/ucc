#include "compiler/StaticMemory.h"

#include "vm/RegisterUtils.h"

#include <cassert>
#include <cstring>

StaticMemory::StaticMemory() {}

unsigned int StaticMemory::allocate(unsigned int size) {
	unsigned int pos = memory.size();
	memory.resize(memory.size() + size, 0);
	return pos;
}

void StaticMemory::initialize(unsigned int pos, const void *data, unsigned int size) {
	memcpy(&memory[pos], data, size);
}

const StaticMemory::Memory & StaticMemory::getMemory() const {
	return memory;
}
