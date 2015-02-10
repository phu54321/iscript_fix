#include <stdio.h>
#include "iscript.h"
#include <fstream>

int main()
{
	IScript is(std::ifstream("iscript.bin",
		std::ifstream::in | std::ifstream::binary));
	getchar();
	return 0;
}
