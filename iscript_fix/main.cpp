#include "iscript.h"
#include "resource.h"

#include <cstdio>
#include <cassert>

#include <string>
#include <sstream>
#include <fstream>

#include <vector>
#include <set>

#include <algorithm>

#include <Windows.h>

std::string GetResource(LPCTSTR fname)
{
	HMODULE hModule = GetModuleHandle(NULL);
	HRSRC hRes = FindResource(hModule, fname, RT_RCDATA);
	HGLOBAL hMem = LoadResource(hModule, hRes);
	LPVOID lpResource = LockResource(hMem);
	DWORD size = SizeofResource(hModule, hRes);
	std::string retdata((char*)lpResource, size);
	FreeResource(hMem);
	return retdata;
}

int main(int argc, char* argv[])
{
	if(argc == 1)
	{
		printf("Usage : iscript_fix [input file]\n");
		return -1;
	}

	// Read original iscript
	printf("[1] Reading original iscript.\n");
	std::string origisc_data = GetResource(MAKEINTRESOURCE(IDR_RCDATA1));
	std::istringstream iss(origisc_data);
	IScript origisc(iss);
	printf("\n");

	// Collect originally used iscript entry IDs.
	std::vector<uint16_t> origisc_idv = origisc.EnumEntries();
	std::set<uint16_t> origisc_ids(origisc_idv.begin(), origisc_idv.end());
	origisc_idv.clear();


	// Read user iscript
	printf("[2] Reading custom iscript.\n");
	std::string ifname = argv[1];
	std::string ofname = ifname.substr(0, ifname.size() - 4) + " fixed.bin";
	IScript userisc(std::ifstream(ifname, std::ifstream::binary));
	printf("\n");

	// Collect custom used iscript entry IDs.
	std::vector<uint16_t> userisc_idv = userisc.EnumEntries();
	std::set<uint16_t> userisc_ids(userisc_idv.begin(), userisc_idv.end());
	userisc_idv.clear();


	std::set<uint16_t> ids_merge, ids_diff;
	// Merge
	std::set_union(
		userisc_ids.begin(), userisc_ids.end(),
		origisc_ids.begin(), origisc_ids.end(),
		std::inserter(ids_merge, ids_merge.begin())
		);

	// Diff
	std::set_difference(
		userisc_ids.begin(), userisc_ids.end(),
		origisc_ids.begin(), origisc_ids.end(),
		std::inserter(ids_diff, ids_diff.begin())
		);



	// Allocate opcodes.
	printf("[3] Allocating custom opcodes.\n");
	IScriptDependency isd;
	for(uint16_t entryID : ids_diff)
	{
		printf("\r - Updating depencency of entry %d...", entryID);
		userisc.UpdateDependency(entryID, &isd);
	}

	uint16_t origdataend = *((uint16_t*)origisc_data.data());
	uint32_t alloc_addr = origdataend;
	int chkid = 0, chkn = isd.chkSet.size();
	for(OpcodeChunk* chk : isd.chkSet)
	{
		printf("\r - Allocating chunk %d/%d...", ++chkid, chkn);
		for(Opcode* opc : chk->opcodes)
		{
			opc->allocated_offset = alloc_addr;
			alloc_addr += opc->size;
		}
	}

	// Allocate entries.
	printf("\n[4] Allocating iscript entries.\n");
	std::map<uint16_t, uint16_t> iscEntry_allocaddr;
	for(uint16_t entryID : ids_diff)
	{
		IScriptEntry* iscEntry = userisc.GetEntry(entryID);
		iscEntry_allocaddr[entryID] = alloc_addr;
		alloc_addr += 8 + iscEntry->opcodelist.size() * 2;
	}

	// Allocate entry table.
	alloc_addr += ids_merge.size() * 4 + 4;

	if(alloc_addr > 0x10000)  // iscript.bin too big
	{
		printf("\n[Error] iscript.bin overflow.\n");
		std::abort();
	}



	// Write payload.
	printf("[5] Writing payload.\n");
	std::vector<uint8_t> finalisc_data(alloc_addr);
	uint8_t* datastart = finalisc_data.data();
	uint8_t* datacur = datastart;

	// Write original data
	memcpy(datacur, origisc_data.data(), origdataend);
	datacur += origdataend;

	// Write user opcodes
	for(OpcodeChunk* chk : isd.chkSet)
	{
		for(Opcode* opc : chk->opcodes)
		{
			memcpy(datacur, opc->plaindata.data(), opc->size);
			if(opc->pointer.ptr)
			{
				Opcode* pointee = opc->pointer.ptr;
				memcpy(
					datacur + opc->pointer.arg_offset,
					&pointee->allocated_offset,
					2
					);
			}
			datacur += opc->size;;
		}
	}

	// Write user iscript entries.
	for(uint16_t entryID : ids_diff)
	{
		IScriptEntry* iscEntry = userisc.GetEntry(entryID);
		memcpy(datacur, "SCPE", 4); datacur += 4;
		*datacur = iscEntry->type; datacur++;
		*datacur = 0; datacur++;
		*datacur = 0; datacur++;
		*datacur = 0; datacur++;

		for(Opcode* opc : iscEntry->opcodelist)
		{
			if(opc)
			{
				memcpy(datacur, &opc->allocated_offset, 2);
				datacur += 2;
			}
			else
			{
				*datacur = 0; datacur++;
				*datacur = 0; datacur++;
			}
		}
	}

	// Write iscript tables.
	uint16_t isc_entrytb_offset = datacur - datastart;
	memcpy(datastart, &isc_entrytb_offset, 2);

	uint16_t origisctblen = origisc_data.size() - origdataend - 4;
	memcpy(
		datacur,
		origisc_data.data() + origdataend,
		origisctblen
		);
	datacur += origisctblen;

	for(uint16_t entryID : ids_diff)
	{
		uint16_t entryOffset = iscEntry_allocaddr[entryID];
		memcpy(datacur, &entryID, 2); datacur += 2;
		memcpy(datacur, &entryOffset, 2); datacur += 2;
	}

	memcpy(datacur, "\xFF\xFF\x00\x00", 4); datacur += 4;
	assert(datacur - datastart == alloc_addr);

	std::ofstream os(ofname, std::ofstream::binary);
	os.write((const char*)datastart, alloc_addr);
	os.close();

	printf("[6] Done!\n");

	return 0;
}
