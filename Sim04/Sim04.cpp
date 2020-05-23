/**
@file  Sim04.cpp
@author  Christopher Eichstedt
@version  1.2 - changed the application read in "begin" instead of "start"
@brief  "Assignment 4 will allow you to increase throughput in your simulator by implementing scheduling algorithms."
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

struct timerData
{
	queue<string> timerOpCode;
	queue<string> timerOpCommand;
	queue<double> timerStartValue;
	queue<double> timerEndValue;
	queue<int> timerProcessID;
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
timerData tempTimer;
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
int totalTime;
pthread_mutex_t mtx;
int processID;

//scheduling 
bool fifos = false; //first in first out scheduling
bool ps = false; //priority scheduling
bool sjfs = false; //shortest job first scheduling

//-------------------------
//function declarations
//-------------------------
void readConfig(char*) throw(runtime_error);
void readInput(string) throw(runtime_error);
void output(string);
void memoryAllocate();
void timerProcess(processData);
void* threadProcess(void*);
void cycleProcess();
void delay (int);
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
	output(tempConfig.logFile);
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
				for(int i = 0; i < 2; i++)
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
			
			if(tempQ.front() == "SJF")
			{
				sjfs = true;
			}
			
			else if(tempQ.front() == "PS")
			{
				ps = true;
			}
			
			else
			{
				fifos = true;
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
	//scheduling queues
	queue<processData> fifoQueue;
	priority_queue<processData, vector<processData>, greater<processData> > priorQueue;
	priority_queue<processData, vector<processData>, less<processData> > sjfQueue;
	
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
			tempInput.cycleTime = 0;
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
			tempInput.cycleTime = 0;
			tempProcess.commands.push(tempInput);
			tempProcess.processCount = ++processID;
			
			//using shortest job first scheduling
			if (sjfs == true)
			{
				sjfQueue.push(tempProcess);	 
			}
	
			//using priority scheduling
			else if (ps == true)
			{
				priorQueue.push(tempProcess);	
			}
	
			//using first in, first out scheduling
			else
			{
				fifoQueue.push(tempProcess);
			}
		}
	}
	
	//close file
	fin.close();
	
	//build temp queue to filter through
	queue<processData> tempQueue;
	
	if (sjfs == true)
	{
		while(!sjfQueue.empty())
	  	{
	  		tempQueue.push(sjfQueue.top());
	    		sjfQueue.pop();
	  	}
	}
	
	else if (ps == true)
	{
		while(!priorQueue.empty())
	      	{
		  	tempQueue.push(priorQueue.top());
		  	priorQueue.pop();
	      	}
	}

	else
	{
		tempQueue = fifoQueue;
	}
	
	//running timer process using temp queue
	processData tempProcess2;
	
	while(!tempQueue.empty())
	{
		tempProcess2 = tempQueue.front();
		timerProcess(tempProcess2);
		tempQueue.pop();
	}
}
/**
@brief  a function that outputs the data stored in the global structs for the config and meta-data
@param  string filename
@return  none
@pre  timerProcess()
@post  prints to file, monitor or both with the information taken from the user's config and meta-data information 
**/
void output(string filename)
{
	//open file
	ofstream fout;
	fout.open(filename.c_str());
	
	//system start print out		
	if(logTo == 'M')
	{
		cout << fixed << clockStart << " - Simulator program starting" << endl;
	}
	
	else if(logTo == 'F')
	{
		fout << fixed << clockStart << " - Simulator program starting" << endl;
	}
	
	else
	{
		cout << fixed << clockStart << " - Simulator program starting" << endl;
		fout << fixed << clockStart << " - Simulator program starting" << endl;
	}	
	
	while(!tempTimer.timerStartValue.empty() && !tempTimer.timerEndValue.empty())
	{
		//input operation
		if(tempTimer.timerOpCode.front() == "input")
		{	
			//keyboard command
			if(tempTimer.timerOpCommand.front() == "keyboard")
			{
				if(logTo == 'M')
				{
					cout << fixed << tempTimer.timerStartValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": start keyboard input" << endl;
					cout << fixed << tempTimer.timerEndValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": end keyboard input" << endl;
				}
				
				else if(logTo == 'F')
				{
					fout << fixed << tempTimer.timerStartValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": start keyboard input" << endl;
					fout << fixed << tempTimer.timerEndValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": end keyboard input" << endl;
				}
				
				else
				{
					cout << fixed << tempTimer.timerStartValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": start keyboard input" << endl;
					cout << fixed << tempTimer.timerEndValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": end keyboard input" << endl;
					fout << fixed << tempTimer.timerStartValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": start keyboard input" << endl;
					fout << fixed << tempTimer.timerEndValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": end keyboard input" << endl;
				}
			}
			
			//hdd command
			else if(tempTimer.timerOpCommand.front() == "hdd")
			{
				if(logTo == 'M')
				{
					cout << fixed << tempTimer.timerStartValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": start hard drive input on HDD " << (tempConfig.countHDDIn % tempConfig.numHDD) << endl;;
					cout << fixed << tempTimer.timerEndValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": end hard drive input on HDD " << (tempConfig.countHDDIn % tempConfig.numHDD) << endl;
				}
			
				else if(logTo == 'F')
				{
					fout << fixed << tempTimer.timerStartValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": start hard drive input on HDD " << (tempConfig.countHDDIn % tempConfig.numHDD) << endl;;
					fout << fixed << tempTimer.timerEndValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": end hard drive input on HDD " << (tempConfig.countHDDIn % tempConfig.numHDD) << endl;
				}
				
				else
				{
					cout << fixed << tempTimer.timerStartValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": start hard drive input on HDD " << (tempConfig.countHDDIn % tempConfig.numHDD) << endl;;
					cout << fixed << tempTimer.timerEndValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": end hard drive input on HDD " << (tempConfig.countHDDIn % tempConfig.numHDD) << endl;
					fout << fixed << tempTimer.timerStartValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": start hard drive input on HDD " << (tempConfig.countHDDIn % tempConfig.numHDD) << endl;;
					fout << fixed << tempTimer.timerEndValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": end hard drive input on HDD " << (tempConfig.countHDDIn % tempConfig.numHDD) << endl;
				}

				tempConfig.countHDDIn++;
			}
			
			//scanner command
			else if(tempTimer.timerOpCommand.front() == "scanner")
			{
				if(logTo == 'M')
				{
					cout << fixed << tempTimer.timerStartValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": start scanner input" << endl;
					cout << fixed << tempTimer.timerEndValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": end scanner input" << endl;
				}
			
				else if(logTo == 'F')
				{
					fout << fixed << tempTimer.timerStartValue.front() << " -Process " << tempTimer.timerProcessID.front() << ": start scanner input" << endl;
					fout << fixed << tempTimer.timerEndValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": end scanner input" << endl;
				}
				
				else
				{
					cout << fixed << tempTimer.timerStartValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": start scanner input" << endl;
					cout << fixed << tempTimer.timerEndValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": end scanner input" << endl;
					fout << fixed << tempTimer.timerStartValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": start scanner input" << endl;
					fout << fixed << tempTimer.timerEndValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": end scanner input" << endl;
				}
			}
		}
		
		//output operation
		else if(tempTimer.timerOpCode.front() == "output")
		{
			//monitor command
			if(tempTimer.timerOpCommand.front() == "monitor")
			{
				if(logTo == 'M')
				{
					cout << fixed << tempTimer.timerStartValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": start monitor output" << endl;
					cout << fixed << tempTimer.timerEndValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": end monitor output" << endl;
				}
				else if(logTo == 'F')
				{
					fout << fixed << tempTimer.timerStartValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": start monitor output" << endl;
					fout << fixed << tempTimer.timerEndValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": end monitor output" << endl;
				}
				
				else
				{
					cout << fixed << tempTimer.timerStartValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": start monitor output" << endl;
					cout << fixed << tempTimer.timerEndValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": end monitor output" << endl;
					fout << fixed << tempTimer.timerStartValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": start monitor output" << endl;
					fout << fixed << tempTimer.timerEndValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": end monitor output" << endl;
				}
			}
			
			//projector command
			else if(tempTimer.timerOpCommand.front() == "projector")
			{
				if(logTo == 'M')
				{
					cout << fixed << tempTimer.timerStartValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": start projector output on PROJ " << (tempConfig.countProjOut % tempConfig.numProj) << endl;
					cout << fixed << tempTimer.timerEndValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": end projector output on PROJ " << (tempConfig.countProjOut % tempConfig.numProj) << endl;
				}
				else if(logTo == 'F')
				{
					fout << fixed << tempTimer.timerStartValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": start projector output on PROJ " << (tempConfig.countProjOut % tempConfig.numProj) << endl;
					fout << fixed << tempTimer.timerEndValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": end projector output on PROJ " << (tempConfig.countProjOut % tempConfig.numProj) << endl;
				}
				
				else
				{
					cout << fixed << tempTimer.timerStartValue.front() << " - Process " << tempTimer.timerProcessID.front() << ":start projector output on PROJ " << (tempConfig.countProjOut % tempConfig.numProj) << endl;
					cout << fixed << tempTimer.timerEndValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": end projector output on PROJ " << (tempConfig.countProjOut % tempConfig.numProj) << endl;
					fout << fixed << tempTimer.timerStartValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": start projector output on PROJ " << (tempConfig.countProjOut % tempConfig.numProj) << endl;
					fout << fixed << tempTimer.timerEndValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": end projector output on PROJ " << (tempConfig.countProjOut % tempConfig.numProj) << endl;
				}
			
				tempConfig.countProjOut++;
			}
			
			//hdd command
			else if(tempTimer.timerOpCommand.front() == "hdd")
			{
				if(logTo == 'M')
				{
					cout << fixed << tempTimer.timerStartValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": start hard drive output on HDD " << (tempConfig.countHDDOut % tempConfig.numHDD) << endl;;
					cout << fixed << tempTimer.timerEndValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": end hard drive output on HDD " << (tempConfig.countHDDOut % tempConfig.numHDD) << endl;
				}
				else if(logTo == 'F')
				{
					fout << fixed << tempTimer.timerStartValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": start hard drive output on HDD " << (tempConfig.countHDDOut % tempConfig.numHDD) << endl;;
					fout << fixed << tempTimer.timerEndValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": end hard drive output on HDD " << (tempConfig.countHDDOut % tempConfig.numHDD) << endl;
				}
				
				else
				{
					cout << fixed << tempTimer.timerStartValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": start hard drive output on HDD " << (tempConfig.countHDDOut % tempConfig.numHDD) << endl;;
					cout << fixed << tempTimer.timerEndValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": end hard drive output on HDD " << (tempConfig.countHDDOut % tempConfig.numHDD) << endl;
					fout << fixed << tempTimer.timerStartValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": start hard drive output on HDD " << (tempConfig.countHDDOut % tempConfig.numHDD) << endl;;
					fout << fixed << tempTimer.timerEndValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": end hard drive output on HDD " << (tempConfig.countHDDOut % tempConfig.numHDD) << endl;
				}
			
				tempConfig.countHDDOut++;
			}
		}
		
		else
		{
			//memory operation
			if(tempTimer.timerOpCode.front() == "memory")
			{
				//allocate command
				if(tempTimer.timerOpCommand.front() == "allocate")
				{
					if(tempMemory.count > tempMemory.total)
					{
						tempMemory.count = 0;
					}
					
					if(logTo == 'M')
					{
						cout << fixed << tempTimer.timerStartValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": allocating memory" << endl;
						cout << fixed << tempTimer.timerEndValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": memory allocated at " << "0x" << setfill('0') << setw(8) << hex << tempMemory.count << endl;
					}
			
					else if(logTo == 'F')
					{
						fout << fixed << tempTimer.timerStartValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": allocating memory" << endl;
						fout << fixed << tempTimer.timerEndValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": memory allocated at " << "0x" << setfill('0') << setw(8) << hex << tempMemory.count << endl;
					}
					
					else
					{
						cout << fixed << tempTimer.timerStartValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": allocating memory" << endl;
						cout << fixed << tempTimer.timerEndValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": memory allocated at " << "0x" << setfill('0') << setw(8) << hex << tempMemory.count << endl;
						fout << fixed << tempTimer.timerStartValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": allocating memory" << endl;
						fout << fixed << tempTimer.timerEndValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": memory allocated at " << "0x" << setfill('0') << setw(8) << hex << tempMemory.count << endl;
					}
					
					tempMemory.count += tempMemory.blockSize;
				}
				
				//block command
				else if(tempTimer.timerOpCommand.front() == "block")
				{
					if(logTo == 'M')
					{
						cout << fixed << tempTimer.timerStartValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": start memory blocking" << endl;
						cout << fixed << tempTimer.timerEndValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": end memory blocking" << endl;
					}
					
					else if(logTo == 'F')
					{
						fout << fixed << tempTimer.timerStartValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": start memory blocking" << endl;
						fout << fixed << tempTimer.timerEndValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": end memory blocking" << endl;
					}
					
					else
					{
						cout << fixed << tempTimer.timerStartValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": start memory blocking" << endl;
						cout << fixed << tempTimer.timerEndValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": end memory blocking" << endl;
						fout << fixed << tempTimer.timerStartValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": start memory blocking" << endl;
						fout << fixed << tempTimer.timerEndValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": end memory blocking" << endl;
					}
					
				}
			}
			
			//application
			else if(tempTimer.timerOpCode.front() == "application")
			{
				//start process
				if(tempTimer.timerOpCommand.front() == "start")
				{	
					if(logTo == 'M')
					{
						cout << fixed << tempTimer.timerStartValue.front() << " - OS: preparing process " << tempTimer.timerProcessID.front() << endl;
						cout << fixed << tempTimer.timerStartValue.front() << " - OS: starting process " << tempTimer.timerProcessID.front() << endl;
					}
					
					else if(logTo == 'F')
					{
						fout << fixed << tempTimer.timerStartValue.front() << " - OS: preparing process " << tempTimer.timerProcessID.front() << endl;
						fout << fixed << tempTimer.timerStartValue.front() << " - OS: starting process " << tempTimer.timerProcessID.front() << endl;
					}
					
					else
					{
						cout << fixed << tempTimer.timerStartValue.front() << " - OS: preparing process " << tempTimer.timerProcessID.front() << endl;
						cout << fixed << tempTimer.timerStartValue.front() << " - OS: starting process " << tempTimer.timerProcessID.front() << endl;
						fout << fixed << tempTimer.timerStartValue.front() << " - OS: preparing process " << tempTimer.timerProcessID.front() << endl;
						fout << fixed << tempTimer.timerStartValue.front() << " - OS: starting process " << tempTimer.timerProcessID.front() << endl;
					}
				}
				
				//end process
				else
				{
					if(logTo == 'M')
					{
						cout << fixed << tempTimer.timerStartValue.front() << " - End process " << tempTimer.timerProcessID.front() << endl;
					}
					
					else if(logTo == 'F')
					{
						fout << fixed << tempTimer.timerStartValue.front() << " - End process " << tempTimer.timerProcessID.front() << endl;
					}
					
					else
					{
						cout << fixed << tempTimer.timerStartValue.front() << " - End process " << tempTimer.timerProcessID.front() << endl;
						fout << fixed << tempTimer.timerStartValue.front() << " - End process " << tempTimer.timerProcessID.front() << endl;
					}
				}
			}
			
			//processor
			else
			{
			
				if(logTo == 'M')
				{
					cout << fixed << tempTimer.timerStartValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": start processing action" << endl;
					cout << fixed << tempTimer.timerEndValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": end processing action" << endl;
				}
			
				else if(logTo == 'F')
				{
					fout << fixed << tempTimer.timerStartValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": start processing action" << endl;
					fout << fixed << tempTimer.timerEndValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": end processing action" << endl;
				}
				
				else
				{
					cout << fixed << tempTimer.timerStartValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": start processing action" << endl;
					cout << fixed << tempTimer.timerEndValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": end processing action" << endl;
					fout << fixed << tempTimer.timerStartValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": start processing action" << endl;
					fout << fixed << tempTimer.timerEndValue.front() << " - Process " << tempTimer.timerProcessID.front() << ": end processing action" << endl;
				}
			}
		}
		
		tempTimer.timerOpCode.pop();
		tempTimer.timerOpCommand.pop();
		tempTimer.timerStartValue.pop();
		tempTimer.timerEndValue.pop();
		tempTimer.timerProcessID.pop();
	}
	
	//system end print out
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
@pre  threadProcess(), cycleProcess()
@post  sets the PCB and runs the thread according
**/
void timerProcess(processData tempProcess)
{
	//create thread and start process control block
	pthread_t thread1;
	tempPCB.processState = PCBstart;
	
	while(!tempProcess.commands.empty())
	{	
		tempPCB.processState = PCBrunning;
		totalTime = tempProcess.commands.front().cycleTime;
		tempTimer.timerOpCode.push(tempProcess.commands.front().opCode);
		tempTimer.timerOpCommand.push(tempProcess.commands.front().opCommand);
		tempTimer.timerProcessID.push(tempProcess.processCount);
		
		//if input/output, run thread using threadProcess()
		if(tempProcess.commands.front().opCode == "input" || tempProcess.commands.front().opCode == "output")
		{
			pthread_create(&thread1, NULL, &threadProcess, NULL);
			pthread_join(thread1, NULL);
			tempPCB.processState = PCBwaiting;
			tempPCB.processState = PCBexit;
		}
		
		//else, call cycleProcess()
		else
		{
			cycleProcess();
		}
		
		tempPCB.processState = PCBexit;
		tempProcess.commands.pop();
	}
	
	//exit process control block
	tempPCB.processState = PCBexit;
}

/**
@brief  a function that is similar to cycleProcess() but is for an input/output cycle that uses a thread
@param  void *threadid
@return  none
@pre  delay()
@post  takes the global tempInput.processValue and puts the cycle time into tempTimer.StartValue and tempTimer.EndValue
**/
void* threadProcess(void *threadid)
{
	//mutex lock
	pthread_mutex_lock(&mtx);
	
	//process time start
	startTime = (clock()-timerStart)/(double)(CLOCKS_PER_SEC);
	tempTimer.timerStartValue.push(startTime);
	
	//simulate delay
 	delay(totalTime);
	
	//process time end
	endTime = (clock()-timerStart)/(double)(CLOCKS_PER_SEC);	
  	tempTimer.timerEndValue.push(endTime);
  	
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
void cycleProcess()
{	
	//process time start
	startTime = (clock()-timerStart)/(double)(CLOCKS_PER_SEC);
	tempTimer.timerStartValue.push(startTime);
	
	//simulate delay
 	delay(totalTime);
	
	//process time end
	endTime = (clock()-timerStart)/(double)(CLOCKS_PER_SEC);	
  	tempTimer.timerEndValue.push(endTime);
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
