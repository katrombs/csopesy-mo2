#include "ConsoleManager.h"
#include "MainConsole.h"
#include <iostream>
#include "BaseScreen.h"
#include "Process.h"
#include <thread>
#include <fstream>
#include "ScheduleWorker.h"
#include <algorithm> 
#include <iostream>


int ScheduleWorker::usedCores = 0;
int ScheduleWorker::availableCores = 0;
int MainConsole::totalNumCores = 0;

ConsoleManager* ConsoleManager::sharedInstance = nullptr;
ConsoleManager* ConsoleManager::getInstance() {
	return sharedInstance;
}

void ConsoleManager::initialize() {
	sharedInstance = new ConsoleManager();
}

void ConsoleManager::destroy() {
	delete sharedInstance;
}

void ConsoleManager::drawConsole() const {
	if (this->currentConsole != nullptr) {
		this->currentConsole->display();
	}
	else {
		std::cerr << "There is no assigned console. Please check." << std::endl;
	}
}

void ConsoleManager::process() const {
	if (this->currentConsole != nullptr) {
		this->currentConsole->process();
	}
	else {
		std::cerr << "There is no assigned console. Please check." << std::endl;
	}
}

Process* ConsoleManager::getProcessByName(const std::string& processName) const {
	for (const auto& process : unfinishedProcessList) {
		if (process->getName() == processName) {
			return process;
		}
	}

	for (const auto& process : finishedProcesses) {
		if (process->getName() == processName) {
			return process;
		}
	}
	return nullptr;
}

void ConsoleManager::switchConsole(String consoleName) {
	if (this->consoleTable.contains(consoleName)) {
		// Clear the screen
		system("cls");
		this->previousConsole = this->currentConsole;
		this->currentConsole = this->consoleTable[consoleName];
		this->currentConsole->onEnabled();
	}
	else {
		std::cerr << "Console Name " << consoleName << " not found. Was it initialized?" << std::endl;
	}
}

void ConsoleManager::registerScreen(std::shared_ptr<BaseScreen> screenRef) {
	if (this->consoleTable.contains(screenRef->getName())) {
		std::cerr << "Screen name " << screenRef->getName() << " already exists. Please use a different name." << std::endl;
	}

	this->consoleTable[screenRef->getName()] = screenRef;
}

void ConsoleManager::switchToScreen(String screenName) {
	if (this->consoleTable.contains(screenName)) {
		//Clear the screen
		system("cls");
		this->previousConsole = this->currentConsole;
		this->currentConsole = this->consoleTable[screenName];
		this->currentConsole->onEnabled();
	}
	else {
		std::cerr << "Screen Name " << screenName << " not found. Was it initialized?" << std::endl;
	}
}

void ConsoleManager::nullProcessName() {
	std::cerr << "You did not enter a process name. Enter a Screen Name." << std::endl;
}

void ConsoleManager::invalidPrefix() {
	std::cerr << "Invalid prefix. Use \'-s\' to start a new screen or \'-r\' to reattach an existing screen." << std::endl;
}

void ConsoleManager::unregisterScreen(String screenName) {
	if (this->consoleTable.contains(screenName)) {
		this->consoleTable.erase(screenName);
	}
	else {
		std::cerr << "Unable to unregister " << screenName << ". Was it registered?" << std::endl;
	}
}

void ConsoleManager::returnToPreviousConsole() {
	if (this->previousConsole != nullptr) {
		// Clear the screen
		system("cls");
		this->currentConsole = this->previousConsole;
		this->previousConsole = nullptr;
		this->currentConsole->onEnabled();
	}
	else {
		std::cerr << "Previous Console not found." << std::endl;
	}
}

void ConsoleManager::exitApplication() {
	this->running = false;
}

bool ConsoleManager::isRunning() const {
	return this->running;
}

void ConsoleManager::addFinishedProcess(Process* process) {
	// Remove the process from unfinishedProcessList
	auto it_unfinished = std::find(unfinishedProcessList.begin(), unfinishedProcessList.end(), process);
	if (it_unfinished != unfinishedProcessList.end()) {
		unfinishedProcessList.erase(it_unfinished);
	}

	// Add process to finishedProcesses if not already present
	if (std::find(finishedProcesses.begin(), finishedProcesses.end(), process) == finishedProcesses.end()) {
		finishedProcesses.push_back(process);
	}

	// Remove the process from waitingProcesses
	auto it_waiting = std::find(waitingProcesses.begin(), waitingProcesses.end(), process);
	if (it_waiting != waitingProcesses.end()) {
		waitingProcesses.erase(it_waiting);
	}
}

std::vector<Process*> ConsoleManager::getProcessesInMemory() const {
	return unfinishedProcessList;
}
int ConsoleManager::calculateExternalFragmentation(int maxMemory) const {
	int fragmentation = 0;
	int lastEndAddress = 0;
	// If no processes are loaded, all memory is fragmented
	if (unfinishedProcessList.empty()) {
		return maxMemory;
	}
	// Sort processes by starting address
	std::vector<Process*> sortedProcesses = unfinishedProcessList;
	std::sort(sortedProcesses.begin(), sortedProcesses.end(), [](Process* a, Process* b) {
		return a->getStartAddress() < b->getStartAddress();
		});
	// Calculate gaps between processes
	for (const auto& process : sortedProcesses) {
		fragmentation += process->getStartAddress() - lastEndAddress;
		lastEndAddress = process->getEndAddress();
	}
	// Add remaining space after the last process
	fragmentation += maxMemory - lastEndAddress;
	return fragmentation;
}
void ConsoleManager::waitingProcess(Process* process) {

	
	for (int i = 0; i < unfinishedProcessList.size(); i++) {
		if (unfinishedProcessList[i] == process) {
			unfinishedProcessList.erase(unfinishedProcessList.begin() + i);
			break;
		}
	}
	

	if (std::find(waitingProcesses.begin(), waitingProcesses.end(), process) == waitingProcesses.end()) {
		waitingProcesses.push_back(process);
	}
}

void ConsoleManager::listFinishedProcesses(bool writeToFile) {
	std::ostream* outStream;
	std::ofstream logFile;

	// Write to file only if report-util is called
	if (writeToFile) {
		logFile.open("csopesy-log.txt", std::ios::out | std::ios::trunc);
		if (!logFile.is_open()) {
			std::cerr << "Error: Unable to open log file." << std::endl;
			return;
		}
		outStream = &logFile; // Point to the file stream
	}
	else {
		outStream = &std::cout; // Point to the console output
	}

	// TODO: call respective variables
	// Compute CPU Utilization
	int cpuUtilPercent = (static_cast<float>(ScheduleWorker::usedCores) / MainConsole::totalNumCores) * 100;
	int availCores = abs((ScheduleWorker::usedCores - ScheduleWorker::availableCores));
	*outStream << "\nCPU utilization: " << cpuUtilPercent << "%/100%" << "\nCores used: " << ScheduleWorker::usedCores << "\nCores available: " << availCores << "\n" << std::endl;

	
	*outStream << "--------------------------------------\n";
	*outStream << "Waiting Processes:" << std::endl;
	for (const auto& process : waitingProcesses) {
		*outStream << process->getName() // Process Name
			<< "\t" << "(" << process->getTimeCreated() << ")" // Timestamp of time created
			<< "\t" << "Core: " << process->getCoreAssigned() // Core that worked on process
			<< "\t" << process->getCurrentLine() << " / " << process->getTotalLines() // Current line / total line of execution
			//<< ", Unfinished: " << (process->isFinished() ? "Yes" : "No") // Unfinished? Y/N
			<< std::endl;
	}
	

	*outStream << "--------------------------------------\n";
	*outStream << "Running Processes:" << std::endl;
	for (const auto& process : unfinishedProcessList) {
		*outStream << process->getName() // Process Name
			<< "\t" << "(" << process->getTimeCreated() << ")" // Timestamp of time created
			<< "\t" << "Core: " << process->getCoreAssigned() // Core that worked on process
			<< "\t" << process->getCurrentLine() << " / " << process->getTotalLines() // Current line / total line of execution
			//<< ", Unfinished: " << (process->isFinished() ? "Yes" : "No") // Unfinished? Y/N
			<< std::endl;
	}

	*outStream << "\nFinished Processes:" << std::endl;
	for (const auto& process : finishedProcesses) {
		*outStream << process->getName() 
			<< "\t" << "(" << process->getTimeCreated() << ")" // Timestamp of time created
			<< "\t\t" << "Finished"
			<< "\t" << process->getCurrentLine() << " / " << process->getTotalLines() // Current line / total line of execution
			<< std::endl;
	}
	*outStream << "--------------------------------------\n";

	if (writeToFile) {
		logFile.close();
	}
}
void ConsoleManager::vmstat() const {
	std::cerr << "\n--------------------------------------------\n";
	std::cerr << "| VMSTAT V01.00 Driver Version: 01.00      |\n";
	std::cerr << "--------------------------------------------\n";

	long long totalMemory = MainConsole::maxOverallMem;
	long long usedMemory = getUsedMemory();

	if (totalMemory < usedMemory) {
		std::cerr << "Error: Used memory exceeds total memory!\n";
		return;
	}

	long long freeMemory = totalMemory - usedMemory;

	//int totalCpuTicks = activeCpuTicks + idleCpuTicks;

	//// Memory statistics
	std::cerr << "Total Memory: " << totalMemory << " KB\n";
	std::cerr << "Used Memory: " << usedMemory << " KB\n";
	std::cerr << "Free Memory: " << freeMemory << " KB\n";

	///////// TODO implement these
	//// CPU ticks statistics 
	//std::cerr << "Idle CPU Ticks: " << idleCpuTicks << "\n";
	//std::cerr << "Active CPU Ticks: " << activeCpuTicks << "\n";
	//std::cerr << "Total CPU Ticks: " << totalCpuTicks << "\n";

	//// Paging statistics
	//std::cerr << "Pages Paged In: " << numPagedIn << "\n";
	//std::cerr << "Pages Paged Out: " << numPagedOut << "\n";

	std::cerr << "-------------------------------------------\n";
}

long long ConsoleManager::getUsedMemory() const {
	long long usedMemory = 0;
	for (const auto& process : unfinishedProcessList) {
		usedMemory += process->getMemoryUsage();
	}
	return usedMemory;
}



ConsoleManager::ConsoleManager() {

	this->running = true;

	//Initialize consoles
	this->consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);

	const std::shared_ptr<MainConsole> mainConsole = std::make_shared<MainConsole>(MAIN_CONSOLE);

	this->consoleTable[MAIN_CONSOLE] = mainConsole;

	this->switchConsole(MAIN_CONSOLE);
}