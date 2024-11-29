#include <iostream>
#include "MainConsole.h"
#include "MemoryManager.h"
#include <algorithm>

MemoryManager* MemoryManager::instance = nullptr;
MemoryManager* MemoryManager::getInstance() {
    if (instance == nullptr) {
        instance = new MemoryManager();
    }
    return instance;
}

void MemoryManager::initialize(long long overallMemory, int frameSize) {
    this->totalMemory = overallMemory;
    this->frameSize = frameSize;
    this->usedMemory = 0;
    this->maxOverallMemory = overallMemory; 

    memoryBlocks.clear();
    memoryBlocks.push_back(std::make_pair(0, overallMemory));

    prepareMemoryBlocks();
}

void MemoryManager::prepareMemoryBlocks() {
    if (this->frameSize > 0) {
        long long numFrames = this->maxOverallMemory / this->frameSize;
        frameTable = std::vector<bool>(numFrames, false); // false if frames are free
    }
}


int MemoryManager::findFreeFrame() {
    for (int i = 0; i < frameTable.size(); ++i) {
        if (!frameTable[i]) {
            return i;
        }
    }
    return -1;  // No free frame
}

// release a frame, making it available again
void MemoryManager::releaseFrame(int frameNumber) {
    if (frameNumber >= 0 && frameNumber < frameTable.size()) {
        frameTable[frameNumber] = false;  // mark as free 
    }
}

bool MemoryManager::allocateFlat(long long memRequired, long long& startAddress) {
    std::lock_guard<std::mutex> lock(memoryMutex); 

    for (auto it = memoryBlocks.begin(); it != memoryBlocks.end(); ++it) {
        long long blockStart = it->first;
        long long blockEnd = it->second;
        long long blockSize = blockEnd - blockStart;

        if (blockSize >= memRequired) {
            startAddress = blockStart;
            if (blockSize == memRequired) {
                memoryBlocks.erase(it);
            }
            else {

                it->first += memRequired;
            }
            usedMemory += memRequired;
            return true;
        }
    }
    return false;
}

void MemoryManager::deallocateFlat(long long startAddress, long long memReleased) {
    std::lock_guard<std::mutex> lock(memoryMutex);

    memoryBlocks.push_back(std::make_pair(startAddress, startAddress + memReleased)); // add free block to mem

    if (usedMemory >= memReleased) {
        usedMemory -= memReleased;
    }
    else {
        usedMemory = 0;
    }

    //Merge
    std::sort(memoryBlocks.begin(), memoryBlocks.end()); 
    for (auto it = memoryBlocks.begin(); it != memoryBlocks.end();) {
        auto nextIt = std::next(it);
        if (nextIt != memoryBlocks.end() && it->second == nextIt->first) {
            it->second = nextIt->second;
            memoryBlocks.erase(nextIt);
        }
        else {
            ++it;
        }
    }
}

int MemoryManager::allocatePage() {
    for (int i = 0; i < frameTable.size(); i++) {
        if (!frameTable[i]) {  // frame is free
            frameTable[i] = true;
            return i;  // return frame number
        }
    }
    return -1;  
}

void MemoryManager::releasePage(int frameNumber) {
    if (frameNumber >= 0 && frameNumber < frameTable.size()) {
        frameTable[frameNumber] = false;  // frame is free
    }
}

int MemoryManager::getFrameSize() {
    return frameSize;
}

long long MemoryManager::getUsedMemory() {
    std::lock_guard<std::mutex> lock(memoryMutex);
    return usedMemory;
}