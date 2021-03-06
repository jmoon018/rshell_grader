/* HIGH LEVEL DESCRIPTION OF PROGRAM

	This program will compare the output of commands, given
	in an INPUT file, of bash and rshell, and display which commands
	failed and passed

	1. Create directory for the user/bash logs. Will be used more extensively later
	2. Loop through each line of the INPUT file
		3. Create an rshell instance, and using script -c along with
			redirection, input the command. Record output to log
		3b. Sleep for .25 seconds, or else the the while loop will
			iterate too quickly and the program wont work
		4. Same as 3, but for the bash instance
		5. call/store compareOutput() to see if the two log files are the same
	6. Cleanup directories/open files 
	7. Print the results

	** TODO
	1. Testing. Needs more testing for edge cases, etc
	2. (basic) cheating detection -- for echo statements, etc
	3. Check to see if the environment (ls -lra, pwd) is the same
		after each command. This will help test cd, mkdir, etc
	4. Better formatting, more specific feedback
	5. ??? 

	- Jamal Moon
*/

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <algorithm>
#include <unistd.h>

using namespace std;

// The path to your rshell program. This is important!
string PATH_TO_RSHELL = "/rshell"; 

// Two directories will be made. They will each be the testing environments
//		for the user/bash shell versions. The purpose of this is to test
//		that cd, pwd, ls, etc all work properly
//	They will also contain the log files.
//	The directories may be automatically deleted afterwards, if AUTO_DELETE=true
const string USER_FOLDER	= "user_folder_tmp";
const string BASH_FOLDER	= "bash_folder_tmp";
const bool AUTO_DELETE	= true; // Delete the files after you're done?

// The name of the log files
const string USER_OUT_FILE = "user_out.log";
const string BASH_OUT_FILE = "bash_out.log";

const string bashpath = (BASH_FOLDER + "/" + BASH_OUT_FILE);
const string userpath = (USER_FOLDER + "/" + USER_OUT_FILE);

bool IS_READY = false;

// Input file
// Has list of commands-to-be-tested delimited by a line
string INPUT_FILE = "../tests/input";


// Just like system(xxx.c_str()), but faster to type. 
int cmd(string cmdstr) {
	int x = system(cmdstr.c_str());
	return x;
}


// A simple struct that will be used in a vector to record
//		the results of the tests
struct test_case {
	string command;
	bool pass;
};


// fixString
// Param: string (one line of text in user/bash.log) by address
// Purpose: Removes random characters (^M at end of lines)
//		and deletes the "Script started on...Script done on...*date*" 
//		lines because they are unnecessary for comparisons
void fixString(string &line)
{
	if(line.find("Script started on ") != string::npos || line.find("Script done on ") != string::npos)
	{
		line = "";
	}
	line.erase(std::remove(line.begin(), line.end(), '\x0D'), line.end());

	//line = "\"" + line + "\"";
}


// fixUserOutput
// Param: string cmd -- the comamnd currently being tested
//		  char* fileName -- the name of the user output file
// Purpose: Some of the rshell programs may print *name*@hammer.cs.ucr: *command*
//		but the bash terminal does not display that, and thus the comparisons
//		between the two output files are being thrown off
void fixUserOutput(const string& cmd) {
	ifstream user(userpath.c_str()); 
	vector<string> lines; // contains the list of each line in the output file
	string firstPart = ""; // this will contain the string that is outputted before
					  // each command.. EG: if it's jmoon018$ echo hi
					  // then firstPart = jmoon018$ 
	
	string curLine;
	while (getline(user, curLine)) {
		// Find the command
		cout << "LINE: " << curLine << endl << endl;
		size_t location = curLine.find(" ");
		if(location >= curLine.size()) {
			continue;
		}
		if(firstPart == "") {
			firstPart = curLine.substr(0, location);
			cout << "FIRST PART: " << firstPart << endl << endl;
			string theRet = curLine.substr(location, 10000);
			//cout << "RET: " << curLine << endl;
			lines.push_back(theRet);
		}
		else {
			if(curLine.compare(firstPart) == 0)
				curLine = "";
			//cout << "RET2: " << curLine << "." << endl;
			//cout << "RET3: " << firstPart << "." << endl;
			lines.push_back(curLine);
			//lines.push_back("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
			//cout << "Added " << cmd << " to lines.." << lines.at(lines.size()-1) << "." << endl;
		}
	}

	user.close();

	// Now rewrite to file
	ofstream userout(userpath.c_str(), ios::trunc);
	//cout << "Fixed output.. printing it out.." << lines.size() << endl;
	//cout << "line0: " << lines.at(0) << endl;
	for(unsigned i = 0; i < lines.size(); i++) {
		userout << lines.at(i) << endl;
		cout << lines.at(i) << endl;
	}
	userout.close(); // close
}

// compareOutput
// PARAM: none
// Purpose: compares the user.log and bash.log files
// Returns true if they are the same, false otherwise
//
bool compareOutput() {
	// Create a file stream for the user.log and bash.log
	//		so we cane read and compare
	ifstream user(userpath.c_str());
	ifstream bash(bashpath.c_str());

	// Turns false when the input is not the same
	// or when a file has reached the end but the other hasn't
	// I made this variable instead of just returning in the loop
	//		so that I can close the files before returning
	bool retValue = true;

	string userline; // a single line from the user log
	string bashline; // a single line from the bash log

	// Fix the useroutput 
	// Loop thru userlog.
	// Bash log could be larger than userlog, so check afterwards
	while(getline(user, userline)) {
		getline(bash, bashline);
		
	//	cout << "Userline: " << userline << endl;
	//	cout << "Bashline: " << bashline << endl;

		// Fix the strings to remove the pesky ^M character + other things
		fixString(bashline);
		fixString(userline);
		
//		cout << "bl: " << bashline << endl;
//		cout << "ul: " << userline << endl;

//		cout << "COMPARING OUTPUT\nBASH: " << bashline << "\nUSER: " << userline << endl;
		// Compare
		if(userline != bashline)
			retValue = false;	
	}

	// Check if the bash log file has more space in the file. If so, 
	//	the user and bash files are not equivalent.
	if(retValue)
	{
		//	cout << "Testing if " << BASH_LOG_FILE << " is also complete" << endl;
		retValue = !getline(bash, bashline);  
	}

	// Close files and return
	user.close();
	bash.close(); 
	return retValue;
}

void cleanFiles() {
	vector<string> userLines;
	vector<string> bashLines;
	
	ifstream user(userpath.c_str());
	ifstream bash(bashpath.c_str());
	string curLine;
	while(getline(user, curLine)) { userLines.push_back(curLine); }
	while(getline(bash, curLine)) { bashLines.push_back(curLine); }

	user.close();
	bash.close();

	ofstream uo(userpath.c_str());
	ofstream bo(bashpath.c_str());
	for(unsigned i = 1; i < (userLines.size()); i++) {
		uo << userLines.at(i) << endl;
	}

	for(unsigned i = 1; i < (bashLines.size()); i++) {
		bo << bashLines.at(i) << endl;
	}
	uo.close();
	bo.close();
}

//	clearOutputFiles
//	Param: none
//	Purpose: clears the bash/user log files from any text
//	Returns nothing
//	CURRENTLY NOT USED
void clearOutputFiles()
{
	ofstream user(userpath.c_str());
	ofstream bash(bashpath.c_str());
	user.close();
	bash.close();
}


/* createLogs
	Param: none
	Purpose: to create the log files with proper parameters
	Return: nothing
	CURRENTLY NOT BEING USED 
*/

void createLogs()
{
	ofstream user (userpath.c_str());
	ofstream bash (userpath.c_str());

	string x = "chmod a+x " + userpath;
	string y = "chmod a+x " + bashpath;
	cmd(x);
	cmd(y);

	user.close();
	bash.close();
}

/* SETREADY
	Param: 1 or 0 
	Purpose: creates a file with value 1 or 0 which determines
		whether or the program should continue
	return nothing
*/

void setReady(string ready) {
	ofstream file ("readyFile.txt", ofstream::trunc);
	file << ready;
	file.close();
}

bool isReady() {
	ifstream file("readyFile.txt");
	char c;
	file.get(c);
	file.close();
	cout << "C IS !! " << c;
	if(c == '1') {
		return true;
	}
	else {
		return false;
	}
}


// This method will find the current working directory and 
// then determine if we need to go in the /bin folder or not
// in order to get the executable
void getProperPath() {
	char buf[1024];
	if(getcwd(buf, sizeof(buf)) == NULL) {
		cout << "ERROR GETTING CURRENT WORKING DIRECTORY" << endl;
	}

	// Check if we are in 'bin'
	string path = buf;
	string lastThree = path.substr(path.size()-3, 3);
	if(lastThree != "bin") {
		//cout << "CHANGING THE PATH TO BIN" << endl;
		cout << lastThree << "!!!" << endl;
		PATH_TO_RSHELL = "bin/" + PATH_TO_RSHELL;
		INPUT_FILE = "tests/input";
	}
	else {
//		cout << "OUR CWD IS CHILL" << endl;
		PATH_TO_RSHELL = "./" + PATH_TO_RSHELL;
	}

}

int main(int argc, char** argv)
{
	if(argc == 1) { 
		PATH_TO_RSHELL = "rshell";
	}
	else {
		PATH_TO_RSHELL = argv[1];
	}
	// Make these directories for separate testing environments
	cmd("mkdir " + USER_FOLDER);
	//cmd("touch " + USER_FOLDER + "/" + USER_OUT_FILE); 
	cmd("mkdir " + BASH_FOLDER);
	//cmd("touch " + BASH_FOLDER + "/" + BASH_OUT_FILE); 


	// Test list
	// Each command from the input file will be added to this vector
	//		and it will provide information about whether it passed/failed
	vector<test_case> tests;

	getProperPath();

	// ifstream of the INPUT_FILE.. this contains the list of commands to test
	ifstream infile(INPUT_FILE.c_str());
	//cout << "IFILE: " << INPUT_FILE.c_str() << endl;

	// Go through each command in the input file
	string line; // contains the command that is currently being tested
	while(getline(infile, line)) {
		// Make sure they are clean before doing anything.	
		//clearOutputFiles();
		//createLogs();

		// Make a file for redirection
		ofstream ofile ("commandtext.txt");
		ofile << line << endl;
		ofile << "exit" << endl;
		ofile.close();

		// test the user's shell
		string theCommand = "script -q -c " + PATH_TO_RSHELL + " < commandtext.txt "
			+ userpath;
		cmd(theCommand);
		cmd("exit");

		// Add a delay because if this while loop runs 
		// too quickly, the program won't run properly
		// a small sleep time like this is sufficient, should work
		// for any machine
		cmd("sleep .005");

		// Test the actual bash
		string cmd2 = "script -q -c \"" + line + "\" " + bashpath;
		// Run the command and exit
		cmd(cmd2 + "; exit;");
		//cout << "exit" << endl;


		// Create an instance of a test case
		// Add it to the tests vector
		test_case tc;
		//cleanFiles();
		//fixUserOutput("");
		tc.pass = compareOutput();
		tc.command = line; 
		tests.push_back(tc);

		cmd("rm commandtext.txt");
		cmd("rm -r " + userpath);
		cmd("rm -r " + bashpath);
	//	cout << "GOING BACK" << endl;
	}
		

	// Remove the directories
	// DO NOT screw this up or else everything might just get deleted...
	if(AUTO_DELETE) {
		cmd("rm -r " +  USER_FOLDER);
		cmd("rm -r " + BASH_FOLDER);
	}

	// close the inputfile
	infile.close();

	// Output test cases
	cout << "Printing test results." << endl;
	unsigned total=0, passed=0;
	for(total = 0; total < tests.size(); total++)
	{
		cout << "Test case " << total << ":" << endl;
		cout << "Command: " << tests.at(total).command << endl;
		cout << "Pass: " << tests.at(total).pass << endl;
		cout << endl;

		if(tests.at(total).pass)
			passed++;
	}
	cout << "Final Results: " << passed << "/" << total << " passed." << endl;
	return 0;
}



