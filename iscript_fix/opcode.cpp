#include "iscript_opcode.h"
#include <istream>

struct
{
	int opcodeLength;  // -1: variable-length
	int opcodePtr;  // 0: no ptr
} opcdata[] = {
	{ 3 },
	{ 3 },
	{ 2 },
	{ 2 },
	{ 3 },
	{ 2 },
	{ 3 },
	{ 3, 1 },
	{ 5 },
	{ 5 },
	{ 3 },
	{ 3 },
	{ 1 },
	{ 5 },
	{ 5 },
	{ 5 },
	{ 5 },
	{ 5 },
	{ 3 },
	{ 5 },
	{ 5 },
	{ 4 },
	{ 1 },
	{ 2 },
	{ 3 },
	{ -1 },  // Variable-length 
	{ 5 },
	{ 1 },
	{ -1 },  // Variable-length
	{ 1 },
	{ 4 },
	{ 2 },
	{ 2 },
	{ 1 },
	{ 2 },
	{ 2 },
	{ 2 },
	{ 2 },
	{ 1 },
	{ 1 },
	{ 2 },
	{ 2 },
	{ 1 },
	{ 2 },
	{ 2 },
	{ 1 },
	{ 1 },
	{ 1 },
	{ 1 },
	{ 2 },
	{ 1 },
	{ 1 },
	{ 2 },
	{ 3, 1 },
	{ 1 },
	{ 3 },
	{ 2 },
	{ 3, 1 },
	{ 5, 3 },
	{ 7, 5 },
	{ 7, 5 },
	{ 3 },
	{ 1 },
	{ 3, 1 },
	{ 3 },
	{ 2 },
	{ 5 },
	{ 1 },
	{ 1 },
};

Opcode* GetOpcode(std::istream& is)
{
	Opcode* opc = new Opcode;
	memset(opc, 0, sizeof(Opcode));

	opc->parent = nullptr;
	opc->prev = nullptr;
	opc->next = nullptr;
	opc->allocated_offset = 0xFFFF;


	// Get opcode type
	char opcType;
	is.read(&opcType, 1);

	if(opcType < 0 || opcType > 0x44)
	{
		printf("\n[Error] Invalid opcode 0x%02x at %d.\n",
			opcType & 0xFF, is.tellg().seekpos());
		std::abort();
	}


	// Read entire opcode data
	char opcodedata[1024];
	opcodedata[0] = opcType;

	int opcLength = opcdata[opcType].opcodeLength;
	if(opcLength == -1)  // Variable-length opcode
	{
		uint8_t shortn;
		is.read((char*)&shortn, 1);
		opcLength = 1 + 1 + 2 * shortn;
		is.seekg(-2, is.cur);
	}
	else is.seekg(-1, is.cur);
	
	opc->size = opcLength;
	opc->plaindata.resize(opcLength);
	is.read((char*)opc->plaindata.data(), opcLength);


	// Read ptr data
	int ptrPos = opcdata[opcType].opcodePtr;
	if(ptrPos)  // Opcode has pointer information -> Read it.
	{
		uint16_t ptrdata;
		is.seekg(ptrPos - opcLength, is.cur);
		is.read((char*)&ptrdata, 2);
		opc->pointer.ptr = reinterpret_cast<Opcode*>(ptrdata);
		opc->pointer.arg_offset = ptrPos;
	}
	else
	{
		opc->pointer.ptr = nullptr;
	}

	return opc;
}
