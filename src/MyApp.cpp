#include "MyApp.h"
#include <string>
#include <map>
#include <algorithm>
#include <bitset>
#include <vector>
#include <cstdlib>
#include <stdio.h>
#include <stdlib.h>
#include <array>
#include <fstream>
#include <sstream>

//Initial window dimensions
#define WINDOW_WIDTH  1200
#define WINDOW_HEIGHT 600

//The values in the code table and the memory table are stored in the following arrays
std::string labels[5000], instructions[5000], comments[5000], data[4096];

//The Address symbol table
std::map<std::string, int> label_to_address;

//The registers in the Basic computer
std::vector<uint16_t> IR, AC, DR, PC, AR, MAR, TR;
std::vector<bool> I, E, R, IEN, FGI, FGO;
std::vector<uint8_t> INPR, OUTR;

//A vector that stores the data that is changed in the memory after each execution
std::vector<std::pair<int, std::string>> DATA_CHANGE;

//Check whether the given string has a letter other than 0-9 or A-Z
bool has_non_alphanumeric(std::string s) {
	return std::count_if(s.begin(), s.end(), [](char c){return !(('0' <= c && c <= '9') || ('A' <= c && c <= 'Z'));}) > 0;
}

//Check whether the given string has a letter other than 0-9 or A-Z or space or minus(dash)
bool has_non_alphanumeric_or_space(std::string s) {
	return std::count_if(s.begin(), s.end(), [](char c){return !(('0' <= c && c <= '9') || ('A' <= c && c <= 'Z') || c == ' ' || c == '-');}) > 0;
}

//Check whether the given string has a letter other than 0 or 1
bool has_non_binary(std::string s) {
	return std::count_if(s.begin(), s.end(), [](char c){return !(c == '0' || c == '1');}) > 0;
}

//Check whether the given string has a letter other than 0-9 or A-F
bool check_bad_HEX(std::string s) {
	return std::count_if(s.begin(), s.end(), [](char c){return !(('0' <= c && c <= '9') || ('A' <= c && c <= 'F'));}) > 0;
}

//Check whether the given string has a letter other than 0-9 or a minus(dash) at the beginning
bool check_bad_DEC(std::string s) {
	bool x = std::count_if(s.begin(), s.end(), [](char c){return !(('0' <= c && c <= '9') || c == '-');}) > 0;
	bool y = std::count_if(s.begin() + 1, s.end(), [](char c){return !(('0' <= c && c <= '9'));}) > 0;
	return x & y;
}


//Execute a memory register reference command
void do_MRI(std::string instruction, std::string &error_text, int i, int lc) {
	//Each MRI must be between 5 and 9 characters long (including spaces)
	if (9 < instructions[i].size() || instructions[i].size() < 5) {
		if (error_text.empty())
			error_text = "Line " + std::to_string(i) + ": Invalid instruction.";
		return;
	}
	//Each MRI has a three letter instruction, a space, a 1-3 letter symbolic address, and then maybe a space and I
	//Therefor the second part which starts from index=4 is the symbolic address
	std::string second_part = "";
	int index = 4;
	while (index < instructions[i].size() && instructions[i][index] != ' ')
		second_part += instructions[i][index++];
	//Assure that the symbolic address has a length of 1-3 characters
	if (3 < second_part.size() || second_part.size() < 1) {
		if (error_text.empty())
			error_text = "Line " + std::to_string(i) + ": Invalid instruction.";
		return;
	}
	//Assure that the first character of the symbolic address is not a digit
	if (isdigit(second_part[0])) {
		if (error_text.empty())
			error_text = "Line " + std::to_string(i) + ": The second part of an MRI must be a symbolic address.";
		return;
	}
	//Check whether the symbolic address exists in the symbolic address table
	if (label_to_address.find(second_part) == label_to_address.end()) {
		if (error_text.empty())
			error_text = "Line " + std::to_string(i) + ": Label not defined.";
		return;
	}
	//Save the first character of the data in memory based on whether the addressing is direct or indirect
	//Based on the size, check whether an I (indirect addressing) exists as it should
	if (instructions[i].size() == 4 + second_part.size() + 2) {
		if (instructions[i][instructions[i].size() - 2] != ' ' || instructions[i].back() != 'I') {
			if (error_text.empty())
				error_text = "Line " + std::to_string(i) + ": Invalid instruction.";
			return;
		}
		else
			data[lc] = "1";
	}
	//Save the first character as 0 if the addressing is direct
	else if (instructions[i].size() == 4 + second_part.size())
		data[lc] = "0";
	else {
		if (error_text.empty())
			error_text = "Line " + std::to_string(i) + ": Invalid instruction.";
		return;
	}
	//Save the next three characters (OP-Code) based on the operation
	if (instruction == "AND")
		data[lc] += "000";
	else if (instruction == "ADD")
		data[lc] += "001";
	else if (instruction == "LDA")
		data[lc] += "010";
	else if (instruction == "STA")
		data[lc] += "011";
	else if (instruction == "BUN")
		data[lc] += "100";
	else if (instruction == "BSA")
		data[lc] += "101";
	else if (instruction == "ISZ")
		data[lc] += "110";
	//Save the address obtained from the symbolic address table in the last 12 characters
	data[lc] += std::bitset<12>(label_to_address[second_part]).to_string();
	return;
}

//Update the values of the register table in the GUI
void refreshVariablesCommand(std::string &command) {
	command = "document.getElementById('IR').innerHTML = '" + std::bitset<16>(IR.back()).to_string() + "';";
	command += "document.getElementById('I').innerHTML = '" + std::bitset<1>(I.back()).to_string() + "';";
	command += "document.getElementById('AC').innerHTML = '" + std::bitset<16>(AC.back()).to_string() + "';";
	command += "document.getElementById('DR').innerHTML = '" + std::bitset<16>(DR.back()).to_string() + "';";
	command += "document.getElementById('PC').innerHTML = '" + std::bitset<12>(PC.back()).to_string() + "';";
	command += "document.getElementById('AR').innerHTML = '" + std::bitset<12>(AR.back()).to_string() + "';";
	command += "document.getElementById('MAR').innerHTML = '" + std::bitset<16>(MAR.back()).to_string() + "';";
	command += "document.getElementById('E').innerHTML = '" + std::bitset<1>(E.back()).to_string() + "';";
	command += "document.getElementById('TR').innerHTML = '" + std::bitset<16>(TR.back()).to_string() + "';";
	command += "document.getElementById('INPR').value = '" + std::bitset<8>(INPR.back()).to_string() + "';";
	command += "document.getElementById('OUTR').innerHTML = '" + std::bitset<8>(OUTR.back()).to_string() + "';";
	command += "document.getElementById('R').innerHTML = '" + std::bitset<1>(R.back()).to_string() + "';";
	command += "document.getElementById('IEN').innerHTML = '" + std::bitset<1>(IEN.back()).to_string() + "';";
	command += "document.getElementById('FGI').value = '" + std::bitset<1>(FGI.back()).to_string() + "';";
	command += "document.getElementById('FGO').value = '" + std::bitset<1>(FGO.back()).to_string() + "';";

	command += "document.getElementById('preIR').innerHTML = '" + std::bitset<16>(IR[IR.size() - 2]).to_string() + "';";
	command += "document.getElementById('preI').innerHTML = '" + std::bitset<1>(I[I.size() - 2]).to_string() + "';";
	command += "document.getElementById('preAC').innerHTML = '" + std::bitset<16>(AC[AC.size() - 2]).to_string() + "';";
	command += "document.getElementById('preDR').innerHTML = '" + std::bitset<16>(DR[DR.size() - 2]).to_string() + "';";
	command += "document.getElementById('prePC').innerHTML = '" + std::bitset<12>(PC[PC.size() - 2]).to_string() + "';";
	command += "document.getElementById('preAR').innerHTML = '" + std::bitset<12>(AR[AR.size() - 2]).to_string() + "';";
	command += "document.getElementById('preMAR').innerHTML = '" + std::bitset<16>(MAR[MAR.size() - 2]).to_string() + "';";
	command += "document.getElementById('preE').innerHTML = '" + std::bitset<1>(E[E.size() - 2]).to_string() + "';";
	command += "document.getElementById('preTR').innerHTML = '" + std::bitset<16>(TR[TR.size() - 2]).to_string() + "';";
	command += "document.getElementById('preINPR').innerHTML = '" + std::bitset<8>(INPR[INPR.size() - 2]).to_string() + "';";
	command += "document.getElementById('preOUTR').innerHTML = '" + std::bitset<8>(OUTR[OUTR.size() - 2]).to_string() + "';";
	command += "document.getElementById('preR').innerHTML = '" + std::bitset<1>(R[R.size() - 2]).to_string() + "';";
	command += "document.getElementById('preIEN').innerHTML = '" + std::bitset<1>(IEN[IEN.size() - 2]).to_string() + "';";
	command += "document.getElementById('preFGI').innerHTML = '" + std::bitset<1>(FGI[FGI.size() - 2]).to_string() + "';";
	command += "document.getElementById('preFGO').innerHTML = '" + std::bitset<1>(FGO[FGO.size() - 2]).to_string() + "';";
	command += "makeHEXs();";
	for (int i = 0; i < 4096; i++)
			command += "document.getElementsByClassName('rowData')[" + std::to_string(i) + "].innerHTML = '" + data[i] + "';";
}

//The assembler function
JSValueRef assemble(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception) {
	JSStringRef script;
	std::string command = "", result;
	//Clear the symbolic address table
	label_to_address.clear();
	//Clear the memory
	for (int i = 0; i < 4096; i++) {
		data[i] = "0000000000000000";
		command += "document.getElementsByClassName('memoryRow')[" + std::to_string(i) + "].style.backgroundColor = 'initial';";
	}
	script = JSStringCreateWithUTF8CString(command.c_str());
	JSEvaluateScript(ctx, script, 0, 0, 0, 0);

	//Fetch each line of the code from the GUI
	for (int i = 0; i < 5000; i++) {
		command = "document.getElementsByClassName('rowLabelInput')[" + std::to_string(i) + "].value.toString()";
		script = JSStringCreateWithUTF8CString(command.c_str());
		result = std::string(String(JSString(JSValueToStringCopy(ctx, JSEvaluateScript(ctx, script, 0, 0, 0, 0), 0))).utf8().data());
		std::transform(result.begin(), result.end(), result.begin(), ::toupper);
		labels[i] = result;
		command = "document.getElementsByClassName('rowInstructionInput')[" + std::to_string(i) + "].value.toString()";
		script = JSStringCreateWithUTF8CString(command.c_str());
		result = std::string(String(JSString(JSValueToStringCopy(ctx, JSEvaluateScript(ctx, script, 0, 0, 0, 0), 0))).utf8().data());
		std::transform(result.begin(), result.end(), result.begin(), ::toupper);
		instructions[i] = result;
		command = "document.getElementsByClassName('rowCommentInput')[" + std::to_string(i) + "].value.toString()";
		script = JSStringCreateWithUTF8CString(command.c_str());
		result = std::string(String(JSString(JSValueToStringCopy(ctx, JSEvaluateScript(ctx, script, 0, 0, 0, 0), 0))).utf8().data());
		comments[i] = result;
	}

	std::string error_text = "";

	//Run the first pass of the assembler + Error detection
	int lc = -1;
	for (int i = 0; i < 5000; i++) {
		lc++;
		if (!comments[i].empty()) {
			if (comments[i][0] != '/') {
				if (error_text.empty())
					error_text = "Line " + std::to_string(i) + ": Comments must start with \\'/\\'.";
				break;
			}
		}
		if (!labels[i].empty()) {
			if (labels[i].back() != ',') {
				if (error_text.empty())
					error_text = "Line " + std::to_string(i) + ": Labels must end with \\',\\'.";
				break;
			}
			if (instructions[i] == "END" || (instructions[i].size() > 3 && instructions[i].substr(0, 3) == "ORG")) {
				if (error_text.empty())
					error_text = "Line " + std::to_string(i) + ": ORG and END instructions must not have labels.";
				break;
			}
			else if (labels[i].size() > 4 || has_non_alphanumeric(labels[i].substr(0, labels[i].size() - 1))) {
				if (error_text.empty())
					error_text = "Line " + std::to_string(i) + ": Invalid label.";
				break;
			}
			else if (label_to_address.find(labels[i].substr(0, labels[i].size() - 1)) != label_to_address.end()) {
				if (error_text.empty())
					error_text = "Line " + std::to_string(i) + ": Label redefined.";
				break;
			}
			else
				label_to_address[labels[i].substr(0, labels[i].size() - 1)] = lc;
		}
		else if (instructions[i] == "END")
			break;
		else if (instructions[i].size() > 3 && instructions[i].substr(0, 3) == "ORG") {
			if (instructions[i].size() < 5 || instructions[i][3] != ' ' || has_non_alphanumeric(instructions[i].substr(4))) {
				if (error_text.empty())
					error_text = "Line " + std::to_string(i) + ": Invalid ORG instruction.";
				break;
			}
			else {
				int org_line = std::stoi(instructions[i].substr(4), nullptr, 16);
				lc = org_line - 1;
			}
		}
	}

	//Run the second pass of the assembler + Error detection
	lc = -1;
	for (int i = 0; i < 5000; i++) {
		if (labels[i].empty() && instructions[i].empty())
			continue;
		lc++;
		if (instructions[i].size() < 3 || has_non_alphanumeric_or_space(instructions[i])) {
			if (error_text.empty())
				error_text = "Line " + std::to_string(i) + ": Invalid instruction.";
			break;
		}
		else {
			std::string instruction = instructions[i].substr(0, 3);
			if (instruction == "END")
				break;
			else if (lc >= 4096) {
				if (error_text.empty())
					error_text = "Line " + std::to_string(i) + ": LC exceeded 4095.";
				break;
			}
			else if (instruction == "ORG") {
				if (instructions[i].size() < 5 || instructions[i][3] != ' ' || has_non_alphanumeric(instructions[i].substr(4))) {
					if (error_text.empty())
						error_text = "Line " + std::to_string(i) + ": Invalid ORG instruction";
					break;
				}
				else {
					int org_line = std::stoi(instructions[i].substr(4), nullptr, 16);
					lc = org_line - 1;
				}
			}
			else if (instruction == "HEX") {
				if (instructions[i].size() < 5 || instructions[i][3] != ' ') {
					if (error_text.empty())
						error_text = "Line " + std::to_string(i) + ": Invalid HEX instruction.";
					break;
				}
				else {
					if (check_bad_HEX(instructions[i].substr(4))) {
						if (error_text.empty())
							error_text = "Line " + std::to_string(i) + ": Invalid HEX number.";
						break;
					}
					int hex_value = std::stoi(instructions[i].substr(4), nullptr, 16);
					if (hex_value > 65535) {
						if (error_text.empty())
							error_text = "Line " + std::to_string(i) + ": HEX number out of range.";
						break;
					}
					data[lc] = std::bitset<16>(hex_value).to_string();
				}
			}
			else if (instruction == "DEC") {
				if (instructions[i].size() < 5 || instructions[i][3] != ' ') {
					if (error_text.empty())
						error_text = "Line " + std::to_string(i) + ": Invalid DEC instruction.";
					break;
				}
				else {
					if (check_bad_DEC(instructions[i].substr(4))) {
						if (error_text.empty())
							error_text = "Line " + std::to_string(i) + ": Invalid DEC number.";
						break;
					}
					int dec_value = std::stoi(instructions[i].substr(4));
					if (32767 < dec_value || dec_value < -32768) {
						if (error_text.empty())
							error_text = "Line " + std::to_string(i) + ": DEC number out of range.";
						break;
					}
					data[lc] = std::bitset<16>(dec_value).to_string();
				}
			}
			else if (instruction == "AND" || instruction == "ADD" || instruction == "LDA" || instruction == "STA" || instruction == "BUN" || instruction == "BSA" || instruction == "ISZ") {
				do_MRI(instruction, error_text, i, lc);
				if (!error_text.empty())
					break;
			}
			else if (instructions[i].size() != 3) {
				if (error_text.empty())
					error_text = "Line " + std::to_string(i) + ": A non-MRI instruction must have 3 characters.";
				break;
			}
			else if (instruction == "CLA")
				data[lc] = "0111100000000000";
			else if (instruction == "CLE")
				data[lc] = "0111010000000000";
			else if (instruction == "CMA")
				data[lc] = "0111001000000000";
			else if (instruction == "CME")
				data[lc] = "0111000100000000";
			else if (instruction == "CIR")
				data[lc] = "0111000010000000";
			else if (instruction == "CIL")
				data[lc] = "0111000001000000";
			else if (instruction == "INC")
				data[lc] = "0111000000100000";
			else if (instruction == "SPA")
				data[lc] = "0111000000010000";
			else if (instruction == "SNA")
				data[lc] = "0111000000001000";
			else if (instruction == "SZA")
				data[lc] = "0111000000000100";
			else if (instruction == "SZE")
				data[lc] = "0111000000000010";
			else if (instruction == "HLT")
				data[lc] = "0111000000000001";
			else if (instruction == "INP")
				data[lc] = "1111100000000000";
			else if (instruction == "OUT")
				data[lc] = "1111010000000000";
			else if (instruction == "SKI")
				data[lc] = "11110010000000000";
			else if (instruction == "SKO")
				data[lc] = "1111000100000000";
			else if (instruction == "ION")
				data[lc] = "1111000010000000";
			else if (instruction == "IOF")
				data[lc] = "1111000001000000";
			else if (error_text.empty())
				error_text = "Line " + std::to_string(i) + ": Invalid instruction.";
		}
	}

	//If an error has occurred, display the error on the GUI
	if (!error_text.empty()) {
		command = "document.getElementById('log').innerHTML = '" + error_text + "';document.getElementById('log').style.color = 'rgb(110, 10, 10)';";
		script = JSStringCreateWithUTF8CString(command.c_str());
		JSEvaluateScript(ctx, script, 0, 0, 0, 0);
	}
	//If no has occurred, display a success message on the GUI and initialize the registers
	else {
		command = "document.getElementById('log').innerHTML = 'Program assembled successfully.';document.getElementById('log').style.color = 'rgb(10, 110, 10)';";
		for (int i = 0; i < 4096; i++)
			command += "document.getElementsByClassName('rowData')[" + std::to_string(i) + "].innerHTML = '" + data[i] + "';";
		IR.clear();
		IR.push_back(0);
		I.clear();
		I.push_back(0);
		AC.clear();
		AC.push_back(0);
		DR.clear();
		DR.push_back(0);
		PC.clear();
		PC.push_back(0);
		AR.clear();
		AR.push_back(0);
		MAR.clear();
		MAR.push_back(0);
		E.clear();
		E.push_back(0);
		TR.clear();
		TR.push_back(0);
		INPR.clear();
		INPR.push_back(0);
		OUTR.clear();
		OUTR.push_back(0);
		R.clear();
		R.push_back(0);
		IEN.clear();
		IEN.push_back(0);
		FGI.clear();
		FGI.push_back(0);
		FGO.clear();
		FGO.push_back(0);
		DATA_CHANGE.clear();
		DATA_CHANGE.emplace_back(0, "0000000000000000");
		command += "finished = false;";
		script = JSStringCreateWithUTF8CString(command.c_str());
		JSEvaluateScript(ctx, script, 0, 0, 0, 0);
	}

	JSStringRelease(script);

	return JSValueMakeNull(ctx);
}


//Execute the next instruction
JSValueRef execute_next(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception) {
	JSStringRef script;
	std::string command, result;

	//If the IR register is empty, then the program hasn't been assembled yet
	if (IR.empty()) {
		command = "document.getElementById('log').innerHTML = 'No data has been assembled.';document.getElementById('log').style.color = 'rgb(110, 10, 10)';";
		script = JSStringCreateWithUTF8CString(command.c_str());
		JSEvaluateScript(ctx, script, 0, 0, 0, 0);
		JSStringRelease(script);
		return JSValueMakeNull(ctx);
	}

	//A flag that becomes true when the computer halts
	bool halt = false;

	//The register values of the current execution
	uint16_t ir, ac, dr, pc, ar, mar, tr;
	bool i, e, r, ien, fgi, fgo;
	uint8_t inpr, outr;
	std::pair<int, std::string> data_change;
	
	//Initialize the register values
	ir = IR.back();
	ac = AC.back();
	dr = DR.back();
	pc = PC.back();
	ar = AR.back();
	mar = MAR.back();
	tr = TR.back();
	i = I.back();
	e = E.back();
	r = R.back();
	ien = IEN.back();
	//Get FGI from user input
	command = "FGI.value.toString()";
	script = JSStringCreateWithUTF8CString(command.c_str());
	result = std::string(String(JSString(JSValueToStringCopy(ctx, JSEvaluateScript(ctx, script, 0, 0, 0, 0), 0))).utf8().data());
	if (result.size() != 1 || has_non_binary(result)) {
		command = "document.getElementById('log').innerHTML = 'FGI must be a 1 digit binary number.';document.getElementById('log').style.color = 'rgb(110, 10, 10)';";
		script = JSStringCreateWithUTF8CString(command.c_str());
		JSEvaluateScript(ctx, script, 0, 0, 0, 0);
		JSStringRelease(script);
		return JSValueMakeNull(ctx);
	}
	fgi = std::stoi(result, nullptr, 2);
	//Get FGO from user input
	command = "FGO.value.toString()";
	script = JSStringCreateWithUTF8CString(command.c_str());
	result = std::string(String(JSString(JSValueToStringCopy(ctx, JSEvaluateScript(ctx, script, 0, 0, 0, 0), 0))).utf8().data());
	if (result.size() != 1 || has_non_binary(result)) {
		command = "document.getElementById('log').innerHTML = 'FGO must be a 1 digit binary number.';document.getElementById('log').style.color = 'rgb(110, 10, 10)';";
		script = JSStringCreateWithUTF8CString(command.c_str());
		JSEvaluateScript(ctx, script, 0, 0, 0, 0);
		JSStringRelease(script);
		return JSValueMakeNull(ctx);
	}
	fgo = std::stoi(result, nullptr, 2);
	//Get INPR from user input
	command = "INPR.value.toString()";
	script = JSStringCreateWithUTF8CString(command.c_str());
	result = std::string(String(JSString(JSValueToStringCopy(ctx, JSEvaluateScript(ctx, script, 0, 0, 0, 0), 0))).utf8().data());
	if (result.size() != 8 || has_non_binary(result)) {
		command = "document.getElementById('log').innerHTML = 'INPR must be an 8 digit binary number.';document.getElementById('log').style.color = 'rgb(110, 10, 10)';";
		script = JSStringCreateWithUTF8CString(command.c_str());
		JSEvaluateScript(ctx, script, 0, 0, 0, 0);
		JSStringRelease(script);
		return JSValueMakeNull(ctx);
	}
	inpr = std::stoi(result, nullptr, 2);
	outr = OUTR.back();

	//If the R flag is true, run the interrupt cycle
	if (r) {
		ar = 0;
		tr = pc;
		data[ar] = std::bitset<16>(tr).to_string();
		mar = (std::stoi(data[ar], nullptr, 2));
		pc = 0;
		pc = pc + 1;
		ien = 0;
		r = 0;
	}
	//If the R flag is false, run the instruction cycle
	else {
		//Highlight the current memory line that is being executed in the GUI
		command = "";
		if (PC.size() > 1)
			command += "document.getElementsByClassName('memoryRow')[" + std::to_string(PC[PC.size() - 2]) + "].style.backgroundColor = 'initial';";
		command += "document.getElementsByClassName('memoryRow')[" + std::to_string(PC[PC.size() - 1]) + "].style.backgroundColor = 'rgb(220, 255, 220)';";
		command += "document.getElementsByClassName('memoryRow')[" + std::to_string(PC[PC.size() - 1]) + "].scrollIntoView(false);";
		script = JSStringCreateWithUTF8CString(command.c_str());
		JSEvaluateScript(ctx, script, 0, 0, 0, 0);

		//Fetch and decode
		ar = pc;
		mar = std::stoi(data[ar], nullptr, 2);
	
		ir = mar;
		pc = pc + 1;

		int opcode = ((ir & ((1 << 15) - 1)) >> 12);
		ar = (ir & ((1 << 12) - 1));
		mar = std::stoi(data[ar], nullptr, 2);
		i = (ir >> 15);
		
		//Save the data that might be changed in the current execution along with its address
		data_change = std::make_pair(ar, data[ar]);

		//Execute register-reference instruction (starts with 7) or IO instruction (starts with F)
		if (opcode == 7) {
			switch(ir) {
				case 0x7800: {
					ac = 0;
					break;
				}
				case 0x7400: {
					e = 0;
					break;
				}
				case 0x7200: {
					ac = ~ac;
					break;
				}
				case 0x7100: {
					e = !e;
					break;
				}
				case 0x7080: {
					bool tmp = e;
					e = (ac & 1);
					ac = ((ac >> 1) | (uint16_t(tmp) << 15));
					break;
				}
				case 0x7040: {
					bool tmp = e;
					e = ((ac & (1 << 15)) >> 15);
					ac = ((ac << 1) | uint16_t(e));
					break;
				}
				case 0x7020: {
					ac = ac + 1;
					break;
				}
				case 0x7010: {
					if ((ac & (1 << 15)) == 0)
						pc = pc + 1;
					break;
				}
				case 0x7008: {
					if ((ac & (1 << 15)) != 0)
						pc = pc + 1;
					break;
				}
				case 0x7004: {
					if (ac == 0)
						pc = pc + 1;
					break;
				}
				case 0x7002: {
					if (e == 0)
						pc = pc + 1;
					break;
				}
				case 0x7001: {
					halt = true;
					break;
				}
				case 0xF800: {
					ac = inpr;
					fgi = 0;
					break;
				}
				case 0xF400: {
					outr = (ac & ((1 << 8) - 1));
					fgo = 0;
					break;
				}
				case 0xF200: {
					if (fgi == 1)
						pc = pc + 1;
					break;
				}
				case 0xF100: {
					if (fgo == 1)
						pc = pc + 1;
					break;
				}
				case 0xF080: {
					ien = 1;
					break;
				}
				case 0xF040: {
					ien = 0;
					break;
				}
			}
		}
		//Execute memory-reference instruction
		else {
			if (i) {
				ar = (mar & ((1 << 12) - 1));
				mar = std::stoi(data[ar], nullptr, 2);
			}
			switch (opcode) {
				case 0b000: {
					dr = mar;
					ac = (ac & dr);
					break;
				}
				case 0b001: {
					dr = mar;
					ac = ac + dr;
					e = ((ac >> 15) & (dr >> 15));
					break;
				}
				case 0b010: {
					dr = mar;
					ac = dr;
					break;
				}
				case 0b011: {
					data[ar] = std::bitset<16>(ac).to_string();
					mar = (std::stoi(data[ar], nullptr, 2));
					break;
				}
				case 0b100: {
					pc = ar;
					break;
				}
				case 0b101: {
					data[ar] = std::bitset<16>(pc).to_string();
					ar = ar + 1;
					mar = (std::stoi(data[ar], nullptr, 2));
					pc = ar;
					break;
				}
				case 0b110: {
					dr = mar;
					dr = dr + 1;
					data[ar] = std::bitset<16>(dr).to_string();
					mar = (std::stoi(data[ar], nullptr, 2));
					if (dr == 0)
						pc = pc + 1;
					break;
				}
			}
		}
	}
	//Check the conditions for the R flag
	r = ien & (fgo | fgi);
	//If the computer has halted, then the PC register shouldn't be incremented
	if (halt)
		pc = pc - 1;
	//If the computer hasn't halted or the PC register has changed, store the new register values
	//The PC register might change while the computer has halted when the Execute next button is pressed once more after it has halted
	if (!halt || PC[PC.size() - 2] != PC.back()) {
		IR.push_back(ir);
		AC.push_back(ac);
		DR.push_back(dr);
		PC.push_back(pc);
		AR.push_back(ar);
		MAR.push_back(mar);
		TR.push_back(tr);
		I.push_back(i);
		E.push_back(e);
		R.push_back(r);
		IEN.push_back(ien);
		FGI.push_back(fgi);
		FGO.push_back(fgo);
		INPR.push_back(inpr);
		OUTR.push_back(outr);
		DATA_CHANGE.push_back(data_change);
	}

	//Update the register values in the GUI
	refreshVariablesCommand(command);

	script = JSStringCreateWithUTF8CString(command.c_str());
	JSEvaluateScript(ctx, script, 0, 0, 0, 0);

	//If the computer has halted, display a finish message
	if (halt) {
		command = "log.innerHTML = 'Execution finished.';log.style.color = 'rgb(10, 110, 10)';";
		command += "finished = true;";
	}
	//If not, clear the message log
	else
		command = "log.innerHTML = '';";
	script = JSStringCreateWithUTF8CString(command.c_str());
	JSEvaluateScript(ctx, script, 0, 0, 0, 0);
	
	JSStringRelease(script);

	return JSValueMakeNull(ctx);
}

//Go to the previous state
JSValueRef previous_state(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception) {
	JSStringRef script;
	std::string command, result;

	//If the IR vector (or any other register vector) has less than 3 elements, then a previous state doesn't exist
	if (IR.size() <= 2) {
		command = "document.getElementById('log').innerHTML = 'No previous state exists.';document.getElementById('log').style.color = 'rgb(110, 10, 10)';";
		script = JSStringCreateWithUTF8CString(command.c_str());
		JSEvaluateScript(ctx, script, 0, 0, 0, 0);
		JSStringRelease(script);
		return JSValueMakeNull(ctx);
	}

	//Clear the message log
	command = "document.getElementById('log').innerHTML = '';";
	//Remove the highlight from the current memory line
	command += "document.getElementsByClassName('memoryRow')[" + std::to_string(PC[PC.size() - 2]) + "].style.backgroundColor = 'initial';";

	//Remove the last element from each of the register vectors
	IR.pop_back();
	AC.pop_back();
	DR.pop_back();
	PC.pop_back();
	AR.pop_back();
	MAR.pop_back();
	data[DATA_CHANGE.back().first] = DATA_CHANGE.back().second;
	DATA_CHANGE.pop_back();
	TR.pop_back();
	I.pop_back();
	E.pop_back();
	R.pop_back();
	IEN.pop_back();
	FGI.pop_back();
	FGO.pop_back();
	INPR.pop_back();
	OUTR.pop_back();

	//Add the highlight to the previous memory line
	command += "document.getElementsByClassName('memoryRow')[" + std::to_string(PC[PC.size() - 2]) + "].style.backgroundColor = 'rgb(220, 255, 220)';";
	command += "document.getElementsByClassName('memoryRow')[" + std::to_string(PC[PC.size() - 2]) + "].scrollIntoView(false);";
	script = JSStringCreateWithUTF8CString(command.c_str());
	JSEvaluateScript(ctx, script, 0, 0, 0, 0);
	
	//Change the 'finished' variable in javascript, which indicates whether the program has executed until it has reached a halt (For the Execute all button)
	command = "finished = false;";
	script = JSStringCreateWithUTF8CString(command.c_str());
	JSEvaluateScript(ctx, script, 0, 0, 0, 0);

	refreshVariablesCommand(command);

	script = JSStringCreateWithUTF8CString(command.c_str());
	JSEvaluateScript(ctx, script, 0, 0, 0, 0);
	
	JSStringRelease(script);

	return JSValueMakeNull(ctx);
}

//A function that executes a bash command in the OS and returns the result
std::string exec(const char* cmd) {
	std::string result = "";
    std::array<char, 128> buffer;
    FILE* pPipe = _popen(cmd, "r");
    if (pPipe == NULL)
        return "***Failed***";
    while (fgets(buffer.data(), 128, pPipe) != NULL)
        result += buffer.data();
    int endOfFileVal = feof(pPipe);
    int closeReturnVal = _pclose(pPipe);
    if (!endOfFileVal)
        return "***Failed***";
	//Remove the \n at the end
	result.pop_back();
	 //For windows operating systems
	#if defined(_WIN32) || defined(_WIN64)
	if (result == "Cancel\n") //Remove the other \n at the end
		result.pop_back();
	else //Remove the OK and \n at the beginning
		result.erase(0, 3);
	//For linux operating systems
	#else
	if (result == "")
		return "Cancel";
	#endif
	return result;
}

//The load file function
JSValueRef show_load_file(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception) {
	JSStringRef script;
	std::string command = "", address;

	#if defined(_WIN32) || defined(_WIN64)
		//A Windows CMD command that runs the powershell script for opening a load file dialog box
		address = exec("powershell -WindowStyle hidden -executionpolicy bypass -file openfiledialog-txt.ps1");
	#else
		//A Linux terminal command that runs the script for opening a load file dialog box
		address = exec("zenity --file-selection");
	#endif

	if (address == "***Failed***") {
		command = "log.innerHTML = 'Failed to load file.';log.style.color = 'rgb(110, 10, 10)';";
		script = JSStringCreateWithUTF8CString(command.c_str());
		JSEvaluateScript(ctx, script, 0, 0, 0, 0);
	}
	//If the user doesn't cancel opening a file
	else if (address != "Cancel") {
		std::ifstream code_file(address);
		std::vector<std::vector<std::string>> result;
		std::vector<std::string> current_line;
		std::string tmp, line_label, line_instruction, line_comment;
		//Get each line of the file and store it in tmp
		while(getline(code_file, tmp)) {
			if (tmp.empty())
				continue;
			current_line.clear();
			//Check whether a slash exists, if it does then separate the comment section
			if (tmp.find('/') != std::string::npos) {
				line_comment = tmp.substr(tmp.find('/'));
				tmp.erase(tmp.find('/'), std::string::npos);
			}
			else
				line_comment = "";
			//All parts of the instruction, excluding the comment section, should be capitalized
			std::transform(tmp.begin(), tmp.end(), tmp.begin(), ::toupper);
			//Check for double spaces and illegal(useless) characters and remove them
			int i = 0;
			while (i < tmp.size()) {
				if (i + 1 < tmp.size() && tmp[i] == ' ' && tmp[i + 1] == ' ')
					tmp.erase(i, 1);
				else if (('0' > tmp[i] || tmp[i] > '9') && ('A' > tmp[i] || tmp[i] > 'Z') && tmp[i] != ',' && tmp[i] != ' ' && tmp[i] != '-')
					tmp.erase(i, 1);
				else
					i++;
			}
			//Check whether a comma exists, if it does then separate the label section
			if (tmp.find(',') != std::string::npos) {
				line_label = tmp.substr(0, tmp.find(',') + 1);
				tmp.erase(0, tmp.find(',') + 1);
			}
			else
				line_label = "";
			//What's left of the current line would be the instruction itself
			line_instruction = tmp;
			current_line.push_back(line_label);
			current_line.push_back(line_instruction);
			current_line.push_back(line_comment);
			//Add the current line to the rest
			result.push_back(current_line);
		}
		code_file.close();
		//If more than 5000 lines of code exist, throw an error
		if (result.size() > 5000) {
			command = "log.innerHTML = 'The file contains more than 5000 lines.';log.style.color = 'rgb(110, 10, 10)';";
			script = JSStringCreateWithUTF8CString(command.c_str());
			JSEvaluateScript(ctx, script, 0, 0, 0, 0);
		}
		else {
			//Store the data from the file into the code table in the GUI
			for (int i = 0; i < result.size(); i++) {
				command += "document.getElementsByClassName('rowLabelInput')[" + std::to_string(i) + "].value = '" + result[i][0] + "';";
				command += "document.getElementsByClassName('rowInstructionInput')[" + std::to_string(i) + "].value = '" + result[i][1] + "';";
				command += "document.getElementsByClassName('rowCommentInput')[" + std::to_string(i) + "].value = '" + result[i][2] + "';";
			}
			command += "log.innerHTML = 'File successfully loaded.';log.style.color = 'rgb(10, 110, 10)';";
			script = JSStringCreateWithUTF8CString(command.c_str());
			JSEvaluateScript(ctx, script, 0, 0, 0, 0);
		}
	}
	//If the user cancels opening a file
	else {
		command = "log.innerHTML = 'Load cancelled.';log.style.color = 'rgb(0, 0, 0)';";
		script = JSStringCreateWithUTF8CString(command.c_str());
		JSEvaluateScript(ctx, script, 0, 0, 0, 0);
	}

	JSStringRelease(script);

	return JSValueMakeNull(ctx);
}

//The save file function
JSValueRef show_save_file(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception) {
	JSStringRef script;
	std::string command = "", address;

	#if defined(_WIN32) || defined(_WIN64)
		//A Windows CMD command that runs the powershell script for opening a load file dialog box
		address = exec("powershell -WindowStyle hidden -executionpolicy bypass -file savefiledialog-txt.ps1");
	#else
		//A Linux terminal command that runs the script for opening a load file dialog box
		address = exec("zenity --file-selection --save");
	#endif

	if (address == "***Failed***") {
		command = "log.innerHTML = 'Failed to save file.';log.style.color = 'rgb(110, 10, 10)';";
		script = JSStringCreateWithUTF8CString(command.c_str());
		JSEvaluateScript(ctx, script, 0, 0, 0, 0);
	}
	//If the user doesn't cancel opening a file
	else if (address != "Cancel") {
		std::ofstream code_file(address);
		std::string result, line_label, line_instruction, line_comment;
		//Fetch each line of code from the table in the GUI
		for (int i = 0; i < 5000; i++) {
			command = "document.getElementsByClassName('rowLabelInput')[" + std::to_string(i) + "].value.toString()";
			script = JSStringCreateWithUTF8CString(command.c_str());
			result = std::string(String(JSString(JSValueToStringCopy(ctx, JSEvaluateScript(ctx, script, 0, 0, 0, 0), 0))).utf8().data());
			line_label = result;
			command = "document.getElementsByClassName('rowInstructionInput')[" + std::to_string(i) + "].value.toString()";
			script = JSStringCreateWithUTF8CString(command.c_str());
			result = std::string(String(JSString(JSValueToStringCopy(ctx, JSEvaluateScript(ctx, script, 0, 0, 0, 0), 0))).utf8().data());
			line_instruction = result;
			command = "document.getElementsByClassName('rowcommentInput')[" + std::to_string(i) + "].value.toString()";
			script = JSStringCreateWithUTF8CString(command.c_str());
			result = std::string(String(JSString(JSValueToStringCopy(ctx, JSEvaluateScript(ctx, script, 0, 0, 0, 0), 0))).utf8().data());
			line_comment = result;
			//If the line is not empty, then store it in the file
			if (!line_instruction.empty()) {
				code_file << line_label;
				code_file << "\t";
				code_file << line_instruction;
				code_file << "\t";
				code_file << line_comment;
				code_file << "\n";
			}
		}
		code_file.close();
		command = "log.innerHTML = 'File successfully saved.';log.style.color = 'rgb(10, 110, 10)';";
		script = JSStringCreateWithUTF8CString(command.c_str());
		JSEvaluateScript(ctx, script, 0, 0, 0, 0);
	}
	else {
		command = "log.innerHTML = 'Save cancelled.';log.style.color = 'rgb(0, 0, 0)';";
		script = JSStringCreateWithUTF8CString(command.c_str());
		JSEvaluateScript(ctx, script, 0, 0, 0, 0);
	}

	JSStringRelease(script);

	return JSValueMakeNull(ctx);
}

MyApp::MyApp() {
	app_ = App::Create();
	window_ = Window::Create(app_->main_monitor(), WINDOW_WIDTH, WINDOW_HEIGHT, false, kWindowFlags_Titled | kWindowFlags_Resizable);
	overlay_ = Overlay::Create(window_, 1, 1, 0, 0);
	OnResize(window_.get(), window_->width(), window_->height());
	window_->MoveToCenter();
	overlay_->view()->LoadURL("file:///app.html");
	app_->set_listener(this);
	window_->set_listener(this);
	overlay_->view()->set_load_listener(this);
	overlay_->view()->set_view_listener(this);
}

MyApp::~MyApp() {}

void MyApp::Run() {
	app_->Run();
}

void MyApp::OnUpdate() {}

void MyApp::OnClose(ultralight::Window* window) {
	app_->Quit();
}

void MyApp::OnResize(ultralight::Window* window, uint32_t width, uint32_t height) {
	overlay_->Resize(width, height);
}

void MyApp::OnFinishLoading(ultralight::View* caller, uint64_t frame_id, bool is_main_frame, const String& url) {}

void MyApp::OnDOMReady(ultralight::View* caller, uint64_t frame_id, bool is_main_frame, const String& url) {
	auto scoped_context = caller->LockJSContext();
	JSContextRef ctx = (*scoped_context);
	JSObjectRef globalObj = JSContextGetGlobalObject(ctx);
  
	//Define the javascript functions that will be executed using c++

	JSStringRef name1 = JSStringCreateWithUTF8CString("assemble");
	JSObjectRef func1 = JSObjectMakeFunctionWithCallback(ctx, name1, assemble);
  
	JSObjectSetProperty(ctx, globalObj, name1, func1, 0, 0);
  
	JSStringRef name2 = JSStringCreateWithUTF8CString("executeNext");
	JSObjectRef func2 = JSObjectMakeFunctionWithCallback(ctx, name2, execute_next);
  
	JSObjectSetProperty(ctx, globalObj, name2, func2, 0, 0);
  
	JSStringRef name3 = JSStringCreateWithUTF8CString("previousState");
	JSObjectRef func3 = JSObjectMakeFunctionWithCallback(ctx, name3, previous_state);
  
	JSObjectSetProperty(ctx, globalObj, name3, func3, 0, 0);
  
	JSStringRef name4 = JSStringCreateWithUTF8CString("showLoadFile");
	JSObjectRef func4 = JSObjectMakeFunctionWithCallback(ctx, name4, show_load_file);
  
	JSObjectSetProperty(ctx, globalObj, name4, func4, 0, 0);
  
	JSStringRef name5 = JSStringCreateWithUTF8CString("showSaveFile");
	JSObjectRef func5 = JSObjectMakeFunctionWithCallback(ctx, name5, show_save_file);
  
	JSObjectSetProperty(ctx, globalObj, name5, func5, 0, 0);

	JSStringRelease(name1);
	JSStringRelease(name2);
	JSStringRelease(name3);
	JSStringRelease(name4);
	JSStringRelease(name5);
}

void MyApp::OnChangeCursor(ultralight::View* caller, Cursor cursor) {
	window_->SetCursor(cursor);
}

void MyApp::OnChangeTitle(ultralight::View* caller, const String& title) {
	window_->SetTitle(title.utf8().data());
}
