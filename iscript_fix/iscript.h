#pragma once

#ifndef ISCRIPT_HEADER_
#define ISCRIPT_HEADER_

#include <cstdint>

#include <map>
#include <vector>
#include <istream>

#include "iscript_opcode.h"

struct IScriptEntry
{
	uint8_t type;
	std::vector<Opcode*> opcodelist;
};

class IScript
{
public:
	IScript(std::istream& is);
	~IScript();

	const IScriptEntry* GetEntry(uint16_t entryID);

private:
	std::map<uint16_t, IScriptEntry*> _entries;
};

#endif
