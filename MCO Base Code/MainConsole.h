#include <iostream>
#include "TypedefRepo.h"
#include "ConsoleManager.h"

class MainConsole : public AConsole {
public:
	MainConsole(String name);
	~MainConsole();

	// Overriding BaseScreen methods
	void display() override;
	void process() override;
	void onEnabled() override;

	// Other Variables (For Whole System)
	static long long batchProcessFreq;
	static int totalNumCores;
	static String scheduler;

	// Other Variables (For Process)
	static long long quantumCycles;
	static long long minimumIns;
	static long long maximumIns;
	static long long delaysPerExec;

	// First-fit Memory manager
	static long long maxOverallMem;
	static int memPerFrame;
	static long long minMemPerProc;
	static long long maxMemPerProc;

	static std::vector <std::string> processesNameList;
	static std::atomic<int> curClockCycle;
	void startClock();


private:
	bool refresh;
};