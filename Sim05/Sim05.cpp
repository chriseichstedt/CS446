/**
@file  Sim05.cpp
@author  Christopher Eichstedt
@version  1.0
@brief  You will now be required to implement your simulator with two scheduling algorithms using interrupts.
**/

//----------------------------------------------------------------------------------------------------
//libraries, declarations & global variables
//----------------------------------------------------------------------------------------------------

//-------------------------
//included libraries
//-------------------------
#include <iostream>
#include <iomanip>
#include <fstream>
#include <queue>
#include <string>
#include <sstream>
#include <time.h>
#include <pthread.h>
using namespace std;

//-------------------------
//struct declarations
//-------------------------
struct configData
{
	int mdt; //monitor display time
	int pct; //processor cycle time
	int sct; //scanner cycle time
	int hdct; //hard drive cycle time
	int kct; //keyboard cycle time
	int mct; //memory cycle time
	int pjct; //projector cycle time
	int numProj; //number of projectors
	int numHDD; //number of hard drives
	int countProjOut; //count projector output calls
	int countHDDOut; //count hard drive output calls
	int countHDDIn;	//count hard drive input calls
	int pqn; //processor quantum number
	string logFile; //log file for output
	string inputFile; //input file for meta data
};

struct inputData
{
	string opCode;
	string opCommand;
	int cycleTime;
};

struct processData
{
	queue<inputData> commands;
	int countIO;
	int countTask;
	int processCount;
};

struct PCB
{
	int processState;
};

struct systemMemory
{
	int total;
	int blockSize;
	int count;
};

//-------------------------
//global variables
//-------------------------

//structs
configData tempConfig;
systemMemory tempMemory;

//process control block
PCB tempPCB;
const int PCBexit = 0;
const int PCBstart = 1;
const int PCBready = 2;
const int PCBrunning = 3;
const int PCBwaiting = 4;

//clock
int timerStart = clock();
double clockStart = (clock()-timerStart)/(double)(CLOCKS_PER_SEC);
double startTime;
double endTime;

//system
unsigned int memLocation;
string inputFileName;
char logTo;
int quantumTime;
int totalTime;
int totalCount;
int processID;
pthread_mutex_t mtx;
pthread_mutex_t mtx2;
bool isReset = false;

//scheduling
bool strs = false; //shortest time first scheduling
bool rrs = false; //round robin scheduling

//master queue
priority_queue<processData, vector<processData>, less<processData> > strQueue;
queue<processData> rrQueue;
queue<processData> masterQueue;

//-------------------------
//function declarations
//-------------------------
void readConfig(char*) throw(runtime_error);
void readInput(string) throw(runtime_error);
void output(string, string, string, int, double, double);
void memoryAllocate();
void timerProcess(processData);
void simulateProcess(string);
void* rrThread(void*);
void* loadThread(void*);
void* ioProcess(void*);
void loadProcess();
void nonIOProcess();
void delay (int);
void startOutput(string);
void endOutput(string);
bool operator>(processData, processData);
bool operator<(processData, processData);

//----------------------------------------------------------------------------------------------------
//main driver
//----------------------------------------------------------------------------------------------------

/**
@brief  main function, calls 3 other functions to read config, read input and output to file
@param  command line arguments for the config file name
@return  returns 0 to end program
@pre  readConfig(), readInput(), output()
@post  takes the config and input information and outputs them to a file
**/
int main(int argc, char* argv[])
{
	readConfig(argv[1]);
	readInput(tempConfig.inputFile);
	return 0;
}

//----------------------------------------------------------------------------------------------------
//functions
//----------------------------------------------------------------------------------------------------

/**
@brief  a function that reads in the config file the user passes, and stores the data into a global struct
@param  string filename
@return  none
@pre  none
@post  stores config data into global struct
**/
void readConfig(char* filename) throw(runtime_error)
{
	//open file
	ifstream fin;
	fin.open(filename);
	queue<string> tempQ;
	string tempS;
	
	//check if file exists
	if(!fin.good())
	{
		throw runtime_error("config file not found");
	}
	
	//read in a configuration file
	while(fin.good())
	{
		fin >> tempS; 
		tempQ.push(tempS);
	}
	
	//close file
	fin.close();
	
	//store file data into global struct
	while(!tempQ.empty())
	{
		string tempS = tempQ.front();
		int val = 0;
		
		if (tempS == "Path:")
		{
			for(int i = 0; i < 1; i++)
			{
				tempQ.pop();
			}
			
			tempConfig.inputFile = tempQ.front();
		}
		
		if (tempS == "Monitor")
		{
			for(int i = 0; i < 4; i++)
			{
				tempQ.pop();
			}
			
			stringstream tempMDT(tempQ.front());
			tempMDT >> val;
			tempConfig.mdt = val;
		}
		
		if (tempS == "Processor")
		{
			tempQ.pop();
			
			//processor quantum number
			if (tempQ.front() == "Quantum")
			{
				for(int i = 0; i < 3; i++)
				{	
					tempQ.pop();
				}
				
				stringstream tempPQN(tempQ.front());
				tempPQN >> val;
				tempConfig.pqn = val;
			}
			
			else
			{
				for(int i = 0; i < 3; i++)
				{
					tempQ.pop();
				}
			
				stringstream tempPCT(tempQ.front());
				tempPCT >> val;
				tempConfig.pct = val;
			}
		}
		
		if (tempS == "Scanner")
		{
			for(int i = 0; i < 4; i++)
			{
				tempQ.pop();
			}
			
			stringstream tempSCT(tempQ.front());
			tempSCT >> val;
			tempConfig.sct = val;
		}
		
		if (tempS == "Hard")
		{
			for(int i = 0; i < 5; i++)
			{
				tempQ.pop();
			}
			
			stringstream tempHDCT(tempQ.front());
			tempHDCT >> val;
			tempConfig.hdct = val;
		}
		
		if (tempS == "Keyboard")
		{
			for(int i = 0; i < 4; i++)
			{
				tempQ.pop();
			}
			
			stringstream tempKCT(tempQ.front());
			tempKCT >> val;
			tempConfig.kct = val;
		}
		
		if (tempS == "Memory")
		{
			for(int i = 0; i < 4; i++)
			{
				tempQ.pop();
			}
			
			stringstream tempMCT(tempQ.front());
			tempMCT >> val;
			tempConfig.mct = val;
		}
		
		if (tempS == "Projector")
		{
			tempQ.pop();
			
			//checks the number of projectors and hard drives
			if (tempQ.front() == "quantity:")
			{
				tempQ.pop();
				int val;
				string tempVal = tempQ.front();
				stringstream tempPRPV(tempVal);
				tempPRPV >> val;
				tempConfig.numProj = val;
		
				for(int i = 0; i < 4; i++)
				{
					tempQ.pop();
				}
		
				string tempVal2 = tempQ.front();
				stringstream tempPRPV2(tempVal2);
				tempPRPV2 >> val;
				tempConfig.numHDD = val;
			}
			
			else
			{
				for(int i = 0; i < 3; i++)
				{
					tempQ.pop();
				}
				stringstream tempPJCT(tempQ.front());
				tempPJCT >> val;
				tempConfig.pjct = val;
			}
		}
		
		//log if monitor, file or both
		if (tempS == "Log")
		{
			for(int i = 0; i < 2; i++)
			{	
				tempQ.pop();
			}
			
			if (tempQ.front() == "Monitor")
			{
				logTo = 'M';
				
				for(int i = 0; i < 4; i++)
				{
					tempQ.pop();
				}
				
				tempConfig.logFile = tempQ.front();
			}
			
			else if (tempQ.front() == "File")
			{
				logTo = 'F';
				
				for(int i = 0; i < 4; i++)
				{
					tempQ.pop();
				}
				
				tempConfig.logFile = tempQ.front();
			}
			
			else if (tempQ.front() == "Both")
			{
				logTo = 'B';
				
				for(int i = 0; i < 4; i++)
				{
					tempQ.pop();
				}
				
				tempConfig.logFile = tempQ.front();
			}
				
			//output if log is incorrect
			else
			{
				throw runtime_error("log error");
			}
		}
		
		//system and block sizes
		if(tempS == "System")
		{
			for(int i = 0; i < 2; i++)
			{
				tempQ.pop();
			}
					
			//checking system memory size
			if (tempQ.front() == "{kbytes}:")
			{
				tempQ.pop();
				int val;
				string tempVal = tempQ.front();
				stringstream tempPRPV(tempVal);
				tempPRPV >> val;
				tempMemory.total = val;
				
				for(int i = 0; i < 4; i++)
				{
					tempQ.pop();
				}
				
				if (tempQ.front() == "{kbytes}:")
				{
					tempQ.pop();
					string tempVal2 = tempQ.front();
					stringstream tempPRPV2(tempVal2);
					tempPRPV2 >> val;
					tempMemory.blockSize = val;
				}
			}
			
			else if (tempQ.front() == "{Mbytes}:")
			{
				tempQ.pop();
				int val;
				string tempVal = tempQ.front();
				stringstream tempPRPV(tempVal);
				tempPRPV >> val;
				tempMemory.total = (val*1024);
				
				for(int i = 0; i < 4; i++)
				{
					tempQ.pop();
				}
				
				if (tempQ.front() == "{Mbytes}:")
				{
					tempQ.pop();
					string tempVal2 = tempQ.front();
					stringstream tempPRPV2(tempVal2);
					tempPRPV2 >> val;
					tempMemory.blockSize = (val*1024);
				}
			}
		
			else if (tempQ.front() == "{Gbytes}:")
			{
				tempQ.pop();
				int val;
				string tempVal = tempQ.front();
				stringstream tempPRPV(tempVal);
				tempPRPV >> val;
				tempMemory.total = (val*1024*1024);
				
				for(int i = 0; i < 4; i++)
				{
					tempQ.pop();
				}
				
				if (tempQ.front() == "{Gbytes}:")
				{
					tempQ.pop();
					string tempVal2 = tempQ.front();
					stringstream tempPRPV2(tempVal2);
					tempPRPV2 >> val;
					tempMemory.blockSize = (val*1024*1024);
				}
			}
		}
		
		//cpu scheduling
		if (tempS == "CPU")
		{
			for(int i = 0; i < 3; i++)
			{
				tempQ.pop();
			}
			
			if(tempQ.front() == "STR")
			{
				strs = true;
			}
			
			else
			{
				rrs = true;
			}
		}
		
		else
		{
			tempQ.pop();
		}
	}
}

/**
@brief  a function that reads in the user defined meta-data input file and stores information into a global struct
@param  string filename
@return  none
@pre  none
@post  stores meta-data into a queue depending on scheduling and stores the calculated cycle time into another
**/
void readInput(string filename) throw(runtime_error)
{	
	//open file
	ifstream fin;
	fin.open(filename.c_str());
	string tempS;
	
	//check if file exists
	if(!fin.good())
	{
		throw runtime_error("meta-data file not found");
	}
	
	//read in input file
	while(fin.good())
	{
		fin >> tempS; 
		string tempCutStr = tempS.substr(0, 3);
		string tempVal;
		int val, bracket, semiColon;
		processData tempProcess;
		
		//application(start)
		if(tempCutStr == "A{b")
		{	
			inputData tempInput;
			tempProcess.countTask = 0;
			tempProcess.countIO = 0;
			tempInput.opCode = "application";
			tempInput.opCommand = "start";
			tempInput.cycleTime = 1;
			tempProcess.commands.push(tempInput);
			
			while(tempCutStr != "A{f")
			{
				fin >> tempS;
				tempCutStr = tempS.substr(0, 3);
				
				//processor(run)
				if(tempCutStr == "P{r")
				{
					bracket = tempS.find("}");
					semiColon = tempS.find(";");
					tempVal = tempS.substr(bracket+1, ((semiColon-bracket) - 1));
			
					stringstream tempPRPV(tempVal);
					tempPRPV >> val;
					inputData tempInput;
					tempInput.opCode = "processor";
					tempInput.opCommand = "run";
					tempInput.cycleTime = (val*tempConfig.pct);
					tempProcess.commands.push(tempInput);
					tempProcess.countTask++;
				}
				
				//memory(allocate)
				else if(tempCutStr == "M{a")
				{
					bracket = tempS.find("}");
					semiColon = tempS.find(";");
					tempVal = tempS.substr(bracket+1, ((semiColon-bracket) - 1));
			
					stringstream tempMAPV(tempVal);
					tempMAPV >> val;
					inputData tempInput;
					tempInput.opCode = "memory";
					tempInput.opCommand = "allocate";
					tempInput.cycleTime = (val*tempConfig.mct);
					tempProcess.commands.push(tempInput);
					tempProcess.countTask++;
				}
				
				//memory(block)
				else if(tempCutStr == "M{b")
				{
					bracket = tempS.find("}");
					semiColon = tempS.find(";");
					tempVal = tempS.substr(bracket+1, ((semiColon-bracket) - 1));
			
					stringstream tempMBPV(tempVal);
					tempMBPV >> val;
					inputData tempInput;
					tempInput.opCode = "memory";
					tempInput.opCommand = "block";
					tempInput.cycleTime = (val*tempConfig.mct);
					tempProcess.commands.push(tempInput);
					tempProcess.countTask++;
				}
				
				//output(monitor)
				else if(tempCutStr == "O{m")
				{
					bracket = tempS.find("}");
					semiColon = tempS.find(";");
					tempVal = tempS.substr(bracket+1, ((semiColon-bracket) - 1));
			
					stringstream tempOMPV(tempVal);
					tempOMPV >> val;
					inputData tempInput;
					tempInput.opCode = "output";
					tempInput.opCommand = "monitor";
					tempInput.cycleTime = (val*tempConfig.mdt);
					tempProcess.commands.push(tempInput);
					tempProcess.countIO++;
					tempProcess.countTask++;
				}
				
				//output(projector)
				else if(tempCutStr == "O{p")
				{
					bracket = tempS.find("}");
					semiColon = tempS.find(";");
					tempVal = tempS.substr(bracket+1, ((semiColon-bracket) - 1));
			
					stringstream tempOPPV(tempVal);
					tempOPPV >> val;
					inputData tempInput;
					tempInput.opCode = "output";
					tempInput.opCommand = "projector";
					tempInput.cycleTime = (val*tempConfig.pjct);
					tempProcess.commands.push(tempInput);
					tempProcess.countIO++;
					tempProcess.countTask++;
				}
				
				//output(hard drive)
				else if(tempCutStr == "O{h")
				{
					fin >> tempS;
					bracket = tempS.find("}");
					semiColon = tempS.find(";");
					tempVal = tempS.substr(bracket+1, ((semiColon-bracket) - 1));
			
					stringstream tempOHPV(tempVal);
					tempOHPV >> val;
					inputData tempInput;
					tempInput.opCode = "output";
					tempInput.opCommand = "hdd";
					tempInput.cycleTime = (val*tempConfig.hdct);
					tempProcess.commands.push(tempInput);
					tempProcess.countIO++;
					tempProcess.countTask++;
				}
				
				//input(keyboard)
				else if(tempCutStr == "I{k")
				{
					bracket = tempS.find("}");
					semiColon = tempS.find(";");
					tempVal = tempS.substr(bracket+1, ((semiColon-bracket) - 1));
			
					stringstream tempIKPV(tempVal);
					tempIKPV >> val;
					inputData tempInput;
					tempInput.opCode = "input";
					tempInput.opCommand = "keyboard";
					tempInput.cycleTime = (val*tempConfig.kct);
					tempProcess.commands.push(tempInput);
					tempProcess.countIO++;
					tempProcess.countTask++;
				}
		
				//input(hard drive)
				else if(tempCutStr == "I{h")
				{
					fin >> tempS;
					bracket = tempS.find("}");
					semiColon = tempS.find(";");
					tempVal = tempS.substr(bracket+1, ((semiColon-bracket) - 1));
			
					stringstream tempIHPV(tempVal);
					tempIHPV >> val;
					inputData tempInput;
					tempInput.opCode = "input";
					tempInput.opCommand = "hdd";
					tempInput.cycleTime = (val*tempConfig.hdct);
					tempProcess.commands.push(tempInput);
					tempProcess.countIO++;
					tempProcess.countTask++;
				}
		
				//input(scanner)
				else if(tempCutStr == "I{s")
				{
					bracket = tempS.find("}");
					semiColon = tempS.find(";");
					tempVal = tempS.substr(bracket+1, ((semiColon-bracket) - 1));
			
					stringstream tempISPV(tempVal);
					tempISPV >> val;
					inputData tempInput;
					tempInput.opCode = "input";
					tempInput.opCommand = "scanner";
					tempInput.cycleTime = (val*tempConfig.sct);
					tempProcess.commands.push(tempInput);
					tempProcess.countIO++;
					tempProcess.countTask++;
				}
				
				//if the process is missing A{finish}
				else if(tempCutStr == "S{f")
				{
					throw runtime_error("A{finish} not found before S{finish}");
				}	
			}
			
			//application(finish)
			tempInput.opCode = "application";
			tempInput.opCommand = "finish";
			tempInput.cycleTime = 1;
			tempProcess.commands.push(tempInput);
			tempProcess.processCount = ++processID;
			
			//using shortest time first scheduling
			if (strs == true)
			{
				strQueue.push(tempProcess);
				masterQueue.push(tempProcess);
			}
	
			//using round robin scheduling
			else
			{
				rrQueue.push(tempProcess);
				masterQueue.push(tempProcess);
			}
		}
	}
	
	//close file
	fin.close();
	
	//running timer process using temp queue
	processData tempProcess;
	
	//if shortest time first scheduling
	if (strs == true)
	{
		startOutput(tempConfig.logFile);
		
		while(!strQueue.empty())
		{
			tempProcess = strQueue.top();
			timerProcess(tempProcess);
			strQueue.pop();
		}
		
		endOutput(tempConfig.logFile);
	}
	
	//if round robin scheduling
	else
	{
		startOutput(tempConfig.logFile);
		
		while(!rrQueue.empty())
		{	
			tempProcess = rrQueue.front();
			timerProcess(tempProcess);
			rrQueue.pop();
		}
		
		endOutput(tempConfig.logFile);
	}
	

}
/**
@brief  a function that outputs the data stored in the global structs for the config and meta-data
@param  string filename
@return  none
@pre  timerProcess()
@post  prints to file, monitor or both with the information taken from the user's config and meta-data information 
**/
void output(string filename, string tempCode, string tempCommand, int tempCount, double tempStart, double tempEnd)
{
	//open file
	ofstream fout;
	fout.open(filename.c_str(),ios::app);
	
	double currentClock = (clock()-clockStart)/(double)(CLOCKS_PER_SEC);
	
	//input operation
	if(tempCode == "input")
	{	
		//keyboard command
		if(tempCommand == "keyboard")
		{
			if(logTo == 'M')
			{
				cout << fixed << tempStart << " - Process " << tempCount << ": start keyboard input" << endl;
				cout << fixed << tempEnd << " - Process " << tempCount << ": end keyboard input" << endl;
			}
				
			else if(logTo == 'F')
			{
				fout << fixed << tempStart << " - Process " << tempCount << ": start keyboard input" << endl;
				fout << fixed << tempEnd << " - Process " << tempCount << ": end keyboard input" << endl;
			}
			
			else
			{
				cout << fixed << tempStart << " - Process " << tempCount << ": start keyboard input" << endl;
				cout << fixed << tempEnd << " - Process " << tempCount << ": end keyboard input" << endl;
				fout << fixed << tempStart << " - Process " << tempCount << ": start keyboard input" << endl;
				fout << fixed << tempEnd << " - Process " << tempCount << ": end keyboard input" << endl;
			}
		}
			
		//hdd command
		else if(tempCode == "hdd")
		{
			if(logTo == 'M')
			{
				cout << fixed << tempStart << " - Process " << tempCount << ": start hard drive input on HDD " << (tempConfig.countHDDIn % tempConfig.numHDD) << endl;
				cout << fixed << tempEnd << " - Process " << tempCount << ": end hard drive input on HDD " << (tempConfig.countHDDIn % tempConfig.numHDD) << endl;
			}
		
			else if(logTo == 'F')
			{
				fout << fixed << tempStart << " - Process " << tempCount << ": start hard drive input on HDD " << (tempConfig.countHDDIn % tempConfig.numHDD) << endl;
				fout << fixed << tempEnd << " - Process " << tempCount << ": end hard drive input on HDD " << (tempConfig.countHDDIn % tempConfig.numHDD) << endl;
			}
			
			else
			{
				cout << fixed << tempStart << " - Process " << tempCount << ": start hard drive input on HDD " << (tempConfig.countHDDIn % tempConfig.numHDD) << endl;
				cout << fixed << tempEnd << " - Process " << tempCount << ": end hard drive input on HDD " << (tempConfig.countHDDIn % tempConfig.numHDD) << endl;
				fout << fixed << tempStart << " - Process " << tempCount << ": start hard drive input on HDD " << (tempConfig.countHDDIn % tempConfig.numHDD) << endl;
				fout << fixed << tempEnd << " - Process " << tempCount << ": end hard drive input on HDD " << (tempConfig.countHDDIn % tempConfig.numHDD) << endl;
			}

			tempConfig.countHDDIn++;
		}
			
		//scanner command
		else if(tempCommand == "scanner")
		{
			if(logTo == 'M')
			{
				cout << fixed << tempStart << " - Process " << tempCount << ": start scanner input" << endl;
				cout << fixed << tempEnd << " - Process " << tempCount << ": end scanner input" << endl;
			}
		
			else if(logTo == 'F')
			{
				fout << fixed << tempStart << " - Process " << tempCount << ": start scanner input" << endl;
				fout << fixed << tempEnd << " - Process " << tempCount << ": end scanner input" << endl;
			}
			
			else
			{
				cout << fixed << tempStart << " - Process " << tempCount << ": start scanner input" << endl;
				cout << fixed << tempEnd << " - Process " << tempCount << ": end scanner input" << endl;
				fout << fixed << tempStart << " - Process " << tempCount << ": start scanner input" << endl;
				fout << fixed << tempEnd << " - Process " << tempCount << ": end scanner input" << endl;
			}
		}
	}
		
	//output operation
	else if(tempCode == "output")
	{
		//monitor command
		if(tempCommand == "monitor")
		{
			if(logTo == 'M')
			{
				cout << fixed << tempStart << " - Process " << tempCount << ": start monitor output" << endl;
				cout << fixed << tempEnd << " - Process " << tempCount << ": end monitor output" << endl;
			}
			else if(logTo == 'F')
			{
				fout << fixed << tempStart << " - Process " << tempCount << ": start monitor output" << endl;
				fout << fixed << tempEnd << " - Process " << tempCount << ": end monitor output" << endl;
			}
			
			else
			{
				cout << fixed << tempStart << " - Process " << tempCount << ": start monitor output" << endl;
				cout << fixed << tempEnd << " - Process " << tempCount << ": end monitor output" << endl;
				fout << fixed << tempStart << " - Process " << tempCount << ": start monitor output" << endl;
				fout << fixed << tempEnd << " - Process " << tempCount << ": end monitor output" << endl;
			}
		}
		
		//projector command
		else if(tempCommand == "projector")
		{
			if(logTo == 'M')
			{
				cout << fixed << tempStart << " - Process " << tempCount << ": start projector output on PROJ " << (tempConfig.countProjOut % tempConfig.numProj) << endl;
				cout << fixed << tempEnd << " - Process " << tempCount << ": end projector output on PROJ " << (tempConfig.countProjOut % tempConfig.numProj) << endl;
			}
			else if(logTo == 'F')
			{
				fout << fixed << tempStart << " - Process " << tempCount << ": start projector output on PROJ " << (tempConfig.countProjOut % tempConfig.numProj) << endl;
				fout << fixed << tempEnd << " - Process " << tempCount << ": end projector output on PROJ " << (tempConfig.countProjOut % tempConfig.numProj) << endl;
			}
			
			else
			{
				cout << fixed << tempStart << " - Process " << tempCount << ": start projector output on PROJ " << (tempConfig.countProjOut % tempConfig.numProj) << endl;
				cout << fixed << tempEnd << " - Process " << tempCount << ": end projector output on PROJ " << (tempConfig.countProjOut % tempConfig.numProj) << endl;
				fout << fixed << tempStart << " - Process " << tempCount << ": start projector output on PROJ " << (tempConfig.countProjOut % tempConfig.numProj) << endl;
				fout << fixed << tempEnd << " - Process " << tempCount << ": end projector output on PROJ " << (tempConfig.countProjOut % tempConfig.numProj) << endl;
			}
		
			tempConfig.countProjOut++;
		}
		
		//hdd command
		else if(tempCommand == "hdd")
		{
			if(logTo == 'M')
			{
				cout << fixed << tempStart << " - Process " << tempCount << ": start hard drive output on HDD " << (tempConfig.countHDDOut % tempConfig.numHDD) << endl;
				cout << fixed << tempEnd << " - Process " << tempCount << ": end hard drive output on HDD " << (tempConfig.countHDDOut % tempConfig.numHDD) << endl;
			}
			else if(logTo == 'F')
			{
				fout << fixed << tempStart << " - Process " << tempCount << ": start hard drive output on HDD " << (tempConfig.countHDDOut % tempConfig.numHDD) << endl;
				fout << fixed << tempEnd << " - Process " << tempCount << ": end hard drive output on HDD " << (tempConfig.countHDDOut % tempConfig.numHDD) << endl;
			}
			
			else
			{
				cout << fixed << tempStart << " - Process " << tempCount << ": start hard drive output on HDD " << (tempConfig.countHDDOut % tempConfig.numHDD) << endl;
				cout << fixed << tempEnd << " - Process " << tempCount << ": end hard drive output on HDD " << (tempConfig.countHDDOut % tempConfig.numHDD) << endl;
				fout << fixed << tempStart << " - Process " << tempCount << ": start hard drive output on HDD " << (tempConfig.countHDDOut % tempConfig.numHDD) << endl;
				fout << fixed << tempEnd << " - Process " << tempCount << ": end hard drive output on HDD " << (tempConfig.countHDDOut % tempConfig.numHDD) << endl;
			}
		
			tempConfig.countHDDOut++;
		}
	}
		
	else
	{
		//memory operation
		if(tempCode == "memory")
		{
			//allocate command
			if(tempCommand == "allocate")
			{
				if(tempMemory.count > tempMemory.total)
				{
					tempMemory.count = 0;
				}
				
				if(logTo == 'M')
				{
					cout << fixed << tempStart << " - Process " << tempCount << ": allocating memory" << endl;
					cout << fixed << tempEnd << " - Process " << tempCount << ": memory allocated at " << "0x" << setfill('0') << setw(8) << hex << tempMemory.count << dec << endl;
				}
		
				else if(logTo == 'F')
				{
					fout << fixed << tempStart << " - Process " << tempCount << ": allocating memory" << endl;
					fout << fixed << tempEnd << " - Process " << tempCount << ": memory allocated at " << "0x" << setfill('0') << setw(8) << hex << tempMemory.count << dec << endl;
				}
				
				else
				{
					cout << fixed << tempStart << " - Process " << tempCount << ": allocating memory" << endl;
					cout << fixed << tempEnd << " - Process " << tempCount << ": memory allocated at " << "0x" << setfill('0') << setw(8) << hex << tempMemory.count << dec << endl;
					fout << fixed << tempStart << " - Process " << tempCount << ": allocating memory" << endl;
					fout << fixed << tempEnd << " - Process " << tempCount << ": memory allocated at " << "0x" << setfill('0') << setw(8) << hex << tempMemory.count << dec << endl;
				}
				
				tempMemory.count += tempMemory.blockSize;
			}
			
			//block command
			else if(tempCommand == "block")
			{
				if(logTo == 'M')
				{
					cout << fixed << tempStart << " - Process " << tempCount << ": start memory blocking" << endl;
					cout << fixed << tempEnd << " - Process " << tempCount << ": end memory blocking" << endl;
				}
				
				else if(logTo == 'F')
				{
					fout << fixed << tempStart << " - Process " << tempCount << ": start memory blocking" << endl;
					fout << fixed << tempEnd << " - Process " << tempCount << ": end memory blocking" << endl;
				}
				
				else
				{
					cout << fixed << tempStart << " - Process " << tempCount << ": start memory blocking" << endl;
					cout << fixed << tempEnd << " - Process " << tempCount << ": end memory blocking" << endl;
					fout << fixed << tempStart << " - Process " << tempCount << ": start memory blocking" << endl;
					fout << fixed << tempEnd << " - Process " << tempCount << ": end memory blocking" << endl;
				}
				
			}
		}
		
		//application
		else if(tempCode == "application")
		{
			//start process
			if(tempCommand == "start")
			{	
				if(logTo == 'M')
				{
					cout << fixed << tempStart << " - OS: preparing process " << tempCount << endl;
					cout << fixed << tempStart << " - OS: starting process " << tempCount << endl;
				}
				
				else if(logTo == 'F')
				{
					fout << fixed << tempStart << " - OS: preparing process " << tempCount << endl;
					fout << fixed << tempStart << " - OS: starting process " << tempCount << endl;
				}
				
				else
				{
					cout << fixed << tempStart << " - OS: preparing process " << tempCount << endl;
					cout << fixed << tempStart << " - OS: starting process " << tempCount << endl;
					fout << fixed << tempStart << " - OS: preparing process " << tempCount << endl;
					fout << fixed << tempStart << " - OS: starting process " << tempCount << endl;
				}
			}
			
			//end process
			else
			{
				if(logTo == 'M')
				{
					cout << fixed << tempStart << " - End process " << tempCount << endl;
				}
				
				else if(logTo == 'F')
				{
					fout << fixed << tempStart << " - End process " << tempCount << endl;
				}
				
				else
				{
					cout << fixed << tempStart << " - End process " << tempCount << endl;
					fout << fixed << tempStart << " - End process " << tempCount << endl;
				}
			}
		}
		
		//processor
		else
		{
		
			if(logTo == 'M')
			{
				cout << fixed << tempStart << " - Process " << tempCount << ": start processing action" << endl;
				cout << fixed << tempEnd << " - Process " << tempCount << ": end processing action" << endl;
			}
		
			else if(logTo == 'F')
			{
				fout << fixed << tempStart << " - Process " << tempCount << ": start processing action" << endl;
				fout << fixed << tempEnd << " - Process " << tempCount << ": end processing action" << endl;
			}
			
			else
			{
				cout << fixed << tempStart << " - Process " << tempCount << ": start processing action" << endl;
				cout << fixed << tempEnd << " - Process " << tempCount << ": end processing action" << endl;
				fout << fixed << tempStart << " - Process " << tempCount << ": start processing action" << endl;
				fout << fixed << tempEnd << " - Process " << tempCount << ": end processing action" << endl;
			}
		}
	}
	
	//if process was interrupted by PQN
	if(isReset == true)
	{
		if(logTo == 'M')
		{
			cout << fixed << currentClock << " - ******* Process was interrupted *******" << endl;
		}
	
		else if(logTo == 'F')
		{
			fout << fixed << currentClock << " - ******* Process was interrupted *******" << endl;
		}
		
		else
		{
			cout << fixed << currentClock << " - ******* Process was interrupted *******" << endl;
			fout << fixed << currentClock << " - ******* Process was interrupted *******" << endl;
		}
	}
	
	//close file
	fout.close();
}
//----------------------------------------------------------------------------------------------------
//resource management system
//----------------------------------------------------------------------------------------------------

/**
@brief  a function that checks if the process is an input/output and calls the appropriate function
@param  none
@return  none  
@pre  ioProcess(), nonIOProcess()
@post  sets the PCB and runs the thread according
**/
void timerProcess(processData tempProcess)
{	
	//create thread, variable declarations and start process control block
	pthread_t thread2;
	pthread_t thread3;
	tempPCB.processState = PCBstart;
	quantumTime = tempConfig.pqn;
	int checkTime;
	int tempCount;
	string tempCode;
	string tempCommand;
	
	while(!tempProcess.commands.empty())
	{	
		//copy process information
		tempPCB.processState = PCBrunning;
		totalTime = tempProcess.commands.front().cycleTime;
		tempCode = tempProcess.commands.front().opCode;
		tempCommand = tempProcess.commands.front().opCommand;
		tempCount = tempProcess.processCount;
		checkTime += totalTime;
		
		//100ms check
		if(checkTime > 100)
		{
			//second thread for adding to STR or RR queue
			//pthread_create(&thread2, NULL, &loadThread, NULL);
			//pthread_join(thread2, NULL);
			loadProcess();
			checkTime = 0;
		}
		
		//round robin scheduling
		if(rrs == true)
		{
			pthread_create(&thread3, NULL, &rrThread, NULL);
			pthread_join(thread3, NULL);
			
			if(totalTime > quantumTime)
			{
				tempProcess.commands.front().cycleTime -= quantumTime;
				totalTime = quantumTime;
				rrQueue.push(tempProcess);
				isReset = true;
				simulateProcess(tempCode);
				output(tempConfig.logFile, tempCode, tempCommand, tempCount, startTime, endTime);
				break;
			}
		
			else if(totalTime < quantumTime)
			{
				quantumTime = quantumTime - totalTime;
				isReset = false;
			
				if(quantumTime < 0)
				{
					quantumTime = 0;
				}
			}
		}
		
		//run if earlier criteria not met
		simulateProcess(tempCode);
		output(tempConfig.logFile, tempCode, tempCommand, tempCount, startTime, endTime);
		tempPCB.processState = PCBexit;
		tempProcess.commands.pop();
	}
	
	//exit process control block
	tempPCB.processState = PCBexit;
}

/**
@brief  round robin thread
@param  void *threadid
@return  none
@pre  none
@post  none
**/
void* rrThread(void *threadid)
{	
	pthread_exit(0);
}

/**
@brief  function that simulates the process task
@param  string opCode
@return  none
@pre  ioProcess(), nonIOProcess()
@post  outputs start and end time for a process task
**/
void simulateProcess(string opCode)
{
	//declare thread
	pthread_t thread1;
	
	//if input/output, run thread using ioProcess()
	if(opCode == "input" || opCode == "output")
	{
		pthread_create(&thread1, NULL, &ioProcess, NULL);
		pthread_join(thread1, NULL);
		tempPCB.processState = PCBwaiting;
		tempPCB.processState = PCBexit;
	}

	//else, call nonIOProcess()
	else
	{
		nonIOProcess();
	}
}

/**
@brief  a function that adds processes to the end of STR or RR queue
@param  none
@return  none
@pre  none
@post  every 100 ms, a duplicate of the earlier processes will be added, up to 10 times
**/
void loadProcess()
{
	if (totalCount < 9)
	{
		//delay(100);
		queue<processData> tempQueue;
		tempQueue = masterQueue;
		
		while(!tempQueue.empty())
		{	
			tempQueue.front().processCount = ++processID;
			
			if(strs == true)
			{
				strQueue.push(tempQueue.front());
			}
			
			else
			{
				rrQueue.push(tempQueue.front());
			}
			
			tempQueue.pop();
		}
		
		totalCount++;
	}
}

/**
@brief  a function that adds processes to the end of STR or RR queue
@param  void *threadid
@return  none
@pre  delay()
@post  every 100 ms, a duplicate of the earlier processes will be added, up to 10 times
**/
void* loadThread(void *threadid)
{
	//mutex lock
	pthread_mutex_lock(&mtx2);
	
	if (totalCount < 9)
	{
		//delay(100);
		queue<processData> tempQueue;
		tempQueue = masterQueue;
		
		while(!tempQueue.empty())
		{	
			tempQueue.front().processCount = ++processID;
			
			if(strs == true)
			{
				strQueue.push(tempQueue.front());
			}
			
			else
			{
				rrQueue.push(tempQueue.front());
			}
			
			tempQueue.pop();
		}
		
		totalCount++;
	}
	
	//mutex unlock and exit the thread
  	pthread_mutex_unlock(&mtx2);
	pthread_exit(0);
}


/**
@brief  a function that is similar to nonIOProcess() but is for an input/output cycle that uses a thread
@param  void *threadid
@return  none
@pre  delay()
@post  takes the global tempInput.processValue and puts the cycle time into tempTimer.StartValue and tempTimer.EndValue
**/
void* ioProcess(void *threadid)
{
	//mutex lock
	pthread_mutex_lock(&mtx);
	
	//process time start
	startTime = (clock()-timerStart)/(double)(CLOCKS_PER_SEC);
	
	//simulate delay
 	delay(totalTime);
	
	//process time end
	endTime = (clock()-timerStart)/(double)(CLOCKS_PER_SEC);	
  	
  	//mutex unlock and exit the thread
  	pthread_mutex_unlock(&mtx);	
  	pthread_exit(0);
}

/**
@brief a function that simulates a cycle time for a given process, not for input/output
@param  none
@return  none
@pre  delay()
@post  takes the global tempInput.processValue and puts the cycle time into tempTimer.StartValue and tempTimer.EndValue
**/
void nonIOProcess()
{	
	//process time start
	startTime = (clock()-timerStart)/(double)(CLOCKS_PER_SEC);
	
	//simulate delay
 	delay(totalTime);
	
	//process time end
	endTime = (clock()-timerStart)/(double)(CLOCKS_PER_SEC);	
}

/**
@brief  a function that simulates a delay based on the process time
@param  int milliseconds
@return  none
@pre  none
@post  none
**/
void delay(int milliseconds)
{
	long duration = milliseconds * (CLOCKS_PER_SEC/1000);
	clock_t alpha, omega;
	
	omega = alpha = clock();
	while ( (omega - alpha) < duration)
	{
			omega = clock();
	}
}

/**
@brief  runs an output print for the start of a simulation
@param  string filename
@return  none
@pre  none
@post  prints the start time for a program simulation
**/
void startOutput(string filename)
{
	ofstream fout;
	fout.open(filename.c_str(),ios::app);
	
	double currentClock = (clock()-clockStart)/(double)(CLOCKS_PER_SEC);
			
	if(logTo == 'M')
	{
		cout << fixed << currentClock << " - Simulator program starting" << endl;
	}
	
	else if(logTo == 'F')
	{
		fout << fixed << currentClock << " - Simulator program starting" << endl;
	}
	
	else
	{
		cout << fixed << currentClock << " - Simulator program starting" << endl;
		fout << fixed << currentClock << " - Simulator program starting" << endl;
	}
	
	fout.close();	
}

/**
@brief  runs an output print for the end of a simulation
@param  string filename
@return  none
@pre  none
@post  prints the end time for a program simulation
**/
void endOutput(string filename)
{
	ofstream fout;
	fout.open(filename.c_str(),ios::app);

	double clockEnd = (clock()-clockStart)/(double)(CLOCKS_PER_SEC);
			
	if(logTo == 'M')
	{
		cout << fixed << clockEnd << " - Simulator program ending" << endl;
	}
	
	else if(logTo == 'F')
	{
		fout << fixed << clockEnd << " - Simulator program ending" << endl;
	}
	
	else
	{
		cout << fixed << clockEnd << " - Simulator program ending" << endl;
		fout << fixed << clockEnd << " - Simulator program ending" << endl;
	}
	
	fout.close();	
}

/**
@brief  priority scheduling comparative operator
@param  processData a, processData b
@return  bool
@pre  none
@post  none
**/

bool operator>(processData x, processData y)
{
	return (x.countIO < y.countIO);
}

/**
@brief  shortest job first scheduling comparative operator
@param  processData a, processData b
@return  none
@pre  none
@post  none
**/

bool operator<(processData x, processData y)
{
	return (x.countTask > y.countTask);
}
