/*
	Name: StyleChecker
	Copyright: 2021
	Author: Daniel R. Collins
	Date: 04/08/21 00:05
	Description: Check approved style for student C++ assignment submissions.
		Broadly aligned with Gaddis C++ textbook styling.
*/
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <cassert>
#include <windows.h>
using namespace std;

// Switch for function length check
const bool CHECK_FUNC_LENGTH = false;

// StyleChecker class
class StyleChecker {
	public:
		bool readFile(char* filecode);
		void writeFile();
		void checkErrors();
		void showTokens();

	protected:

		// Helper functions
		string getFirstFile(string fileCode);
		void printError(string error);
		void printErrors(string error, vector<int> lines);
		int getFirstCommentLine();
		int getFirstNonspace(string line);
		int getLastNonspace(string line);
		bool isIndentTabs(string line);
		bool isBlank(int line);
		bool isBlank(string line);
		bool isPunctuation(char c);
		bool isMidBlockComment(int line);
		bool isLineLabel(string line);
		bool isLineStartCloseBrace(string line);
		bool isRunOnLine(int line);
		bool isOkayIndentLevel(int line);
		bool isSameScope (int startLine, int numLines);
		void scanCommentLines();
		void scanScopeLevels();

		// Token-based helper functions
		string getNextToken(string s, int &pos);
		string getFirstToken(string s);
		string getLastToken(string s);
		bool isType(string s);
		bool isOkConstant(string s);
		bool isOkVariable(string s);
		bool isOkFunction(string s);
		bool isOkStructure(string s);
		bool isOkClass(string s);
		bool isSpacedOperator(string s);
		bool isStartParen(string s);
		bool isFunctionHeader (string s, string &name);
		bool stringStartsWith(string s, string t);
		bool stringEndsWith(string s, string t);

		// Readability items
		void checkLineLength();
		void checkTabUsage();
		void checkIndentLevels();
		void checkSpacesAroundOps();
		void checkExtraneousBlanks();
		void checkVariableNames();
		void checkConstantNames();
		void checkFunctionNames();
		void checkStructureNames();
		void checkClassNames();
		void checkPunctuationSpacing();
		void checkSpacedOperators();
		void checkFunctionLength();

		// Documentation items
		void checkAnyComments();
		void checkHeaderStart();
		void checkHeaderFormat();
		void checkEndLineComments();
		void checkBlanksBeforeComments();
		void checkTooFewComments();
		void checkTooManyComments();
		void checkEndlineRunonComments();
		void checkStartSpaceComments();

	private:
		vector<string> fileLines;
		vector<int> commentLines;
		vector<int> scopeLevels;
};

// Character codes to avoid confusing checker on this file
const char COMMA = 44;
const char SEMICOLON = 59;
const char QUESTION_MARK = 63;
const char LEFT_BRACE = 123;
const char RIGHT_BRACE = 125;

// String codes to avoid confusing checker on this file
const char C_COMMENT_START[] = {'/', '*', '\0'};
const char C_COMMENT_END[] = {'*', '/', '\0'};
const char DOUBLE_SLASH[] = {'/', '/', '\0'};

// Combined check-errors function
//   Prioritize these (by what visually bugs me most)
//   since after 4 errors per section
//   we'll cut off the rest for student focus.
void StyleChecker::checkErrors() {

	// Readability items
	cout << "\n# Readability #\n";
	checkFunctionLength();
	checkLineLength();
	checkIndentLevels();
	checkTabUsage();
	checkVariableNames();
	checkConstantNames();
	checkFunctionNames();
	checkStructureNames();
	checkClassNames();
	checkExtraneousBlanks();
	checkPunctuationSpacing();
	checkSpacedOperators();

	// Documentation items
	cout << "\n# Documentation #\n";
	checkAnyComments();
	checkHeaderStart();
	checkHeaderFormat();
	checkBlanksBeforeComments();
	checkEndLineComments();
	checkEndlineRunonComments();
	checkTooFewComments();
	checkTooManyComments();
	checkStartSpaceComments();
}

// Get first file that matches argument (Windows specific)
//   From: https://stackoverflow.com/questions/612097
string StyleChecker::getFirstFile(string filecode) {
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind = FindFirstFile(filecode.c_str(), &FindFileData);
	if (hFind == INVALID_HANDLE_VALUE) {
		return "";
	}
	else {
		FindClose(hFind);
		return FindFileData.cFileName;
	}
}

// Read a code file
bool StyleChecker::readFile(char* filecode) {

	// Open the file
	string firstFile = getFirstFile(filecode);
	ifstream inFile(firstFile);
	if (!inFile) {
		cerr << "Error: File not found.\n";
		return false;
	}

	// Read the file
	string nextLine;
	while (getline(inFile, nextLine)) {
		fileLines.push_back(nextLine);
	}
	inFile.close();

	// Post-processing
	scanCommentLines();
	scanScopeLevels();
	return true;
}

// Print the read file (for testing)
void StyleChecker::writeFile() {
	for (string line: fileLines) {
		cout << line << endl;
	}
	cout << endl;
}

// Get first nonspace position in a string
//   Returns -1 if none such.
int StyleChecker::getFirstNonspace(string line) {
	for (int i = 0; i < line.length(); i++) {
		if (!isspace(line[i])) {
			return i;
		}
	}
	return -1;
}

// Get last nonspace position in a string
//   Returns -1 if none such.
int StyleChecker::getLastNonspace(string line) {
	for (int i = line.length() - 1; i >= 0; i--) {
		if (!isspace(line[i])) {
			return i;
		}
	}
	return -1;
}

// Is this line a blank (all whitespace)?
bool StyleChecker::isBlank(string line) {
	return getFirstNonspace(line) == -1;
}

// Is this line number a blank?
bool StyleChecker::isBlank(int line) {
	return isBlank(fileLines[line]);
}

// Does string s start with string t?
bool StyleChecker::stringStartsWith(string s, string t) {
	if (s.length() >= t.length()) {
		for (int i = 0; i < t.length(); i++) {
			if (s[i] != t[i]) {
				return false;
			}
		}
		return true;
	}
	return false;
}

// Does string s end with string t?
bool StyleChecker::stringEndsWith(string s, string t) {
	if (s.length() >= t.length()) {
		int offset = s.length() - t.length();
		for (int i = 0; i < t.length(); i++) {
			if (s[i + offset] != t[i]) {
				return false;
			}
		}
		return true;
	}
	return false;
}

// Find where the comment lines are
//    Assumes comments are full lines (no end-line comments, etc.)
//    Records lines as 0: no comment, 1: C-style, 2: C++-style
void StyleChecker::scanCommentLines() {
	commentLines.resize(fileLines.size());
	bool inCstyleComment = false;
	for (int i = 0; i < fileLines.size(); i++) {
		string firstToken = getFirstToken(fileLines[i]);
		string lastToken = getLastToken(fileLines[i]);

		// Check C-style comment
		if (stringStartsWith(firstToken, C_COMMENT_START)) {
			inCstyleComment = true;
		}
		if (inCstyleComment) {
			commentLines[i] = 1;
		}
		if (stringEndsWith(lastToken, C_COMMENT_END)) {
			inCstyleComment = false;
		}

		// Check C++-style comment
		if (stringStartsWith(firstToken, DOUBLE_SLASH)) {
			commentLines[i] = 2;
		}
	}
}

// Does this line start with a closing brace?
bool StyleChecker::isLineStartCloseBrace(string line) {
	int firstChar = getFirstNonspace(line);
	return firstChar >= 0 && line[firstChar] == RIGHT_BRACE;
}

// Does this line start with a label of interest?
bool StyleChecker::isLineLabel(string line) {
	const string LABELS[] = {"case", "public", "private", "protected"};
	string firstToken = getFirstToken(line);
	for (string label: LABELS) {
		if (firstToken == label) {
			return true;
		}
	}
	return false;
}

// Scan for scope level at each line
//   Increments at each brace
//   Also counts labels as incremented scope
void StyleChecker::scanScopeLevels() {
	scopeLevels.resize(fileLines.size());
	int scopeLevel = 0;
	bool inLabel = false;
	for (int i = 0; i < fileLines.size(); i++) {
		if (commentLines[i]) {
			scopeLevels[i] = scopeLevel;
		}
		else {
			string line = fileLines[i];
			bool lineLabel = isLineLabel(line);

			// Decrement for start closing brace
			if (isLineStartCloseBrace(line)) {
				scopeLevel--;
				if (inLabel) {
					scopeLevel--;
					inLabel = false;
				}
			}

			// Decrement for label-in-label
			if (inLabel && lineLabel) {
				scopeLevel--;
			}

			// Set that level
			scopeLevels[i] = scopeLevel;

			// Increment for a label
			if (isLineLabel(line)) {
				scopeLevel++;
				inLabel = true;
			}

			// Scan rest of line for more braces to adjust
			int firstChar = getFirstNonspace(line);
			if (firstChar >= 0) {
				for (int i = firstChar; i < line.length(); i++) {
					switch (line[i]) {
						case LEFT_BRACE: scopeLevel++; break;
						case RIGHT_BRACE: if (i > firstChar) {
								scopeLevel--; break;
						}
					}
				}
			}
		}
	}
}

// Print basic error
void StyleChecker::printError(string error) {
	cout << error << "\n";
}

// Format an error report with line numbers
//   If lines vector is empty, then nothing is printed.
//   Line numbers are incremented for user display.
void StyleChecker::printErrors(string error, vector<int> lines) {

	// Singular error
	if (lines.size() == 1) {
		cout << error << " (line " << lines[0] + 1 << ").\n";
	}

	// Multiple errors
	else if (lines.size() > 1) {
		const int MAX_SHOWN = 3;
		cout << error << " (lines " << lines[0] + 1;
		for (int i = 1; i < lines.size() && i < MAX_SHOWN; i++) {
			cout << ", " << lines[i] + 1;
		}
		if (lines.size() > MAX_SHOWN) {
			cout << ", etc";
		}
		cout << ").\n";
	}
}

// Get index of first comment line
//   Returns -1 if none whatsoever
int StyleChecker::getFirstCommentLine() {
	for (int i = 0; i < fileLines.size(); i++) {
		if (commentLines[i]) {
			return i;
		}
	}
	return -1;
}

// Check if the file has any comment lines at all
void StyleChecker::checkAnyComments() {
	if (getFirstCommentLine() == -1) {
		printError("File lacks any comment lines!");
	}
}

// Check header start
//   File should start with a comment
void StyleChecker::checkHeaderStart() {
	if (getFirstCommentLine() != 0) {
		printError("Misplaced file comment header (line 1).");
	}
}

// Check file header
void StyleChecker::checkHeaderFormat() {
	vector<int> errorLines;
	const string HEADER[] = {C_COMMENT_START, "\tName:", "\tCopyright:",
		"\tAuthor:", "\tDate:", "\tDescription:"};
	int currLine = getFirstCommentLine();
	if (currLine >= 0) {
		for (string headPrefix: HEADER) {
			if (currLine >= fileLines.size()
				|| !stringStartsWith(fileLines[currLine], headPrefix))
			{
				errorLines.push_back(currLine);
			}
			currLine++;
		}
	}
	printErrors("Invalid comment header", errorLines);
}

// Check line lengths
void StyleChecker::checkLineLength() {
	const int MAX_LENGTH = 80;
	vector<int> errorLines;
	for (int i = 0; i < fileLines.size(); i++) {
		if (fileLines[i].length() > MAX_LENGTH) {
			errorLines.push_back(i);
		}
	}
	printErrors("Line is too long", errorLines);
}

// Is the indent in this line all tabs?
bool StyleChecker::isIndentTabs(string line) {
	int firstChar = getFirstNonspace(line);
	for (int i = 0; i < firstChar; i++) {
		if (line[i] != '\t') {
			return false;
		}
	}
	return true;
}

// Check end-line comments
void StyleChecker::checkEndLineComments() {
	vector<int> errorLines;
	for (int i = 0; i < fileLines.size(); i++) {
		if (!commentLines[i]
			&& (fileLines[i].find(DOUBLE_SLASH) != string::npos
			|| fileLines[i].find(C_COMMENT_START) != string::npos))
		{
			errorLines.push_back(i);
		}
	}
	printErrors("End-line comments shouldn't be used", errorLines);
}

// Check tab usage for indents
void StyleChecker::checkTabUsage() {
	vector<int> errorLines;
	for (int i = 0; i < fileLines.size(); i++) {
		if (!isIndentTabs(fileLines[i])) {
			errorLines.push_back(i);
		}
	}
	printErrors("Tabs should be used for indents", errorLines);
}

// Is this line in the middle of a C-style block comment?
bool StyleChecker::isMidBlockComment(int line){
	return commentLines[line] == 1
		&& (line > 0 && commentLines[line - 1] == 1)
		&& (line < commentLines.size() - 1 && commentLines[line + 1] == 1);
}

// Is this line a run-on statement spanning multiple lines?
bool StyleChecker::isRunOnLine(int line) {
	if (line > 0 && !isBlank(line - 1) && !commentLines[line - 1]) {
		string priorLine = fileLines[line - 1];
		int lastPriorChar = getLastNonspace(priorLine);
		if (priorLine[lastPriorChar] != SEMICOLON) {
			return true;
		}
	}
	return false;
}

// Is this line at an accept indent level?
bool StyleChecker::isOkayIndentLevel(int line) {

	// Ignore some cases
	if (isBlank(line) || isMidBlockComment(line)
		|| !isIndentTabs(fileLines[line]))
	{
		return true;
	}

	// Standard indent level
	int indentEnd = getFirstNonspace(fileLines[line]);
	if (indentEnd == scopeLevels[line]) {
		return true;
	}

	// Accept extra indent for run-on line in same scope
	if (isRunOnLine(line)
		&& scopeLevels[line] == scopeLevels[line - 1]
		&& indentEnd == scopeLevels[line] + 1)
	{
		return true;
	}

	// Otherwise no
	return false;
}

// Check indent levels
void StyleChecker::checkIndentLevels() {
	vector<int> errorLines;
	int indentLevel = 0;
	for (int i = 0; i < fileLines.size(); i++) {
		if (!isOkayIndentLevel(i)) {
			errorLines.push_back(i);
		}
	}
	printErrors("Indent level errors", errorLines);
}

// Check blanks before comments (required)
void StyleChecker::checkBlanksBeforeComments() {
	vector<int> errorLines;
	for (int i = 1; i < fileLines.size(); i++) {
		if (commentLines[i]) {
			string priorLine = fileLines[i - 1];
			int priorEndIndent = getFirstNonspace(priorLine);
			if (!isBlank(i - 1)
				&& !commentLines[i - 1]
				&& !(priorLine[priorEndIndent] == LEFT_BRACE))
			{
				errorLines.push_back(i);
			}
		}
	}
	printErrors("Missing blank line before comment", errorLines);
}

// Check for no comments in long stretch of statements
void StyleChecker::checkTooFewComments() {
	const int LONG_STRETCH = 24;
	vector<int> errorLines;
	for (int i = 0; i < fileLines.size(); i++) {
		if (commentLines[i]) {
			int end = i + 1;
			while (end < fileLines.size() && !commentLines[end++]);
			if (end - i > LONG_STRETCH) {
				errorLines.push_back(i + LONG_STRETCH / 2);
			}
		}
	}
	printErrors("Too few comments", errorLines);
}

// Check that next N lines all in same scope
bool StyleChecker::isSameScope (int startLine, int numLines) {
	if (startLine >= 0
		&& startLine + numLines < fileLines.size())
	{
		int startLevel = scopeLevels[startLine];
		for (int i = 1; i <= numLines; i++) {
			if (scopeLevels[startLine + i] != startLevel) {
				return false;
			}
		}
		return true;
	}
	return false;
}

// Check for commenting multiple single-line statements
void StyleChecker::checkTooManyComments() {
	vector<int> errorLines;
	for (int i = 0; i < fileLines.size() - 5; i++) {
		if (commentLines[i]
			&& !commentLines[i + 1]
			&& isBlank(i + 2)
			&& commentLines[i + 3]
			&& !commentLines[i + 4]
			&& isBlank(i + 5)
			&& isSameScope(i, 5))
		{
			errorLines.push_back(i + 3);
		}
	}
	printErrors("Too many comments", errorLines);
}

// Is this character an operator that expects spacing?
//    Note we must skip many symbols used for other purposes, e.g.:
//   '<', '>' used for brackets in #includes, templates
//   "++", "--" pre/postfix to variable
//   "-" used for unary negation (start number)
//   "*" used for pointer operator
bool StyleChecker::isSpacedOperator(string s) {
	const string SPACE_OPS[] = {"+", "/", "%", "<<", ">>", "<=", ">=",
		"==", "!=", "&&", "||", "=", "+=", "-=", "*=", "/="};
	for (string op: SPACE_OPS) {
		if (s == op) {
			return true;
		}
	}
	return false;
}

// Check spaces around operators
void StyleChecker::checkSpacedOperators() {
	vector<int> errorLines;
	for (int i = 0; i < fileLines.size(); i++) {
		if (!commentLines[i]) {
			int pos = 0;
			string line = fileLines[i];
			string token = getNextToken(line, pos);
			while (token != "") {
				if (isSpacedOperator(token)) {

					// Check for space before & after
					int startPos = pos - token.length();
					if ((startPos > 0 && !isspace(line[startPos - 1]))
						|| (pos < line.length() && !isspace(line[pos])))
					{
						errorLines.push_back(i);
						break;
					}
				}
				token = getNextToken(line, pos);
			}
		}
	}
	printErrors("Operators should have surrounding spaces", errorLines);
}

// Check for end-line C-style comments that continue to next line
//   Never seen this, but it would foil all our other comment logic.
void StyleChecker::checkEndlineRunonComments() {
	vector<int> errorLines;
	for (int i = 1; i < fileLines.size(); i++) {
		if (!commentLines[i]
			&& fileLines[i].find(C_COMMENT_START) != string::npos
			&& fileLines[i].find(C_COMMENT_END) == string::npos)
		{
			errorLines.push_back(i);
		}
	}
	printErrors("End-line run-on comments used!", errorLines);
}

// Is this a punctuation character?
bool StyleChecker::isPunctuation(char c) {
	const char PUNCT[] = {COMMA, SEMICOLON, QUESTION_MARK};
	int numPunct = sizeof(PUNCT) / sizeof(*PUNCT);
	for (char punct: PUNCT) {
		if (c == punct) {
			return true;
		}
	}
	return false;
}

// Check for spaces after punctuation, but not before
void StyleChecker::checkPunctuationSpacing() {
	vector<int> errorLines;
	for (int i = 0; i < fileLines.size(); i++) {
		string line = fileLines[i];
		for (int j = 0; j < line.length(); j++) {
			if (isPunctuation(line[j])) {
				if (((j > 1 && isspace(line[j - 1]))
					|| (j < line.length() - 1 && !isspace(line[j + 1]))))
				{
					errorLines.push_back(i);
					break;
				}
			}
		}
	}
	printErrors("Punctuation should have space afterward", errorLines);
}

// Get next token from a line
//   Splits on spaces, identifiers, numbers, and punctuation.
//   Simplistic: Gets blocks of punctuation, glues grouping symbols, etc.
//   Starts at pos; updates pos to after found token.
string StyleChecker::getNextToken(string s, int &pos) {

	// Eat spaces
	while (pos < s.length() && isspace(s[pos])) {
		pos++;
	}

	// Get nothing
	int start = pos;
	if (pos >= s.length()) {
		return "";
	}

	// Get a word
	if (isalpha(s[pos]) || s[pos] == '_') {
		while (pos < s.length() && (isalnum(s[pos]) || s[pos] == '_')) {
			pos++;
		}
	}

	// Get a number
	else if (isdigit(s[pos])) {
		while (pos < s.length() && (isdigit(s[pos]) || s[pos] == '.')) {
			pos++;
		}
	}

	// Get punctuation
	else {
		while (pos < s.length() && ispunct(s[pos])) {
			pos++;
		}
	}

	// Return the found token
	return s.substr(start, pos - start);
}

// Get first token on a line
string StyleChecker::getFirstToken(string s) {
	int pos = 0;
	return getNextToken(s, pos);
}

// Get last token on a line
string StyleChecker::getLastToken(string s) {
	int pos = 0;
	string lastToken = "";
	string nextToken = getNextToken(s, pos);
	while (nextToken != "") {
		lastToken = nextToken;
		nextToken = getNextToken(s, pos);
	}
	return lastToken;
}

// Show all tokens in file (for testing)
void StyleChecker::showTokens() {
	for (string line: fileLines) {
		int pos = 0;
		string token = getNextToken(line, pos);
		while (token != "") {
			cout << token << endl;
			token = getNextToken(line, pos);
		}
		cout << endl;
	}
	cout << endl;
}

// Is this string a fundamental type?
bool StyleChecker::isType(string s) {
	const string TYPES[] = {"int", "float", "double",
		"char", "bool", "string", "void"};
	for (string type: TYPES) {
		if (s == type) {
			return true;
		}
	}
	return false;
}

// Is this string an acceptable constant name?
bool StyleChecker::isOkConstant(string s) {
	if (s.length() < 2) {
		return false;
	}
	for (char c: s) {
		if (!isupper(c) && c != '_') {
			return false;
		}
	}
	return true;
}

// Check constant names
void StyleChecker::checkConstantNames() {
	vector<int> errorLines;
	for (int i = 0; i < fileLines.size(); i++) {
		int pos = 0;
		string line = fileLines[i];
		string prefix = getNextToken(line, pos);
		if (prefix == "const") {
			string type = getNextToken(line, pos);
			if (isType(type)) {
				string name = getNextToken(line, pos);
				if (!isOkConstant(name)) {
					errorLines.push_back(i);
				}
			}
		}
	}
	printErrors("Constants should be all-caps name", errorLines);
}

// Is this string an acceptable variable name?
bool StyleChecker::isOkVariable(string s) {
	if (s.length() < 2
		|| !islower(s[0]))
	{
		return false;
	}
	for (int i = 1; i < s.length(); i++) {
		if (!isalnum(s[i])
			|| (isupper(s[i]) && isupper(s[i - 1])))
		{
			return false;
		}
	}
	return true;
}

// Does this token start with an open parenthesis?
bool StyleChecker::isStartParen(string s) {
	return s.length() > 0 && s[0] == '(';
}

// Check variable names
//   Note we check only first variable declared on a line.
void StyleChecker::checkVariableNames() {
	vector<int> errorLines;
	for (int i = 0; i < fileLines.size(); i++) {
		int pos = 0;
		string line = fileLines[i];
		string type = getNextToken(line, pos);
		if (isType(type)) {
			string name = getNextToken(line, pos);
			if (name == "*") {
				string name = getNextToken(line, pos);
			}
			string nextSymbol = getNextToken(line, pos);
			if (!isStartParen(nextSymbol) && nextSymbol != "::") {
				if (!isOkVariable(name)) {
					errorLines.push_back(i);
				}
			}
		}
	}
	printErrors("Variables should be camel-case name", errorLines);
}

// Is this string an acceptable function name?
//   Currently same rule as for variable names.
bool StyleChecker::isOkFunction(string s) {
	return isOkVariable(s);
}

// Check function names
void StyleChecker::checkFunctionNames() {
	vector<int> errorLines;
	for (int i = 0; i < fileLines.size(); i++) {
		string name;
		if (isFunctionHeader(fileLines[i], name)) {
			if (!isOkFunction(name)) {
				errorLines.push_back(i);
			}
		}
	}
	printErrors("Functions should be camel-case name", errorLines);
}

// Is the given line a function header?
//   If so, return function name in parameter.
bool StyleChecker::isFunctionHeader (string line, string &name) {
	int pos = 0;
	string type = getNextToken(line, pos);
	if (isType(type)) {
		name = getNextToken(line, pos);
		if (name == "*") {
			string name = getNextToken(line, pos);
		}
		string nextSymbol = getNextToken(line, pos);
		if (isStartParen(nextSymbol)) {
			int lastChar = line[getLastNonspace(line)];
			if (lastChar != SEMICOLON) {
				return true;
			}
		}
	}
	return false;
}

// Is this string an acceptable structure name?
bool StyleChecker::isOkStructure(string s) {
	if (s.length() < 2
		|| !isupper(s[0]))
	{
		return false;
	}
	for (int i = 1; i < s.length(); i++) {
		if (!isalnum(s[i])
			|| (isupper(s[i]) && isupper(s[i - 1])))
		{
			return false;
		}
	}
	return true;
}

// Check structure names
void StyleChecker::checkStructureNames() {
	vector<int> errorLines;
	for (int i = 0; i < fileLines.size(); i++) {
		int pos = 0;
		string line = fileLines[i];
		string prefix = getNextToken(line, pos);
		if (prefix == "struct") {
			string name = getNextToken(line, pos);
			if (!isOkStructure(name)) {
				errorLines.push_back(i);
			}
		}
	}
	printErrors("Structures should start caps camel-case", errorLines);
}

// Is this string an acceptable class name?
//   Currently same rule as for variable names.
bool StyleChecker::isOkClass(string s) {
	return isOkStructure(s);
}

// Check structure names
void StyleChecker::checkClassNames() {
	vector<int> errorLines;
	for (int i = 0; i < fileLines.size(); i++) {
		int pos = 0;
		string line = fileLines[i];
		string prefix = getNextToken(line, pos);
		if (prefix == "class") {
			string name = getNextToken(line, pos);
			if (!isOkClass(name)) {
				errorLines.push_back(i);
			}
		}
	}
	printErrors("Classes should start caps camel-case", errorLines);
}

// Check extraneous blank lines
//    Blank lines should only occur:
//    before comment, label, or function header
void StyleChecker::checkExtraneousBlanks() {
	vector<int> errorLines;
	for (int i = 0; i < fileLines.size() - 1; i++) {
		if (isBlank(i)) {
			string func;
			int next = i + 1;
			if (!commentLines[next]
				&& !isLineLabel(fileLines[next])
				&& !isFunctionHeader(fileLines[next], func))
			{
				errorLines.push_back(i);
			}
		}
	}
	printErrors("Extraneous blank lines", errorLines);
}

// C++-style comments should have a space after slashes.
void StyleChecker::checkStartSpaceComments() {
	vector<int> errorLines;
	for (int i = 0; i < fileLines.size(); i++) {
		int pos = 0;
		string line = fileLines[i];
		string token = getNextToken(line, pos);
		if (token == DOUBLE_SLASH
			&& pos < line.size()
			&& !isspace(line[pos]))
		{
			errorLines.push_back(i);
		}
	}
	printErrors("Comments need space after slashes", errorLines);
}

// Check for overly long functions.
void StyleChecker::checkFunctionLength() {
	if (CHECK_FUNC_LENGTH) {
		const int LONG_FUNC = 20;
		vector<int> errorLines;
		for (int i = 0; i < fileLines.size(); i++) {
			string func;
			if (isFunctionHeader(fileLines[i], func)) {
				int end = i + 1;
				while (end < fileLines.size() && scopeLevels[end] > 0) {
					end++;
				}
				int funcLength = end - i;
				if (funcLength > LONG_FUNC) {
					errorLines.push_back(i);
				}
			}
		}
		printErrors("Function is too long!", errorLines);
	}
}

// Main test driver
int main(int argc, char** argv) {
	StyleChecker checker;
	if (argc < 2) {
		cout << "Usage: StyleChecker file\n";
		return 1;
	}
	if (checker.readFile(argv[1])) {
		checker.checkErrors();
	}
	return 0;
}