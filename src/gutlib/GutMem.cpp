#include <stdlib.h>

static unsigned int g_MemoryAllocationAlignment = 16;
static unsigned int g_MemoryAllocationMask = ~(16-1);

void GutAllocateMemoryAlignment(int alignment)
{
	// alignment should be power 2
	g_MemoryAllocationAlignment = alignment;
	g_MemoryAllocationMask = ~(alignment-1);
}

// 分配一块内存空间, 并确保返回的内存位址是g_MemoryAllocationAlignment的倍数
void *GutAllocateAlignedMemory(size_t size)
{
	if ( size==0 ) return NULL;
	//unsigned char *block = new unsigned char[size + g_MemoryAllocationAlignment];
	unsigned char *block = (unsigned char *)malloc(size + g_MemoryAllocationAlignment);
	if ( block )
	{
		unsigned int *aligned = (unsigned int*)((unsigned int)(block + g_MemoryAllocationAlignment + 4) & g_MemoryAllocationMask);
		aligned[-1] = (unsigned int) block;
		return aligned;
	}
	else
	{
		return NULL;
	}
}

// 释放通过GutAllocateAlignedMemory分配的内存空间
void GutReleaseAlignedMemory(void *memory)
{
	if ( memory==NULL ) return;

	unsigned int *pMemory = (unsigned int *)memory;
	delete (unsigned int *)pMemory[-1];
}

// 分配一块内存空间, 并确保返回的内存位址是16 bytes的倍数
void *GutAllocate16BytesAlignedMemory(size_t size)
{
	/*
	unsigned char *block = new unsigned char[size + 16];
	unsigned int *aligned = (unsigned int*)((unsigned int)(block + 20) & 0xFFFFFFF0);
	aligned[-1] = (unsigned int) block;
	return aligned;
	*/

	GutAllocateMemoryAlignment(16);
	return GutAllocateAlignedMemory(size);
}

// 释放通过GutAllocate16BytesAlignedMemory分配的内存空间
void GutRelease16BytesAlignedMemory(void *memory)
{
	GutReleaseAlignedMemory(memory);
}

void *operator new[](size_t size)
{
	return GutAllocateAlignedMemory(size);
}

void operator delete[](void *p)
{
	GutReleaseAlignedMemory(p);
}