#pragma once

#include <vector>

class MemoryManager {
public:
	MemoryManager();

	static std::vector<int> memoryBlocks;

	static void prepareMemoryBlocks();

	// Paging
	static std::vector<bool> frameTable;     
	static void prepareFrames();          
	static int findFreeFrame();             
	static void releaseFrame(int frameNumber);

private:
	static long long maxOverallMemory;
	static int frameSize;
};