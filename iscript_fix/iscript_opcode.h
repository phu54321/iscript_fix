#pragma once

/*
Opcode structure for iscript.
*/

#ifndef ISCIRPT_OPCODE_HEADER_
#define ISCIRPT_OPCODE_HEADER_

#include <cstdint>
#include <cstdio>
#include <vector>

/*
Every iscript opcode consists of
 - opcode type
 - data

Data may have pointers (offset value) inside.

Opcode may have explicit 'previous' or 'next' opcode.

 A  ->                 A.next = B
 B  -> B.prev = A      B.next = C
 C  -> C.prev = B
*/

struct Opcode;
struct OpcodeChunk;

struct PtrArg  // Reference to other pointer
{
	Opcode* ptr;
	uint16_t arg_offset;  // Offset of ptrarg inside opcode
	// length of file offset is always 2byte.
};

struct Opcode
{
	OpcodeChunk* parent;

	Opcode *prev, *next;  // Previous & Next opcode according to file.
	uint16_t size;
	std::vector<uint8_t> plaindata;  // opcode type + plain datas all combined
	PtrArg pointer;

	uint16_t allocated_offset;  // Where opcode is allocated.
};

/*
Several opcodes may join together to make a 'chunk'. Opcodes inside chunk
operates within them. Chunks can be shuffled as they wish.

Final opcode offsets are allocated by its belonging chunks.
*/

struct OpcodeChunk
{
	std::vector<Opcode*> opcodes;
	uint16_t size;
	uint16_t allocated_offset;
};

#endif
