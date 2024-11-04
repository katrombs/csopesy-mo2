#pragma once
#include <vector>
#include <memory>
#include <deque>
#include "Process.h" 
#include <mutex>  // Add this include to use std::mutex


class ScheduleWorker {
public:
    ScheduleWorker();
    ~ScheduleWorker();

    void initialize(int numCores);
    static void addProcess(std::shared_ptr<Process> process);
    static void addWaitProcess(std::shared_ptr<Process> process);
    void scheduleProcess();
    void displaySchedule() const;

    static int usedCores;                 
    static int availableCores;              
    std::vector<std::shared_ptr<Process>> schedulerQueue;
    static std::vector<int> cores;
    int coreAssigned;

    static void testSchedule();
    static int schedulerCurCycle;

    static bool stopTest;
    static std::mutex schedulerMutex;  // Add this line


private:

    void initializeCores(int numCores);
    static std::vector<std::shared_ptr<Process>> processList;
    static std::vector<std::shared_ptr<Process>> waitingQueue;

};
