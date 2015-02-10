#include "MemoryPool.h"
#include <stdio.h>
#include <memory.h>
#include <malloc.h>


// memory block size (can hold MP_OBJ_PER_BLOCK objects of same size)
const int MP_OBJ_PER_BLOCK = 128;

// 'address of next cell' size
const int ANC_SIZE = sizeof(void*);

char* NOT_ASSIGNED = (char*)1;


MemoryPool& st_MemoryPool()
{ // There need to be only one memory pool, which is common to any required memory size.
	static MemoryPool memoryPool;
	return memoryPool;
}

MemoryPool::MemoryPool()
{
	for (int i=0; i<OBJ_SIZE_LIMIT; ++i)
		nextFreeCellMap[i] = NOT_ASSIGNED;
	vBlockHeads.reserve(OBJ_SIZE_LIMIT);
}

void* MemoryPool::newMem(size_t objSize)
{
	if (objSize >= OBJ_SIZE_LIMIT)
	{
		// refuse to create a memory block
		return ::operator new(objSize);
	}

	// * A cell consists of 'pointer to next free cell' and 'raw memory for the object'
	int cellSize = ANC_SIZE+objSize;

	byte* ret;
	byte* nextAddress;
	byte*& nextFreeCell = nextFreeCellMap[objSize];

	if (nextFreeCell == NOT_ASSIGNED || // no block assigned for this objectSize yet,
		nextFreeCell == 0) // or current block is full.
	{
		// Get new(another) memory block.
		// * A block consists of MP_OBJ_PER_BLOCK cells
		byte* newBlock = (byte*)malloc(MP_OBJ_PER_BLOCK * cellSize);

		// Remembering the block address - need to delete it when the program ends
		vBlockHeads.push_back(newBlock);

		for (int i=1; i<MP_OBJ_PER_BLOCK-1; ++i)
		{ // Form a linked list with memory cells
			nextAddress = newBlock + (i+1)*cellSize;
			memcpy(newBlock+i*cellSize, &nextAddress, ANC_SIZE);
		}
		nextAddress = NULL; // The last link is NULL, like the typical linked list.
		memcpy(newBlock+(MP_OBJ_PER_BLOCK-1)*cellSize, &nextAddress, ANC_SIZE);
		ret = newBlock + ANC_SIZE;
		nextFreeCellMap[objSize] = newBlock + cellSize;
	}
	else
	{ // There is a free cell left. Just use it.
		ret = nextFreeCell + ANC_SIZE;
		memcpy(&nextAddress, nextFreeCell, ANC_SIZE); // read ANC from nextFreeCell
		nextFreeCell = nextAddress; // nextFreeCell = nextFreeCell->next(ANC)
	}
	return (void*)(ret);
}

void MemoryPool::deleteMem(void* pDeadObject, size_t size)
{
	if (pDeadObject == NULL) return;

	byte*& nextFreeCell = nextFreeCellMap[size];

	if (nextFreeCell == NOT_ASSIGNED ||
		size >= OBJ_SIZE_LIMIT)
	{ // This memory does not belong to the memory pool! (user mistake?)
		::operator delete(pDeadObject); // Apply default delete operation.
		return;
	}

	byte* cellAdress = (byte*)pDeadObject-ANC_SIZE; // Step 1) find ANC section

	// Step 2) Write current nextFreeCell address on the ANC section
	memcpy(cellAdress, &nextFreeCell, ANC_SIZE);

	// Step 3)
	// Next memory allocation will take place here, the last deallocated memory cell.
	// And would be linked to previous nextFreeCell address.. (we linked it in Step 2)
	nextFreeCell = cellAdress;
}

MemoryPool::~MemoryPool()
{
	// Delete all memory blocks. Bye bye.
	vector<byte*>::iterator vi;
	for (vi=vBlockHeads.begin(); vi!=vBlockHeads.end(); ++vi)
		free(*vi);
}

void* MemoryPooled::operator new(size_t size)
{
	if (size==0) size = 1; // standard error handling

	return st_MemoryPool().newMem(size);
}

void MemoryPooled::operator delete(void* deadObject, size_t size)
{
	st_MemoryPool().deleteMem(deadObject, size);
}
