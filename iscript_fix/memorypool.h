#ifndef MEMORYPOOL_H
#define MEMORYPOOL_H

#include <vector>
using namespace std;

/* +------------------------------------------------------+
   | A Slimmest Global Memory Pool                        |
   |  supported by an address chaining algorithm          |
   |  illustrated in Effective C++(a book by Scott Meyers)|
   |                                                      |
   | warning: THIS IS NOT THREAD-SAFE!                    |
   |                                                      |
   | Author : Remisa (itioma@naver.com)                   |
   | Homepage : blog.naver.com/itioma                     |
   | Last modification : 2006. 12. 17.                    |
   +------------------------------------------------------+ */
  
/*
  Description)

  class GameObject // You want this class to use the MemoryPool.
  {
      ....
  };

  class GameObject : public MemoryPooled // Now it uses the MemoryPool. Quite simple isn't it?
  {
      ....
  };

  class Airplane : public GameObject // Inherited classes use the MemoryPool also.
  {
      ....
  };

  GameObject* po = new Airplane; // the required memory will be come from the MemoryPool.
  delete po; // the unused memory will return back to the MemoryPool.

  Pool deallocation is automatically done with the MemoryPool
    when the program ends.
*/


// This damn warning cannot be removed unless
//  YOU change your compile option (disable C++ Exception)
//  or YOU define another operator delete before function main.
// So it was shot down by #pragma. Don't bother it.
#pragma warning ( disable: 4291 )


// A Brand New Memory Pool
class MemoryPool
{
public:
	// The pool does not accept objects of which sizes are greater than 1k byte.
	// Default operator new is selected for those situation.
	enum { OBJ_SIZE_LIMIT = 1024 }; 

	typedef char byte;

	void* newMem(size_t size);
	void deleteMem(void* deadObject, size_t size);

	~MemoryPool();

friend MemoryPool& st_MemoryPool();
private:
	MemoryPool();

	// I first tried to use std::map for these.
	// but after checking performance, I found std::map was much slower than
	// simple array like this.
	// Regard there would be 100000+ new/delete in the memory pool.
	byte* nextFreeCellMap[OBJ_SIZE_LIMIT];
	vector<byte*> vBlockHeads;
};

// Represents poor little children who needs a Memory Pool.
class MemoryPooled
{
public:
	void* operator new(size_t size);
	void operator delete(void* pDeadObject, size_t size);
};

#endif