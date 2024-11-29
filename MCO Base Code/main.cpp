#include <iostream>
#include <string>
#include <sstream> // [Link: https://www.geeksforgeeks.org/processing-strings-using-stdistringstream/]
#include "ConsoleManager.h"
#include "MainConsole.h"
#include <thread>

using namespace std; //To not specify the prefix (std::<syntax>)

void startCpuCycle() {
    while (true) {
        MainConsole::curClockCycle++;
        //std::cerr << MainConsole::curClockCycle << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(300)); // <----------------- CHANGE THIS TO milliseconds(300) TO MAKE EVERYTHING FASTER
    }
}

int main()
{
    ConsoleManager::initialize();
    bool running = true;

    std::thread cpuCycleThread(startCpuCycle);
    cpuCycleThread.detach();

    while (running) {

        ConsoleManager::getInstance()->drawConsole();
        ConsoleManager::getInstance()->process();

        running = ConsoleManager::getInstance()->isRunning();
    }
    
    ConsoleManager::destroy();
    return 0;
}
