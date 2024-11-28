#pragma once

#include <vector>

class MemoryManager {
public:
	MemoryManager(long long maxOverallMem);

	static std::vector<int> memoryBlocks;

	static void prepareMemoryBlocks();

	static void pageIn();
	static void pageOut();
	static long long maxOverallMemory;

private:
	//static long long maxOverallMemory;
	static int totalFrames;

};