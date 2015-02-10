#include <cassert>
#include <cstdio>
#include <stack>
#include <set>

#include <fstream>

#include "iscript.h"

std::map<uint32_t, uint32_t> entryType_opcodeNum_map = {
	{ 0, 2 },
	{ 1, 2 },
	{ 2, 4 },
	{ 12, 14 },
	{ 13, 14 },
	{ 14, 16 },
	{ 15, 16 },
	{ 20, 22 },
	{ 21, 22 },
	{ 23, 24 },
	{ 24, 26 },
	{ 26, 28 },
	{ 27, 28 },
	{ 28, 28 },
	{ 29, 28 },
};

void GetOpcode(std::istream& is, Opcode* opc);

IScript::IScript(std::istream& is)
{
	// Opcodes
	Opcode* opcodeMap[65536];
	for(int i = 0; i < 65536; i++)
	{
		Opcode* p = new Opcode;
		memset(p, 0, sizeof(Opcode));
		p->size = 0xFFFF;
		opcodeMap[i] = p;
	}
	
	// -----------------------------------------------------------------------

	// Locate entry list
	uint16_t entrylistoffset;
	is.seekg(0);
	is.read((char*)&entrylistoffset, 2);
	is.seekg(entrylistoffset);

	// Get list of entries.
	std::map<uint16_t, uint16_t> entry_offset_map;
	printf("Getting list of iscript entries...\n");
	while(1)
	{
		uint16_t entryID, entryOffset;
		is.read((char*)&entryID, 2);
		if(entryID == 0xFFFF) break;  // End of list
		is.read((char*)&entryOffset, 2);
		entry_offset_map.insert(std::make_pair(entryID, entryOffset));
		printf("\r - Entry : Id %5d, Offset %5d", entryID, entryOffset);
	}

	// From opcode entries, get opcode parse start offsets.
	std::stack<uint16_t> opcodeParseStartOffsets;

	printf("\nGetting decompile starting points...\n");
	for(auto& entry_offset : entry_offset_map)
	{
		uint16_t entryID = entry_offset.first;
		uint16_t entryOffset = entry_offset.second;
		IScriptEntry* isce = new IScriptEntry;
		_entries.insert(std::make_pair(entryID, isce));

		is.seekg(entryOffset);

		uint32_t magic, entryType;
		is.read((char*)&magic, 4);
		assert(magic == 'EPCS');  // Magic number check.

		is.read((char*)&entryType, 4);
		int opcodeNum = entryType_opcodeNum_map[entryType];
		isce->type = entryType;

		printf("\r - Entry : Id %5d, Type %d  ", entryID, entryType);
		for(int i = 0; i < opcodeNum; i++)
		{
			uint16_t opcParseReqOffset;
			is.read((char*)&opcParseReqOffset, 2);

			if(opcParseReqOffset)  // There is starting point
			{
				isce->opcodelist.push_back(opcodeMap[opcParseReqOffset]);
				// Queue parse from the offset
				opcodeParseStartOffsets.push(opcParseReqOffset);
			}
			else
			{
				isce->opcodelist.push_back(nullptr);
			}
		}
	}

	// Parse all opcodes and required things.
	printf("\nDecoding opcodes [0]");
	int decoded_opcode_num = 0; 


	while(!opcodeParseStartOffsets.empty())
	{
		uint16_t opcodeOffset = opcodeParseStartOffsets.top();
		opcodeParseStartOffsets.pop();
		if(opcodeMap[opcodeOffset]->size != 0xFFFF)  // Already parsed.
			continue;

		Opcode* opc = opcodeMap[opcodeOffset];
		is.seekg(opcodeOffset);
		GetOpcode(is, opc);
		if(opc->pointer.ptr != nullptr)  // Pointer detected
		{
			uint16_t pOpcOffset = reinterpret_cast<uint16_t>(opc->pointer.ptr);
			// Translate to real pointer.
			opc->pointer.ptr =
				opcodeMap[pOpcOffset];
			opcodeParseStartOffsets.push(pOpcOffset);
		}

		// Queue parse of next opcode.
		uint8_t opcodeType = opc->plaindata[0];
		if(!(
			opcodeType == 0x07 ||  // goto
			opcodeType == 0x16 ||  // end
			opcodeType == 0x36     // return
			))
		{
			opcodeParseStartOffsets.push(opcodeOffset + opc->size);
		}

		decoded_opcode_num++;
		printf("\rDecoding opcodes [%d]", decoded_opcode_num);
	}


	// Intermediate cleanup : free unused offsets
	printf("\nPost-processing iscripts...\n");
	for(int i = 0; i < 65536; i++)
	{
		if(opcodeMap[i]->size == 0xFFFF)
		{
			delete opcodeMap[i];
			opcodeMap[i] = nullptr;
		}
	}

	// Link adjacent opcodes.
	Opcode* prevOpc = nullptr;
	OpcodeChunk* chk = nullptr;
	int chkn = 0;
	for(int off = 0; off < 65536;)
	{
		Opcode* opc = opcodeMap[off];
		if(opc == nullptr)
		{
			prevOpc = nullptr;
			chk = nullptr;
			off++;
			continue;
		}
		
		// Append to opcode chunk.
		if(chk == nullptr)
		{
			chk = new OpcodeChunk;
			chk->allocated_offset = 0xFFFF;
			chk->size = 0;
			chkn++;
		}

		chk->opcodes.push_back(opc);
		chk->size += opc->size;
		opc->parent = chk;

		if(prevOpc)
		{
			opc->prev = prevOpc;
			prevOpc->next = opc;
		}

		uint8_t opcodeType = opc->plaindata[0];
		if(!(
			opcodeType == 0x07 ||  // goto
			opcodeType == 0x16 ||  // end
			opcodeType == 0x36     // return
			))
		{
			prevOpc = opc;
		}
		else
		{
			prevOpc = nullptr;
			chk = nullptr;
		}
		
		off += opc->size;
	}

	for(int off = 0; off < 65536; off++)
	{
		Opcode* opc = opcodeMap[off];
	}

	printf("Iscript reading complete!\n");
	printf(" - Total number of iscript chunks : %d\n", chkn);
}

IScriptEntry* IScript::GetEntry(uint16_t entryID)
{
	return _entries[entryID];
}

const IScriptEntry* IScript::GetEntry(uint16_t entryID) const
{
	return _entries.find(entryID)->second;
}

std::vector<uint16_t> IScript::EnumEntries() const
{
	std::vector<uint16_t> entryIDSet;
	for(auto& it : _entries) entryIDSet.push_back(it.first);
	return entryIDSet;
}

void IScript::UpdateDependency(uint16_t entryID, IScriptDependency* isd) const
{
	std::stack<Opcode*> opcStack;
	const IScriptEntry* entry = GetEntry(entryID);
	for(Opcode* opcode : entry->opcodelist)
	{
		opcStack.push(opcode);
	}

	while(!opcStack.empty())
	{
		Opcode* opcode = opcStack.top();
		opcStack.pop();
		if(opcode == nullptr) continue;
		else if(isd->opcSet.find(opcode) != isd->opcSet.end()) continue;

		opcStack.push(opcode->next);
		opcStack.push(opcode->pointer.ptr);

		isd->opcSet.insert(opcode);
		isd->chkSet.insert(opcode->parent);
	}
}

IScript::~IScript()
{
	IScriptDependency isd;

	// Collect opcodes & chunks
	for(auto& it : _entries)
	{
		UpdateDependency(it.first, &isd);
		delete it.second;
	}

	for(Opcode* opc : isd.opcSet) delete opc;
	for(OpcodeChunk* opcChk : isd.chkSet) delete opcChk;
}
