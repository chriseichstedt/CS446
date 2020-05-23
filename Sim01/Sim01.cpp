/**
@file  Sim01.cpp
@author  Christopher Eichstedt
@version  1.0
@brief  the program will take in a command line argument file name, then call a readIn function.
	the function would then read in the content of the configuration file and store it into a struct.
	then call a read in function for the input file and store its order and values into a queue.
	finally, call an output function and make the necessary calculations from the config and input files.
	storing them into a file.
**/
//------------------------------------------------------

//-------------------------
//included libraries
//-------------------------
#include <iostream>
#include <fstream>
#include <queue>
#include <string>
#include <sstream>
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
	string logFile; //log file for output
	string inputFile; //input file for meta data
};


struct inputData
{
	queue<string> processName;
	queue<int> processValue;
};

//-------------------------
//global variables
//-------------------------
configData tempConfig;
inputData tempInput;
string inputFileName;

//-------------------------
//function declarations
//-------------------------
void readConfig(char*) throw(runtime_error);
void readInput(string) throw(runtime_error);
void output(string);

//------------------------------------------------------

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
			for(int i = 0; i < 4; i++)
			{
				tempQ.pop();
			}
			
			stringstream tempPJCT(tempQ.front());
			tempPJCT >> val;
			tempConfig.pjct = val;
		}
		
		if (tempS == "Log")
		{
			for(int i = 0; i < 6; i++)
			{
				tempQ.pop();
			}
			
			tempConfig.logFile = tempQ.front();
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
@post  stores meta-data command into a queue and stores the calculated cycle time into another
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

		//processor{run}
		if(tempCutStr == "P{r")
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
		
		//system(begin)
		
		//system(finish)
		
		//application(begin)
		
		//application(finish)
		
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
@pre  none
@post  creates a file with the information taken from the user's config and meta-data information 
**/
void output(string filename)
{
	//open file
	ofstream fout;
	fout.open(filename.c_str());

	//write in config data
	fout << "Configuration File Data" << endl;
	fout << "Monitor = " << tempConfig.mdt << " ms/cycle" << endl;
	fout << "Processor = " << tempConfig.pct << " ms/cycle" << endl;
	fout << "Scanner = " << tempConfig.sct << " ms/cycle" << endl;
	fout << "Hard Drive = " << tempConfig.hdct << " ms/cycle" << endl;
	fout << "Keyboard = " << tempConfig.kct << " ms/cycle" << endl;
	fout << "Memory = " << tempConfig.mct << " ms/cycle" << endl;
	fout << "Projector = " << tempConfig.pjct << " ms/cycle" << endl;
	fout << "Logged to: monitor and " << tempConfig.logFile << endl;
	
	//write in meta-data metrics
	int count;
	string originalS, cutS;
	
	fout << endl << "Meta-Data Metrics" << endl;
	
	while(!tempInput.processName.empty() && !tempInput.processValue.empty())
	{
		originalS = tempInput.processName.front();
		count = originalS.find(";");
		cutS = originalS.substr(0,count-1);
		
		fout << cutS << " - " << tempInput.processValue.front() << " ms" << endl;
		tempInput.processName.pop();
		tempInput.processValue.pop();
	}
	
	//close file
	fout.close();
}
