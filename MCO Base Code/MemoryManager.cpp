#include <iostream>
#include "MainConsole.h"
#include "MemoryManager.h"

std::vector<int> MemoryManager::memoryBlocks;
std::vector<bool> MemoryManager::frameTable;
long long MemoryManager::maxOverallMemory = 0;
int MemoryManager::frameSize = 0;

MemoryManager::MemoryManager() {
	prepareMemoryBlocks();
}

void MemoryManager::prepareMemoryBlocks() {
    frameSize = MainConsole::memPerFrame;
    long long numFrames = maxOverallMemory / frameSize;

    frameTable = std::vector<bool>(numFrames, false); // false if frames arwe free
}

int MemoryManager::findFreeFrame() {
    for (int i = 0; i < frameTable.size(); ++i) {
        if (!frameTable[i]) {
            return i;
        }
    }
    return -1;  // no free frame 
}

// release a frame, making it available again
void MemoryManager::releaseFrame(int frameNumber) {
    if (frameNumber >= 0 && frameNumber < frameTable.size()) {
        frameTable[frameNumber] = false;  // mark as free frame
        // debug
        //std::cout << "Frame " << frameNumber << " released." << std::endl;
    }
}