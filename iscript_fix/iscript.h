#pragma once

#ifndef ISCRIPT_HEADER_
#define ISCRIPT_HEADER_

#include <cstdint>

#include <map>
#include <set>
#include <vector>
#include <istream>

#include "iscript_opcode.h"

struct IScriptEntry
{
	uint8_t type;
	std::vector<Opcode*> opcodelist;
};

struct IScriptDependency
{
	std::set<Opcode*> opcSet;
	std::set<OpcodeChunk*> chkSet;
};

class IScript
{
public:
	IScript(std::istream& is);
	~IScript();

	std::vector<uint16_t> EnumEntries() const;
	IScriptEntry* GetEntry(uint16_t entryID);
	const IScriptEntry* GetEntry(uint16_t entryID) const;
	void UpdateDependency(uint16_t entryID, IScriptDependency* isd) const;

private:
	std::map<uint16_t, IScriptEntry*> _entries;
};

#endif
