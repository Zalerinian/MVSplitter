#include "HTMLWriter.h"
#include "Console.h"
#include "FileStatus.h"
#include <Windows.h>
#include <fstream>
#include <iostream>
#include <regex>
#include <vector>
#include <string>
#include <sstream>
#include <direct.h>
#include <thread>
#include <mutex>
#include <algorithm>
#include <iomanip>
#include <map>


using namespace std;
using namespace System;

void Logger(stringstream& stream);
vector<string> stringSplit(string& str, regex& r);
void getContents(ifstream& read, string& out);
bool createDirectory(const char* path);
void findCodeVersion(const char* data);
void processSection(string data, string folder, const char* rpgfile, unsigned int index);
void threadDraw();
void threadParseFile(const char* rpgfile);
void ResetColors();
void DrawHeader();
void SetSize(unsigned int w, unsigned int h);
void ListClasses(string file);
void drawList();


string workingDirectory, codeVersion = "Unknown", windowTitle;
mutex logLock, readMutex, writtenMutex, workerMutex, trackerMutex;
unsigned int totalWritten, totalRead, fileCount, workerThreads, errorCount = 0;
vector<FileStatus> tracker;
vector<thread> threads;
map<const char*, vector<string>> scriptLines;


int main(int argc, char**argv) {
	Console::CursorVisible(false);
	SetWindowText(Console::Handle(), L"MVSplitter");
	workingDirectory = argv[0];
	workingDirectory = workingDirectory.substr(0, workingDirectory.find_last_of('\\') + 1);
	windowTitle = string("MVSplitter - " + workingDirectory);
	if (!createDirectory((workingDirectory + "output\\").c_str())) {
		system("pause");
		return 1;
	}
	vector<char*> coreFiles{ "rpg_core", "rpg_managers", "rpg_objects", "rpg_scenes", "rpg_sprites", "rpg_windows" };
	WINDOWINFO *info = new WINDOWINFO;
	info->cbSize = sizeof(WINDOWINFO);
	GetWindowInfo(Console::Handle(), info);
	SetWindowPos(Console::Handle(), NULL, 200, 200, 1000, 500, 0);
	SetSize(155, 53);
	DrawHeader();

	for (unsigned int fileID = 0; fileID < coreFiles.size(); fileID++) {
		threads.push_back(std::thread(threadParseFile, coreFiles[fileID]));
	}
	threads.push_back(std::thread(threadDraw));
	for (unsigned int i = 0; i < threads.size(); i++) {
		threads[i].join();
	}
	// One final draw now that everything is complete.
	DrawHeader();
	drawList();
	ResetColors();
	Console::SetCursorPosition(0, 46);
	bool htmlopen = HTMLWriter::WriteHead((workingDirectory + "output\\index.html").c_str(), (string("RPG Maker MV ") + codeVersion).c_str());
	if (htmlopen) {
		HTMLWriter::WriteSection("RPG Core", scriptLines["rpg_core"]);
		HTMLWriter::WriteSection("RPG Managers", scriptLines["rpg_managers"]);
		HTMLWriter::WriteSection("RPG Objects", scriptLines["rpg_objects"]);
		HTMLWriter::WriteSection("RPG Scenes", scriptLines["rpg_scenes"]);
		HTMLWriter::WriteSection("RPG Sprites", scriptLines["rpg_sprites"]);
		HTMLWriter::WriteSection("RPG Windows", scriptLines["rpg_windows"]);
		HTMLWriter::WriteEnd();
		cout << "All files output successfully.";
	} else {
		cout << "Error writing index.html file.";
	}
	Console::SetCursorPosition(0, 52);
	system("pause");
	return 0;
}

void Logger(stringstream& stream) {
	logLock.lock();
	Console::SetCursorPosition(0, 40 + errorCount);
	errorCount += 2;
	Console::BackgroundColor(DarkRed);
	Console::ForegroundColor(Gray);
	cout << stream.str().c_str() << endl;
	stream.str(string());
	logLock.unlock();
}

vector<string> stringSplit(string& str, regex& r) {
	smatch m;
	regex_search(str, m, r);
	if (m.size() == 0) {
		// No matches were found, so return nothing.
		return vector<string>();
	}
	regex_iterator<string::iterator> eof, pos(str.begin(), str.end(), r);
	vector<string> ret;
	unsigned int offset = pos->position(0);
	for (++pos; pos != eof; pos++) {
		ret.push_back(str.substr(offset, pos->position() - offset));
		offset = pos->position();
	}
	ret.push_back(str.substr(offset));
	return ret;
}

void getContents(ifstream& read, string& out) {
	unsigned int length = 0;
	read.seekg(0, ios_base::end);
	length = (unsigned int)read.tellg();
	read.seekg(0, ios_base::beg);
	char* fileContents = new char[length];
	memset(fileContents, 0, length);
	read.read(fileContents, length);
	out = fileContents;
	delete[] fileContents;
	readMutex.lock();
	totalRead += length;
	readMutex.unlock();
}

bool createDirectory(const char* path) {
	stringstream stream;
	if (_mkdir(path) != 0) {
		if (errno != EEXIST) {
			char buffer[1024];
			strerror_s(buffer, errno);
			stream << "[ERROR] Could not create folder \"" << path << "\": " << buffer;
			Logger(stream);
			return false;
		}
	}
	return true;
}

void findCodeVersion(const char* data) {
	regex versionFind("Utils.RPGMAKER_VERSION = \"(.*)\"", regex_constants::ECMAScript | regex_constants::icase);
	cmatch m;
	regex_search(data, m, versionFind);
	if (m.size() > 0) {
		codeVersion = m.str(1);
	}
}

void processSection(string data, string folder, const char* rpgfile, unsigned int index) {
	stringstream stream;
	regex functionFinder("^function ([^\\( ]+)\\s*\\(.*\\)", regex_constants::ECMAScript | regex_constants::icase);
	auto inner = stringSplit(data, functionFinder);
	if (inner.size() == 0) {
		stream << "[WARNING] No classes found in code section " << (index + 1) << "!";
		Logger(stream);
		return;
	}
	string classContent = "";
	for (unsigned int j = 0; j < inner.size(); j++) {
		smatch m;
		regex_search(inner[j], m, functionFinder);
		classContent += inner[j];
		if (m.str(1) == "handleiOSTouch") {
			// this function goes along with the graphics class, but since it was declared
			// as a separate function, we need to handle that case here, or it would become
			// its own separate file.
			continue;
		}
		auto className = m.str(1);
		std::ofstream writer((folder + className + ".js").c_str(), ios_base::out);
		writer.write(classContent.c_str(), classContent.size());
		writer.close();
		writtenMutex.lock();
		totalWritten += classContent.size();
		fileCount++;
		writtenMutex.unlock();
		classContent = "";
		trackerMutex.lock();
		auto it = find(tracker.begin(), tracker.end(), className);
		if (it != tracker.end()) {
			(*it).complete = true;
		}
		scriptLines[rpgfile].push_back((string("./js/" + string(rpgfile) + "/" + className)));
		trackerMutex.unlock();
	}
}

void threadDraw() {
	while (workerThreads > 0) {
		trackerMutex.lock();
		logLock.lock();
		Console::Lock(true);
		DrawHeader();
		drawList();
		logLock.unlock();
		trackerMutex.unlock();
		Console::Lock(false);
		this_thread::sleep_for(std::chrono::milliseconds(15));
	}
	ResetColors();
}

void threadParseFile(const char* rpgfile) {
	workerMutex.lock();
	workerThreads++;
	workerMutex.unlock();
	regex classSplitter("//-----------------------------------------------------------------------------", regex_constants::ECMAScript | regex_constants::icase);
	ifstream read;
	stringstream stream;
	read.open((workingDirectory + rpgfile + ".js").c_str(), ios_base::in);
	if (!read.is_open()) {
		char buffer[1024];
		strerror_s(buffer, errno);
		stream << "[ERROR] Could not open file \"" << rpgfile << "\" for reading: " << buffer;
		Logger(stream);
		return;
	}
	string workingFolder = workingDirectory + "output\\" + rpgfile + "\\";
	if (!createDirectory(workingFolder.c_str())) {
		return;
	}

	string file = "";
	getContents(read, file);
	read.close();
	ListClasses(file);
	if (strncmp(rpgfile, "rpg_core", 8) == 0) {
		findCodeVersion(file.c_str());
	}

	auto classes = stringSplit(file, classSplitter);
	for (unsigned int i = 0; i < classes.size(); i++) {
		processSection(classes[i], workingFolder, rpgfile, i);
	}
	workerMutex.lock();
	workerThreads--;
	workerMutex.unlock();
}

void ResetColors() {
	Console::ResetColor();
}

void DrawHeader() {
	Console::SetCursorPosition(0, 0);
	Console::BackgroundColor(DarkRed);
	for (int i = 0; i < Console::WindowWidth() * 2; i++) {
		cout << " ";
	}

	if (windowTitle.size() > 127) {
		windowTitle = "MVSplitter - ..." + windowTitle.substr(windowTitle.size() - 127);
	}
	int left = Console::WindowWidth() / 2 - windowTitle.size() / 2;
	Console::SetCursorPosition(left, 0);
	Console::ForegroundColor(White);
	cout << windowTitle;
	Console::SetCursorPosition((Console::WindowWidth() - 100) / 2, 1);
	cout << "Total bytes read: " << setw(7) << setfill('0') << totalRead << setw(1);
	cout << " | Total bytes written: " << setw(7) << totalWritten << setw(1);
	cout << " | Total files written: " << setw(3) << fileCount;
	cout << " | Version: " << codeVersion;
	ResetColors();
}

void SetSize(unsigned int w, unsigned int h) {
	Console::SetBufferSize(w, h);
	Console::SetWindowSize(w, h);
}

void ListClasses(string file) {
	regex functionFinder("^function ([^\\( ]+)\\s*\\(.*\\)", regex_constants::ECMAScript | regex_constants::icase);
	regex_iterator<string::iterator> finder(file.begin(), file.end(), functionFinder), eof;
	trackerMutex.lock();
	int i = 0;
	for (; finder != eof; finder++) {
		if (finder->str(1) == "handleiOSTouch") {
			continue;
		}
		FileStatus f;
		f.name = finder->str(1);
		tracker.push_back(f);
		if (++i % 3 == 0) {
			drawList();
		}
	}
	drawList();
	trackerMutex.unlock();
}

void drawList() {
	for (unsigned int i = 0; i < tracker.size(); i++) {
		FileStatus f = tracker[i];
		Console::SetCursorPosition(31 * (i % 5), 2 + (i / 5));
		if (f.complete) {
			Console::ForegroundColor(Green);
		} else {
			Console::ForegroundColor(Red);
		}
		cout << f.name.substr(0, 30);
	}
}

