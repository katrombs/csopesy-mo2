#include "FileWrite.h"
#include "Process.h"
#include "MainConsole.h"
#include <vector> 
#include <fstream>
#include <filesystem>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <vector>
#include <string>

namespace fs = std::filesystem;

void FileWrite::generateFile(int processID, const std::string& processName, const std::string& timeCreated, const std::vector<std::string>& logs) {
    // Specify the folder path where you want to save the file
    std::string folderPath = fs::current_path().string();  // Change this to the desired location

    // Concatenate the folder path and the file name
    std::string fileName = folderPath + "\\process_" + std::to_string(processID) + "_" + processName + ".txt";

    std::ofstream outFile(fileName, std::ios::out | std::ios::app);
    if (outFile) {
        outFile << "Process Name: " << processName << "\n";
        outFile << "Process ID: " << processID << "\n";
        outFile << "Created: " << timeCreated << "\n\n";
        outFile << "Logs:\n";

        for (const auto& log : logs) {
            outFile << log << "\n";
        }

        outFile.close();
        //std::cerr << "\nFile saved to: " << fileName << std::endl;  // Inform where the file is saved
    }
    else {
        std::cerr << "Error: Could not open file " << fileName << std::endl;
    }
}

std::string getCurrentTimestamp() {
    time_t currTime;
    struct tm datetime;
    time(&currTime);
    localtime_s(&datetime, &currTime);
    std::ostringstream oss;
    oss << std::put_time(&datetime, "%m/%d/%Y %I:%M:%S %p");
    return oss.str();
}
void FileWrite::generateMemorySnapshot(int quantumCycle, const std::vector<Process*>& processes, int fragmentation) {
    std::string fileName = "memory_stamp_" + std::to_string(quantumCycle) + ".txt";
    std::ofstream outFile(fileName);
    // following mock UI
    outFile << "Timestamp: (" << getCurrentTimestamp() << ")\n";
    outFile << "Number of processes in memory: " << processes.size() << "\n";
    //outFile << "Total external fragmentation in KB: " << MainConsole::maxOverallMem - (MainConsole::memPerProcess * processes.size()) << "\n";
    outFile << "Total external fragmentation in KB: " << fragmentation << "\n";

    outFile << "\n----end---- = " << MainConsole::maxOverallMem << std::endl;
    for (const auto& process : processes) {
        outFile << process->getStartAddress() << "\n";
        outFile << "P" << process->getID() << "\n";
        outFile << process->getEndAddress() << "\n\n";
    }
    outFile << "----start---- = 0\n";
    outFile.close();
}