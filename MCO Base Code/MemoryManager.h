#pragma once

#include <vector>
#include <mutex>

class MemoryManager {
public:
    static MemoryManager* getInstance();
    MemoryManager() {}
    void initialize(long long overallMemory, int frameSize);
    long long maxOverallMemory;


    // Static methods for preparing memory blocks and managing frames
    void prepareMemoryBlocks();
    int findFreeFrame();
    void releaseFrame(int frameNumber);

    // Flat memory allocation method
    bool allocateFlat(long long memRequired, long long& startAddress);

    // Paging memory allocation method
    int allocatePage();
    void releasePage(int frameNumber);

    std::vector<bool> frameTable;

    void deallocateFlat(long long startAddress, long long memReleased);

    int getFrameSize();
    long long getUsedMemory();

    void setMaxOverallMemory(long long value) {
        std::lock_guard<std::mutex> lock(memoryMutex);  
        maxOverallMemory = value;
    }

    long long getMaxOverallMemory() {
        std::lock_guard<std::mutex> lock(memoryMutex);
        return maxOverallMemory;
    }

private:
    static MemoryManager* instance;
    long long totalMemory;
    long long usedMemory;
	int frameSize;
    std::vector<std::pair<long long, long long>> memoryBlocks;
    std::mutex memoryMutex;
};