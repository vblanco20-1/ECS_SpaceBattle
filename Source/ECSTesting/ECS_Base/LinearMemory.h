#pragma once


//linear "vector" type thing, can be reset instantly to whatever

struct LinearMemory {
	void * pointer;
	size_t size;
};


constexpr size_t KILOBYTE = 1024;
constexpr size_t MEGABYTE = 1024 * 1024;
constexpr size_t GIGABYTE = 1024 * 1024 * 1024;

static void FreeLinearMemory(const LinearMemory& mem)
{
	free(mem.pointer);
}
static LinearMemory AllocateLinearMemory(size_t size)
{
	LinearMemory m;
	m.size = size;
	m.pointer = malloc(size);
	return m;
}

template<typename T>
struct TypedLinearMemory {

	TypedLinearMemory(LinearMemory & mem) {
		pointer = static_cast<T*>(mem.pointer);
		
		last = 0;
	}

	void push_back(const T&Item)
	{
		pointer[last] = Item;
		last++;
	}
	T &operator[](size_t i){
		return pointer[i];
	}

	T * pointer;
	size_t last;	

};

