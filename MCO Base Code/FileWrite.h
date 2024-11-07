#pragma once
#include <string>
#include <vector>

class Process;

class FileWrite {
public:
    static void generateFile(int processID, const std::string& processName, const std::string& timeCreated, const std::vector<std::string>& logs);
    static void generateMemorySnapshot(int quantumCycle, const std::vector<Process*>& processes, int fragmentation);
};
