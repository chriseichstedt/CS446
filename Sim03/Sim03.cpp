/**
@file  Sim03.cpp
@author  Christopher Eichstedt
@version  1.1 - fixed an error with the projector cycle time and a typo on line 263
@brief  The program will take in a command line argument file name, then call a readIn function.
	The function would then read in the content of the configuration file and store it into a struct.
	Then call a read in function for the input file and store its order and values into a queue.
	Finally, call an output function and make the necessary calculations from the config and input files,
	storing them into a file. The program uses threads and a mutex for input/output while simulating cycle times.
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
	string logFile; //log file for output
	string inputFile; //input file for meta data
};

struct inputData
{
	queue<string> processName;
	queue<int> processValue;
};

struct PCB
{
	int processState;
};

struct timerData
{
	queue<string> timerName;
	queue<double> timerStartValue;
	queue<double> timerEndValue;
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
inputData tempInput;
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
double clockStart = 0.000001;
double startTime;
double endTime;

//system
unsigned int memLocation;
string inputFileName;
char logTo;

//-------------------------
//function declarations
//-------------------------
void readConfig(char*) throw(runtime_error);
void readInput(string) throw(runtime_error);
void output(string);
void memoryAllocate();
void timerProcess();
void* threadProcess(void*);
void cycleProcess();
void delay (int);

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
			for(int i = 0; i < 4; i++)
			{
				tempQ.pop();
			}
			
			stringstream tempPCT(tempQ.front());
			tempPCT >> val;
			tempConfig.pct = val;
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
@post  stores meta-data into a queue and stores the calculated cycle time into another
**/
void readInput(string filename) throw(runtime_error)
{
	//open file
	ifstream fin;
	fin.open(filename.c_str());
	queue<string> tempQ;
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
		tempQ.push(tempS);
	}
	
	//close file
	fin.close();
	
	//store file data into global struct
	while(!tempQ.empty())
	{
		string tempStr = tempQ.front();
		string tempCutStr = tempStr.substr(0, 3);
		string tempVal;
		int val, bracket, semiColon;
		
		//operating system(start)
		if(tempCutStr == "S{s")
		{
			tempInput.processName.push(tempStr);
			tempInput.processValue.push(0);
			tempQ.pop();
		}
		
		//operating system(finish)
		else if(tempCutStr == "S{f")
		{
			tempInput.processName.push(tempStr);
			tempInput.processValue.push(0);
			tempQ.pop();
		}
		
		//application(start)
		else if(tempCutStr == "A{s")
		{
			tempInput.processName.push(tempStr);
			tempInput.processValue.push(0);
			tempQ.pop();
		}
		
		//application(finish)
		else if(tempCutStr == "A{f")
		{
			tempInput.processName.push(tempStr);
			tempInput.processValue.push(0);
			tempQ.pop();
		}

		//processor(run)
		else if(tempCutStr == "P{r")
		{
			bracket = tempStr.find("}");
			semiColon = tempStr.find(";");
			tempVal = tempStr.substr(bracket+1, ((semiColon-bracket) - 1));
			
			stringstream tempPRPV(tempVal);
			tempPRPV >> val;
			tempInput.processName.push(tempStr);
			tempInput.processValue.push(val*tempConfig.pct);
			tempQ.pop();
		}
		
		//memory(allocate)
		else if(tempCutStr == "M{a")
		{
			bracket = tempStr.find("}");
			semiColon = tempStr.find(";");
			tempVal = tempStr.substr(bracket+1, ((semiColon-bracket) - 1));
			
			stringstream tempMAPV(tempVal);
			tempMAPV >> val;
			tempInput.processName.push(tempStr);
			tempInput.processValue.push(val*tempConfig.mct);
			tempQ.pop();
		}

		//memory(block)
		else if(tempCutStr == "M{b")
		{
			bracket = tempStr.find("}");
			semiColon = tempStr.find(";");
			tempVal = tempStr.substr(bracket+1, ((semiColon-bracket) - 1));
			
			stringstream tempMBPV(tempVal);
			tempMBPV >> val;
			tempInput.processName.push(tempStr);
			tempInput.processValue.push(val*tempConfig.mct);
			tempQ.pop();
		}
		
		
		//output(monitor)
		else if(tempCutStr == "O{m")
		{
			bracket = tempStr.find("}");
			semiColon = tempStr.find(";");
			tempVal = tempStr.substr(bracket+1, ((semiColon-bracket) - 1));
			
			stringstream tempOMPV(tempVal);
			tempOMPV >> val;
			tempInput.processName.push(tempStr);
			tempInput.processValue.push(val*tempConfig.mdt);
			tempQ.pop();
		}
		
		//output(projector)
		else if(tempCutStr == "O{p")
		{
			bracket = tempStr.find("}");
			semiColon = tempStr.find(";");
			tempVal = tempStr.substr(bracket+1, ((semiColon-bracket) - 1));
			
			stringstream tempOPPV(tempVal);
			tempOPPV >> val;
			tempInput.processName.push(tempStr);
			tempInput.processValue.push(val*tempConfig.pjct);
			tempQ.pop();
		}
		
		//output(hard drive)
		else if(tempCutStr == "O{h")
		{
			tempQ.pop();
			tempStr = tempStr + " " + tempQ.front();
			bracket = tempStr.find("}");
			semiColon = tempStr.find(";");
			tempVal = tempStr.substr(bracket+1, ((semiColon-bracket) - 1));
			
			stringstream tempOHPV(tempVal);
			tempOHPV >> val;
			tempInput.processName.push(tempStr);
			tempInput.processValue.push(val*tempConfig.hdct);
			tempQ.pop();
		}
		
		//input(keyboard)
		else if(tempCutStr == "I{k")
		{
			bracket = tempStr.find("}");
			semiColon = tempStr.find(";");
			tempVal = tempStr.substr(bracket+1, ((semiColon-bracket) - 1));
			
			stringstream tempIKPV(tempVal);
			tempIKPV >> val;
			tempInput.processName.push(tempStr);
			tempInput.processValue.push(val*tempConfig.kct);
			tempQ.pop();
		}
		
		//input(hard drive)
		else if(tempCutStr == "I{h")
		{
			tempQ.pop();
			tempStr = tempStr + " " + tempQ.front();
			bracket = tempStr.find("}");
			semiColon = tempStr.find(";");
			tempVal = tempStr.substr(bracket+1, ((semiColon-bracket) - 1));
			
			stringstream tempIHPV(tempVal);
			tempIHPV >> val;
			tempInput.processName.push(tempStr);
			tempInput.processValue.push(val*tempConfig.hdct);
			tempQ.pop();
		}
		
		//input(scanner)
		else if(tempCutStr == "I{s")
		{
			bracket = tempStr.find("}");
			semiColon = tempStr.find(";");
			tempVal = tempStr.substr(bracket+1, ((semiColon-bracket) - 1));
			
			stringstream tempISPV(tempVal);
			tempISPV >> val;
			tempInput.processName.push(tempStr);
			tempInput.processValue.push(val*tempConfig.sct);
			tempQ.pop();
		}
		
		else
		{
			tempQ.pop();
		}
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
	//run the timer process
	timerProcess();
	
	//output if logTo is to File
	if (logTo == 'F')
	{
		//open file
		ofstream fout;
		fout.open(filename.c_str());
		fout << fixed << clockStart << " - Simulator program starting" << endl;
		
		while(!tempTimer.timerName.empty() && !tempTimer.timerStartValue.empty() && !tempTimer.timerEndValue.empty())
		{
			string tempStr = tempTimer.timerName.front();
			string tempCutStr = tempStr.substr(0, 3);
		
			//operating system(start)
			if(tempCutStr == "S{s")
			{
				fout << fixed << tempTimer.timerEndValue.front() << " - OS: preparing process 1" << endl;
				tempTimer.timerName.pop();
				tempTimer.timerStartValue.pop();
				tempTimer.timerEndValue.pop();
			}
		
			//operating system(finish)
			else if(tempCutStr == "S{f")
			{
				fout << fixed << tempTimer.timerEndValue.front() << " - Simulator program ending" << endl;
				tempTimer.timerName.pop();
				tempTimer.timerStartValue.pop();
				tempTimer.timerEndValue.pop();
			}
		
			//application(start)
			else if(tempCutStr == "A{s")
			{
				fout << fixed << tempTimer.timerEndValue.front() << " - OS: starting process 1" << endl;
				tempTimer.timerName.pop();
				tempTimer.timerStartValue.pop();
				tempTimer.timerEndValue.pop();
			}
		
			//application(finish)
			else if(tempCutStr == "A{f")
			{
				fout << fixed << tempTimer.timerEndValue.front() << " - OS: removing process 1" << endl;
				tempTimer.timerName.pop();
				tempTimer.timerStartValue.pop();
				tempTimer.timerEndValue.pop();
			}

			//processor(run)
			else if(tempCutStr == "P{r")
			{
				fout << fixed << tempTimer.timerStartValue.front() << " - Process 1: start processing action" << endl;
				fout << fixed << tempTimer.timerEndValue.front() << " - Process 1: end processing action" << endl;
				tempTimer.timerName.pop();
				tempTimer.timerStartValue.pop();
				tempTimer.timerEndValue.pop();
			}
		
			//memory(allocate)
			else if(tempCutStr == "M{a")
			{
				if(tempMemory.count > tempMemory.total)
				{
					tempMemory.count = 0;
				}
				
				fout << fixed << tempTimer.timerStartValue.front() << " - Process 1: allocating memory" << endl;
				fout << fixed << tempTimer.timerEndValue.front() << " - Process 1: memory allocated at " << "0x" << setfill('0') << setw(8) << hex << tempMemory.count << endl;
				tempTimer.timerName.pop();
				tempTimer.timerStartValue.pop();
				tempTimer.timerEndValue.pop();
				tempMemory.count += tempMemory.blockSize;
				
			}

			//memory(block)
			else if(tempCutStr == "M{b")
			{
				fout << fixed << tempTimer.timerStartValue.front() << " - Process 1: start memory blocking" << endl;
				fout << fixed << tempTimer.timerEndValue.front() << " - Process 1: end memory blocking" << endl;
				tempTimer.timerName.pop();
				tempTimer.timerStartValue.pop();
				tempTimer.timerEndValue.pop();
			}
		
		
			//output(monitor)
			else if(tempCutStr == "O{m")
			{
				fout << fixed << tempTimer.timerStartValue.front() << " - Process 1: start monitor output" << endl;
				fout << fixed << tempTimer.timerEndValue.front() << " - Process 1: end monitor output" << endl;
				tempTimer.timerName.pop();
				tempTimer.timerStartValue.pop();
				tempTimer.timerEndValue.pop();
			}
		
			//output(projector)
			else if(tempCutStr == "O{p")
			{
				fout << fixed << tempTimer.timerStartValue.front() << " - Process 1: start projector output on PROJ " << (tempConfig.countProjOut % tempConfig.numProj) << endl;
				fout << fixed << tempTimer.timerEndValue.front() << " - Process 1: end projector output on PROJ " << (tempConfig.countProjOut % tempConfig.numProj) << endl;
				tempTimer.timerName.pop();
				tempTimer.timerStartValue.pop();
				tempTimer.timerEndValue.pop();
				tempConfig.countProjOut++;
			}
		
			//output(hard drive)
			else if(tempCutStr == "O{h")
			{
				fout << fixed << tempTimer.timerStartValue.front() << " - Process 1: start hard drive output on HDD " << (tempConfig.countHDDOut % tempConfig.numHDD) << endl;;
				fout << fixed << tempTimer.timerEndValue.front() << " - Process 1: end hard drive output on HDD " << (tempConfig.countHDDOut % tempConfig.numHDD) << endl;
				tempTimer.timerName.pop();
				tempTimer.timerStartValue.pop();
				tempTimer.timerEndValue.pop();
				tempConfig.countHDDOut++;
			}
			
			//input(keyboard)
			else if(tempCutStr == "I{k")
			{
				fout << fixed << tempTimer.timerStartValue.front() << " - Process 1: start keyboard input" << endl;
				fout << fixed << tempTimer.timerEndValue.front() << " - Process 1: end keyboard input" << endl;
				tempTimer.timerName.pop();
				tempTimer.timerStartValue.pop();
				tempTimer.timerEndValue.pop();
			}
		
			//input(hard drive)
			else if(tempCutStr == "I{h")
			{
				fout << fixed << tempTimer.timerStartValue.front() << " - Process 1: start hard drive input on HDD " << (tempConfig.countHDDIn % tempConfig.numHDD) << endl;;
				fout << fixed << tempTimer.timerEndValue.front() << " - Process 1: end hard drive input on HDD " << (tempConfig.countHDDIn % tempConfig.numHDD) << endl;
				tempTimer.timerName.pop();
				tempTimer.timerStartValue.pop();
				tempTimer.timerEndValue.pop();
				tempConfig.countHDDIn++;
			}
		
			//input(scanner)
			else if(tempCutStr == "I{s")
			{
				fout << fixed << tempTimer.timerStartValue.front() << " - Process 1: start scanner input" << endl;
				fout << fixed << tempTimer.timerEndValue.front() << " - Process 1: end scanner input" << endl;
				tempTimer.timerName.pop();
				tempTimer.timerStartValue.pop();
				tempTimer.timerEndValue.pop();
			}
		}
		
		//close file
		fout.close();
	}
	
	//output if logTo is to Monitor
	else if (logTo == 'M')
	{
		cout << fixed << clockStart << " - Simulator program starting" << endl;
		while(!tempTimer.timerName.empty() && !tempTimer.timerStartValue.empty() && !tempTimer.timerEndValue.empty())
		{
			string tempStr = tempTimer.timerName.front();
			string tempCutStr = tempStr.substr(0, 3);
		
			//operating system(start)
			if(tempCutStr == "S{s")
			{
				cout << fixed << tempTimer.timerEndValue.front() << " - OS: preparing process 1" << endl;
				tempTimer.timerName.pop();
				tempTimer.timerStartValue.pop();
				tempTimer.timerEndValue.pop();
			}
		
			//operating system(finish)
			else if(tempCutStr == "S{f")
			{
				cout << fixed << tempTimer.timerEndValue.front() << " - Simulator program ending" << endl;
				tempTimer.timerName.pop();
				tempTimer.timerStartValue.pop();
				tempTimer.timerEndValue.pop();
			}
		
			//application(start)
			else if(tempCutStr == "A{s")
			{
				cout << fixed << tempTimer.timerEndValue.front() << " - OS: starting process 1" << endl;
				tempTimer.timerName.pop();
				tempTimer.timerStartValue.pop();
				tempTimer.timerEndValue.pop();
			}
		
			//application(finish)
			else if(tempCutStr == "A{f")
			{
				cout << fixed << tempTimer.timerEndValue.front() << " - OS: removing process 1" << endl;
				tempTimer.timerName.pop();
				tempTimer.timerStartValue.pop();
				tempTimer.timerEndValue.pop();
			}

			//processor(run)
			else if(tempCutStr == "P{r")
			{
				cout << fixed << tempTimer.timerStartValue.front() << " - Process 1: start processing action" << endl;
				cout << fixed << tempTimer.timerEndValue.front() << " - Process 1: end processing action" << endl;
				tempTimer.timerName.pop();
				tempTimer.timerStartValue.pop();
				tempTimer.timerEndValue.pop();
			}
		
			//memory(allocate)
			else if(tempCutStr == "M{a")
			{
				if(tempMemory.count > tempMemory.total)
				{
					tempMemory.count = 0;
				}
				
				cout << fixed << tempTimer.timerStartValue.front() << " - Process 1: allocating memory" << endl;
				cout << fixed << tempTimer.timerEndValue.front() << " - Process 1: memory allocated at " << "0x" << setfill('0') << setw(8) << hex << tempMemory.count << endl;
				tempTimer.timerName.pop();
				tempTimer.timerStartValue.pop();
				tempTimer.timerEndValue.pop();
				tempMemory.count += tempMemory.blockSize;
			}

			//memory(block)
			else if(tempCutStr == "M{b")
			{
				cout << fixed << tempTimer.timerStartValue.front() << " - Process 1: start memory blocking" << endl;
				cout << fixed << tempTimer.timerEndValue.front() << " - Process 1: end memory blocking" << endl;
				tempTimer.timerName.pop();
				tempTimer.timerStartValue.pop();
				tempTimer.timerEndValue.pop();
			}
		
		
			//output(monitor)
			else if(tempCutStr == "O{m")
			{
				cout << fixed << tempTimer.timerStartValue.front() << " - Process 1: start monitor output" << endl;
				cout << fixed << tempTimer.timerEndValue.front() << " - Process 1: end monitor output" << endl;
				tempTimer.timerName.pop();
				tempTimer.timerStartValue.pop();
				tempTimer.timerEndValue.pop();
			}
		
			//output(projector)
			else if(tempCutStr == "O{p")
			{
				cout << fixed << tempTimer.timerStartValue.front() << " - Process 1: start projector output on PROJ " << (tempConfig.countProjOut % tempConfig.numProj) << endl;
				cout << fixed << tempTimer.timerEndValue.front() << " - Process 1: end projector output on PROJ " << (tempConfig.countProjOut % tempConfig.numProj) << endl;
				tempTimer.timerName.pop();
				tempTimer.timerStartValue.pop();
				tempTimer.timerEndValue.pop();
				tempConfig.countProjOut++;
			}
		
			//output(hard drive)
			else if(tempCutStr == "O{h")
			{
				cout << fixed << tempTimer.timerStartValue.front() << " - Process 1: start hard drive output on HDD " << (tempConfig.countHDDOut % tempConfig.numHDD) << endl;;
				cout << fixed << tempTimer.timerEndValue.front() << " - Process 1: end hard drive output on HDD " << (tempConfig.countHDDOut % tempConfig.numHDD) << endl;
				tempTimer.timerName.pop();
				tempTimer.timerStartValue.pop();
				tempTimer.timerEndValue.pop();
				tempConfig.countHDDOut++;
			}
		
			//input(keyboard)
			else if(tempCutStr == "I{k")
			{
				cout << fixed << tempTimer.timerStartValue.front() << " - Process 1: start keyboard input" << endl;
				cout << fixed << tempTimer.timerEndValue.front() << " - Process 1: end keyboard input" << endl;
				tempTimer.timerName.pop();
				tempTimer.timerStartValue.pop();
				tempTimer.timerEndValue.pop();
			}
		
			//input(hard drive)
			else if(tempCutStr == "I{h")
			{
				cout << fixed << tempTimer.timerStartValue.front() << " - Process 1: start hard drive input on HDD " << (tempConfig.countHDDIn % tempConfig.numHDD) << endl;;
				cout << fixed << tempTimer.timerEndValue.front() << " - Process 1: end hard drive input on HDD " << (tempConfig.countHDDIn % tempConfig.numHDD) << endl;
				tempTimer.timerName.pop();
				tempTimer.timerStartValue.pop();
				tempTimer.timerEndValue.pop();
				tempConfig.countHDDIn++;
			}
		
			//input(scanner)
			else if(tempCutStr == "I{s")
			{
				cout << fixed << tempTimer.timerStartValue.front() << " - Process 1: start scanner input" << endl;
				cout << fixed << tempTimer.timerEndValue.front() << " - Process 1: end scanner input" << endl;
				tempTimer.timerName.pop();
				tempTimer.timerStartValue.pop();
				tempTimer.timerEndValue.pop();
			}
		}
	}
	
	//output if logTo is to Both
	else if (logTo == 'B')
	{
		//open file
		ofstream fout;
		fout.open(filename.c_str());
		fout << fixed << clockStart << " - Simulator program starting" << endl;
		cout << fixed << clockStart << " - Simulator program starting" << endl;
		
		while(!tempTimer.timerName.empty() && !tempTimer.timerStartValue.empty() && !tempTimer.timerEndValue.empty())
		{
			string tempStr = tempTimer.timerName.front();
			string tempCutStr = tempStr.substr(0, 3);
		
			//operating system(start)
			if(tempCutStr == "S{s")
			{
				fout << fixed << tempTimer.timerEndValue.front() << " - OS: preparing process 1" << endl;
				cout << fixed << tempTimer.timerEndValue.front() << " - OS: preparing process 1" << endl;
				tempTimer.timerName.pop();
				tempTimer.timerStartValue.pop();
				tempTimer.timerEndValue.pop();
			}
		
			//operating system(finish)
			else if(tempCutStr == "S{f")			{
				fout << fixed << tempTimer.timerEndValue.front() << " - Simulator program ending" << endl;
				cout << fixed << tempTimer.timerEndValue.front() << " - Simulator program ending" << endl;
				tempTimer.timerName.pop();
				tempTimer.timerStartValue.pop();
				tempTimer.timerEndValue.pop();
			}
		
			//application(start)
			else if(tempCutStr == "A{s")
			{
				fout << fixed << tempTimer.timerEndValue.front() << " - OS: starting process 1" << endl;
				cout << fixed << tempTimer.timerEndValue.front() << " - OS: starting process 1" << endl;
				tempTimer.timerName.pop();
				tempTimer.timerStartValue.pop();
				tempTimer.timerEndValue.pop();
			}
		
			//application(finish)
			else if(tempCutStr == "A{f")
			{
				fout << fixed << tempTimer.timerEndValue.front() << " - OS: removing process 1" << endl;
				cout << fixed << tempTimer.timerEndValue.front() << " - OS: removing process 1" << endl;
				tempTimer.timerName.pop();
				tempTimer.timerStartValue.pop();
				tempTimer.timerEndValue.pop();
			}

			//processor(run)
			else if(tempCutStr == "P{r")
			{
				fout << fixed << tempTimer.timerStartValue.front() << " - Process 1: start processing action" << endl;
				fout << fixed << tempTimer.timerEndValue.front() << " - Process 1: end processing action" << endl;
				cout << fixed << tempTimer.timerStartValue.front() << " - Process 1: start processing action" << endl;
				cout << fixed << tempTimer.timerEndValue.front() << " - Process 1: end processing action" << endl;
				tempTimer.timerName.pop();
				tempTimer.timerStartValue.pop();
				tempTimer.timerEndValue.pop();
			}
		
			//memory(allocate)
			else if(tempCutStr == "M{a")
			{
			
				if(tempMemory.count > tempMemory.total)
				{
					tempMemory.count = 0;
				}
				
				fout << fixed << tempTimer.timerStartValue.front() << " - Process 1: allocating memory" << endl;
				fout << fixed << tempTimer.timerEndValue.front() << " - Process 1: memory allocated at " << "0x" << setfill('0') << setw(8) << hex << tempMemory.count << endl;
				cout << fixed << tempTimer.timerStartValue.front() << " - Process 1: allocating memory" << endl;
				cout << fixed << tempTimer.timerEndValue.front() << " - Process 1: memory allocated at " << "0x" << setfill('0') << setw(8) << hex << tempMemory.count << endl;
				tempTimer.timerName.pop();
				tempTimer.timerStartValue.pop();
				tempTimer.timerEndValue.pop();
				tempMemory.count += tempMemory.blockSize;
			}

			//memory(block)
			else if(tempCutStr == "M{b")
			{
				fout << fixed << tempTimer.timerStartValue.front() << " - Process 1: start memory blocking" << endl;
				fout << fixed << tempTimer.timerEndValue.front() << " - Process 1: end memory blocking" << endl;
				cout << fixed << tempTimer.timerStartValue.front() << " - Process 1: start memory blocking" << endl;
				cout << fixed << tempTimer.timerEndValue.front() << " - Process 1: end memory blocking" << endl;
				tempTimer.timerName.pop();
				tempTimer.timerStartValue.pop();
				tempTimer.timerEndValue.pop();
			}
		
		
			//output(monitor)
			else if(tempCutStr == "O{m")
			{
				fout << fixed << tempTimer.timerStartValue.front() << " - Process 1: start monitor output" << endl;
				fout << fixed << tempTimer.timerEndValue.front() << " - Process 1: end monitor output" << endl;
				cout << fixed << tempTimer.timerStartValue.front() << " - Process 1: start monitor output" << endl;
				cout << fixed << tempTimer.timerEndValue.front() << " - Process 1: end monitor output" << endl;
				tempTimer.timerName.pop();
				tempTimer.timerStartValue.pop();
				tempTimer.timerEndValue.pop();
			}
		
			//output(projector)
			else if(tempCutStr == "O{p")
			{
				fout << fixed << tempTimer.timerStartValue.front() << " - Process 1: start projector output on PROJ " << (tempConfig.countProjOut % tempConfig.numProj) << endl;
				fout << fixed << tempTimer.timerEndValue.front() << " - Process 1: end projector output on PROJ " << (tempConfig.countProjOut % tempConfig.numProj) << endl;
				cout << fixed << tempTimer.timerStartValue.front() << " - Process 1: start projector output on PROJ " << (tempConfig.countProjOut % tempConfig.numProj) << endl;
				cout << fixed << tempTimer.timerEndValue.front() << " - Process 1: end projector output on PROJ " << (tempConfig.countProjOut % tempConfig.numProj) << endl;
				tempTimer.timerName.pop();
				tempTimer.timerStartValue.pop();
				tempTimer.timerEndValue.pop();
				tempConfig.countProjOut++;
			}
		
			//output(hard drive)
			else if(tempCutStr == "O{h")
			{
				fout << fixed << tempTimer.timerStartValue.front() << " - Process 1: start hard drive output on HDD " << (tempConfig.countHDDOut % tempConfig.numHDD) << endl;;
				fout << fixed << tempTimer.timerEndValue.front() << " - Process 1: end hard drive output on HDD " << (tempConfig.countHDDOut % tempConfig.numHDD) << endl;
				cout << fixed << tempTimer.timerStartValue.front() << " - Process 1: start hard drive output on HDD " << (tempConfig.countHDDOut % tempConfig.numHDD) << endl;;
				cout << fixed << tempTimer.timerEndValue.front() << " - Process 1: end hard drive output on HDD " << (tempConfig.countHDDOut % tempConfig.numHDD) << endl;
				tempTimer.timerName.pop();
				tempTimer.timerStartValue.pop();
				tempTimer.timerEndValue.pop();
				tempConfig.countHDDOut++;
			}
		
			//input(keyboard)
			else if(tempCutStr == "I{k")
			{
				cout << fixed << tempTimer.timerStartValue.front() << " - Process 1: start keyboard input" << endl;
				cout << fixed << tempTimer.timerEndValue.front() << " - Process 1: end keyboard input" << endl;
				tempTimer.timerName.pop();
				tempTimer.timerStartValue.pop();
				tempTimer.timerEndValue.pop();
			}
		
			//input(hard drive)
			else if(tempCutStr == "I{h")
			{
				fout << fixed << tempTimer.timerStartValue.front() << " - Process 1: start hard drive input on HDD " << (tempConfig.countHDDIn % tempConfig.numHDD) << endl;;
				fout << fixed << tempTimer.timerEndValue.front() << " - Process 1: end hard drive input on HDD " << (tempConfig.countHDDIn % tempConfig.numHDD) << endl;
				cout << fixed << tempTimer.timerStartValue.front() << " - Process 1: start hard drive input on HDD " << (tempConfig.countHDDIn % tempConfig.numHDD) << endl;;
				cout << fixed << tempTimer.timerEndValue.front() << " - Process 1: end hard drive input on HDD " << (tempConfig.countHDDIn % tempConfig.numHDD) << endl;
				tempTimer.timerName.pop();
				tempTimer.timerStartValue.pop();
				tempTimer.timerEndValue.pop();
				tempConfig.countHDDIn++;
			}
		
			//input(scanner)
			else if(tempCutStr == "I{s")
			{
				fout << fixed << tempTimer.timerStartValue.front() << " - Process 1: start scanner input" << endl;
				fout << fixed << tempTimer.timerEndValue.front() << " - Process 1: end scanner input" << endl;
				cout << fixed << tempTimer.timerStartValue.front() << " - Process 1: start scanner input" << endl;
				cout << fixed << tempTimer.timerEndValue.front() << " - Process 1: end scanner input" << endl;
				tempTimer.timerName.pop();
				tempTimer.timerStartValue.pop();
				tempTimer.timerEndValue.pop();
			}
		}
		
		//close file
		fout.close();
	}
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
void timerProcess()
{
	//create thread, mutex and start process control block
	pthread_t thread1;
	pthread_mutex_t mtx;
	tempPCB.processState = PCBstart;
	
	while(!tempInput.processValue.empty() && !tempInput.processName.empty())
	{
		string tempStr = tempInput.processName.front();
		string tempCutStr = tempStr.substr(0, 3);
		tempPCB.processState = PCBready;
		tempTimer.timerName.push(tempStr);
		
		//if input/output, run thread using threadProcess() and locked using mutex
		if(tempCutStr == "O{m" || tempCutStr == "O{p" || tempCutStr == "O{h" || tempCutStr == "I{k" || tempCutStr == "I{h" || tempCutStr == "I{s")
		{
			tempPCB.processState = PCBrunning;
			pthread_mutex_lock(&mtx);
			pthread_create(&thread1, NULL, &threadProcess, NULL);
			pthread_join(thread1, NULL);
			pthread_mutex_unlock(&mtx);
			tempPCB.processState = PCBwaiting;
			tempPCB.processState = PCBexit;
			tempInput.processName.pop();
		}
		
		//else, call cycleProcess()
		else
		{
			tempPCB.processState = PCBrunning;
			cycleProcess();
			tempPCB.processState = PCBexit;
			tempInput.processName.pop();
		}
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
	//process time start
	startTime = (clock()-timerStart)/(double)(CLOCKS_PER_SEC);
	tempTimer.timerStartValue.push(startTime);
	
	//simulate delay
 	delay(tempInput.processValue.front());
	
	//process time end
	endTime = (clock()-timerStart)/(double)(CLOCKS_PER_SEC);	
  	tempTimer.timerEndValue.push(endTime);
  	tempInput.processValue.pop();
  	
  	//exit the thread	
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
 	delay(tempInput.processValue.front());
	
	//process time end
	endTime = (clock()-timerStart)/(double)(CLOCKS_PER_SEC);	
  	tempTimer.timerEndValue.push(endTime);
  	tempInput.processValue.pop();
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
to-do:
	-timer: check every 100ms to see if timer is done
**/
