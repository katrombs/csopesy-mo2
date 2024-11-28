#include <iostream>
#include "MainConsole.h"
#include "MemoryManager.h"

std::vector<int> MemoryManager::memoryBlocks;
long long MemoryManager::maxOverallMemory = 0;
int MemoryManager::totalFrames = 0;

MemoryManager::MemoryManager(long long maxOverallMem) {
	this->maxOverallMemory = maxOverallMem;
}


void MemoryManager::prepareMemoryBlocks() {
    // handle single block
    if (MainConsole::memPerFrame == 0) {
        return;
    }

    long long numBlocks = MemoryManager::maxOverallMemory / MainConsole::memPerFrame;

    if (numBlocks == 1) {
        MemoryManager::memoryBlocks.push_back(MainConsole::memPerFrame);
    }
}


// Paging
void MemoryManager::pageIn() {
    ConsoleManager::numPagedIn++;
}

void MemoryManager::pageOut() {
    ConsoleManager::numPagedOut++;
}
