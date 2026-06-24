/*
*  Program: uci-analyser: a UCI-based Chess Game Analyser
*  Copyright (C) 1994-2015 David Barnes
*  This program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 1, or (at your option)
*  any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program; if not, write to the Free Software
*  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*
*  David Barnes may be contacted as D.J.Barnes@kent.ac.uk
*  http://www.cs.kent.ac.uk/people/staff/djb/
*
*  This software has been modified for compatibility with PGN Spy.  The original may be found at:
*  https://www.cs.kent.ac.uk/people/staff/djb/chessplag/
*
*/

#include "engine.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <string.h>
#include <stdlib.h>
#include <vector>

#include "utils.h"

// Engine state now lives on the Engine instance as atomic members.
// No more global variables — see engine.h for m_bEngineClosed, m_bWaitingForResponse,
// m_iResponseCount, and m_responseBuffer.

namespace {

string trimWhitespace(const string& value) {
   string::size_type start = value.find_first_not_of(" \t\r\n");
   if (start == string::npos)
      return "";

   string::size_type end = value.find_last_not_of(" \t\r\n");
   return value.substr(start, end - start + 1);
}

string normaliseOptionName(const string& name) {
   string normalised = trimWhitespace(name);
   transform(normalised.begin(), normalised.end(), normalised.begin(),
      [](unsigned char ch) { return static_cast<char>(tolower(ch)); });
   return normalised;
}

bool startsWith(const string& value, const char* prefix) {
   size_t prefixLength = strlen(prefix);
   return value.compare(0, prefixLength, prefix) == 0;
}

bool extractOptionName(const string& response, string& optionName) {
   static const char* OPTION_PREFIX = "option name ";
   if (!startsWith(response, OPTION_PREFIX))
      return false;

   size_t nameStart = strlen(OPTION_PREFIX);
   size_t typePos = response.find(" type ", nameStart);
   if (typePos == string::npos)
      optionName = response.substr(nameStart);
   else
      optionName = response.substr(nameStart, typePos - nameStart);

   optionName = trimWhitespace(optionName);
   return !optionName.empty();
}

}

void Engine::clearHash(void) {
   if (supportsOption("Clear Hash")) {
      send("setoption name Clear Hash");
   }
}

void Engine::go(void) {
   stringstream ss;
   ss << "go infinite";
   send(ss.str());
}

void Engine::stop(void) {
    stringstream ss;
    ss << "stop";
    send(ss.str());
}

void Engine::setPosition(const string& moves, const string& fenstring) {
    if(fenstring.length() == 0) {
      send("position startpos moves " + moves);
    }
    else {
        setFENPosition(fenstring, moves);
    }
}

void Engine::setFENPosition(const string& fenstring, const string& moves) {
    stringstream ss;
    ss << "position fen " << fenstring << " moves " + moves;
    send(ss.str());
}

/*
 * Send a setoption command to the engine using the
 * given name and value.
 */
void Engine::setOption(const string& name, const string& value) {
    stringstream ss;
    ss << "setoption name " << name << " value " << value;
    send(ss.str());
}

/*
 * Send a setoption command to the engine using the
 * given name and value.
 */
void Engine::setOption(const string& name, int value) {
    stringstream ss;
    ss << value;
    setOption(name, ss.str());
}

void Engine::setOptionIfSupported(const string& name, const string& value) {
    if (supportsOption(name)) {
        setOption(name, value);
    }
}

void Engine::setOptionIfSupported(const string& name, int value) {
    if (supportsOption(name)) {
        setOption(name, value);
    }
}

void Engine::setOptions(map<string, string>& options) {
    map<string, string>::iterator it;
    for (it = options.begin(); it != options.end(); it++) {
        setOptionIfSupported(it->first, it->second);
    }
}

bool Engine::supportsOption(const string& name) const {
    return supportedOptions.find(normaliseOptionName(name)) != supportedOptions.end();
}

bool Engine::readUciOptions(void) {
    supportedOptions.clear();

    bool eof = false;
    while (!eof) {
        string response = getResponse(eof);
        if (eof) {
            break;
        }

        if (response == "uciok") {
            return true;
        }

        string optionName;
        if (extractOptionName(response, optionName)) {
            supportedOptions.insert(normaliseOptionName(optionName));
        }
    }

    return false;
}

/*
 * Search with the given moves.
 */
void Engine::searchMoves(const string& moves) {
    stringstream ss;
    ss << "go infinite searchmoves " + moves;
    send(ss.str());
}

/*
 * Initialise the UCI engine.
 * Return true if intialised ok; false otherwise.
 */
bool Engine::initEngine(int variations, int searchDepth, int searchMaxTime, int searchMinTime,
        map<string, string>& options) {
    this->variations = variations;
    this->searchDepth = searchDepth;
    this->searchMaxTime = searchMaxTime;
    this->searchMinTime = searchMinTime;

	send("uci");
    if (readUciOptions()) {
        // Set default options.
        setOptionIfSupported("UCI_AnalyseMode", "true");
        setOptionIfSupported("MultiPV", variations);

        // Set command-line options.
        setOptions(options);

        return startNewGame();
    } else {
        return false;
    }
}

/*
 * Check that the engine is ready.
 */
bool Engine::checkIsReady(void) {
    send("isready");
    return waitForResponse("readyok");
}

void Engine::quitEngine(void) {
    if (m_bEngineClosed)
        return;

    m_bEngineClosed = true;

    send("quit");

    if (hEngineMonitor != NULL) {
        // Wait for monitor thread to finish before closing the handle.
        WaitForSingleObject(hEngineMonitor, 30000);
        CloseHandle(hEngineMonitor);
        hEngineMonitor = NULL;
    }

#ifdef _WIN32
    // Close pipe handles to avoid resource leaks.
    if (writeToEngine != NULL) {
        CloseHandle(writeToEngine);
        writeToEngine = NULL;
    }
    if (readFromEngine != NULL) {
        CloseHandle(readFromEngine);
        readFromEngine = NULL;
    }
    if (m_hEngineProcess != NULL) {
        CloseHandle(m_hEngineProcess);
        m_hEngineProcess = NULL;
    }
#endif
}

/*
 * Send the given string to the engine.
 */
void Engine::send(const string &str) {
    send(str.c_str());
}

/*
 * Send the given string to the engine.
 */
void Engine::send(const char *str) {
#ifdef __unix__
    fprintf(toEngine, "%s\n", str);
    fflush(toEngine);
#else
	DWORD dwWritten;
	BOOL bSuccess = FALSE;

   bSuccess = WriteFile(writeToEngine, str, strlen(str), &dwWritten, NULL);
	if (bSuccess) {
		static const char *newl = "\n";
		bSuccess |= WriteFile(writeToEngine, newl, strlen(newl), &dwWritten, NULL);
	}

#ifdef _DEBUG
   communications.append("\nAnalyser:\t");
   communications.append(str);
#endif
#endif
}

/*
 * Wait for the given response from the engine.
 * Return true on success or false on failure (EOF).
 */
bool Engine::waitForResponse(const char *str) {
    bool eof = false;
    string response;
    do {
        response = getResponse(eof);
    } while (!eof && strcmp(str, response.c_str()) != 0);
    return strcmp(str, response.c_str()) == 0;
}

/*
 * Read and return a single line of response from the engine.
 * Set eof if the end of file is reached.
 *
 * Uses m_responseBuffer (a member std::string) to retain partial
 * reads across calls, replacing the old static char buffer[] for
 * thread safety and reentrancy.
 */
string Engine::getResponse(bool& eof) {
    const int MAXBUFF = 1000;
    string result;
    bool endOfLine = false;
    eof = false;
    while (!endOfLine && !eof) {
        if (m_responseBuffer.empty()) {
            // Nothing left from the previous read.
#ifdef __unix__
            char buffer[MAXBUFF + 1];
            char *readResult = fgets(buffer, MAXBUFF, fromEngine);
            if (readResult == NULL) {
                eof = true;
            } else {
                m_responseBuffer = buffer;
            }
#else
            char buffer[MAXBUFF + 1] = {};
            DWORD bytesRead;
            m_bWaitingForResponse = true;
            DWORD success = ReadFile(readFromEngine, buffer, MAXBUFF, &bytesRead, NULL);
            m_bWaitingForResponse = false;
            m_iResponseCount++;
            if (!success || bytesRead == 0) {
                eof = true;
            } else {
                buffer[bytesRead] = '\0';
                m_responseBuffer = buffer;
            }
#endif
        }
        if (!eof) {
            // Look for the end of the line in the buffer.
            size_t index = 0;
            while (index < m_responseBuffer.size() &&
                   m_responseBuffer[index] != '\n' &&
                   m_responseBuffer[index] != '\r') {
                index++;
            }
            if (index < m_responseBuffer.size() &&
                (m_responseBuffer[index] == '\n' || m_responseBuffer[index] == '\r')) {
                endOfLine = true;
                result = m_responseBuffer.substr(0, index);
                index++;
                if (index < m_responseBuffer.size() &&
                    (m_responseBuffer[index] == '\n' || m_responseBuffer[index] == '\r')) {
                    index++;
                }
                // Retain the remainder after the line terminator.
                m_responseBuffer = m_responseBuffer.substr(index);
            } else {
                // No complete line yet; take everything read so far.
                result += m_responseBuffer;
                m_responseBuffer.clear();
            }
        }
    }
#ifdef _DEBUG
    communications.append("\nEngine:\t");
    communications.append(result);
#endif
    return result;
}

#ifdef __unix__
// The unix part of startEngine method is a version of the following:
// http://stackoverflow.com/questions/478898/how-to-execute-a-command-and-get-output-of-command-within-c

// The low-level file descriptor numbers.
#define READFD 0
#define WRITEFD 1
#endif

/*
 * Start the given engine.
 */
bool Engine::startEngine(const string& engineName) {
#ifdef __unix__
    int parentToChild[2];
    int childToParent[2];
    string dataReadFromChild;

    ASSERT_IS(0, pipe(parentToChild));
    ASSERT_IS(0, pipe(childToParent));

    switch (enginePID = fork()) {
        case -1:
            FAIL("Fork failed");
            return false;

        case 0: /* Child */
            ASSERT_NOT(-1, dup2(parentToChild[ READFD ], STDIN_FILENO));
            ASSERT_NOT(-1, dup2(childToParent[ WRITEFD ], STDOUT_FILENO));
            ASSERT_IS(0, close(parentToChild [ WRITEFD ]));
            ASSERT_IS(0, close(childToParent [ READFD ]));

            execlp(engineName.c_str(), engineName.c_str(), (char *) NULL);

            cerr << "Failed to start the engine: " << engineName << endl;
            close(parentToChild[READFD]);
            close(childToParent[WRITEFD]);
            exit(-1);
            return false;

        default: /* Parent */
            ASSERT_IS(0, close(parentToChild [ READFD ]));
            ASSERT_IS(0, close(childToParent [ WRITEFD ]));

            // Set up FILE * wrappers for the file descriptors.
            fromEngine = fdopen(childToParent[READFD], "r");
            toEngine = fdopen(parentToChild[WRITEFD], "w");

            ASSERT_NOT(NULL, fromEngine);
            ASSERT_NOT(NULL, toEngine);
            return true;
    }
#else
    /* The windows version of startEngine is heavily based on the code found in the article,
     * "Creating a Child Process with Redirected Input and Output"
     * at http://msdn.microsoft.com/en-us/library/windows/desktop/ms682499%28v=vs.85%29.aspx
    */
    SECURITY_ATTRIBUTES saAttr;

    // Set the bInheritHandle flag so pipe handles are inherited. 

    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    // Create a pipe for the child process's STDOUT. 
    HANDLE unusedChildStdinRead = NULL, unusedChildStdoutWrite = NULL;

    if (!CreatePipe(&readFromEngine, &unusedChildStdoutWrite, &saAttr, 0)) {
        cerr << "Failed to create the pipe for reading from the engine." << endl;
        return false;
    }

    // Ensure the read handle to the pipe for STDOUT is not inherited.

    if (!SetHandleInformation(readFromEngine, HANDLE_FLAG_INHERIT, 0)) {
        cerr << "Failed to set up the file handle to read from the engine." << endl;
        CloseHandle(readFromEngine);
        readFromEngine = NULL;
        CloseHandle(unusedChildStdoutWrite);
        return false;
    }

    // Create a pipe for the child process's STDIN. 

    if (!CreatePipe(&unusedChildStdinRead, &writeToEngine, &saAttr, 0)) {
        cerr << "Failed to set up the file handle to write to the engine." << endl;
        CloseHandle(readFromEngine);
        readFromEngine = NULL;
        CloseHandle(unusedChildStdoutWrite);
        return false;
    }

    // Ensure the write handle to the pipe for STDIN is not inherited. 

    if (!SetHandleInformation(writeToEngine, HANDLE_FLAG_INHERIT, 0)){
        cerr << "Failed to create the pipe for writing to the engine." << endl;
        CloseHandle(writeToEngine);
        writeToEngine = NULL;
        CloseHandle(readFromEngine);
        readFromEngine = NULL;
        CloseHandle(unusedChildStdinRead);
        CloseHandle(unusedChildStdoutWrite);
        return false;
    }

    // Create the child process. 
    PROCESS_INFORMATION piProcInfo;
    STARTUPINFO siStartInfo;
    BOOL bSuccess = FALSE;

    // Set up members of the PROCESS_INFORMATION structure. 

    ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

    // Set up members of the STARTUPINFO structure. 
    // This structure specifies the STDIN and STDOUT handles for redirection.

    ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
    siStartInfo.cb = sizeof(STARTUPINFO);
    siStartInfo.hStdError = unusedChildStdoutWrite;
    siStartInfo.hStdOutput = unusedChildStdoutWrite;
    siStartInfo.hStdInput = unusedChildStdinRead;
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    // Create the child process. 
    string commandLine = "\"" + engineName + "\"";
    vector<char> commandLineBuffer(commandLine.begin(), commandLine.end());
    commandLineBuffer.push_back('\0');
    bSuccess = CreateProcessA(engineName.c_str(),
        &commandLineBuffer[0],     // command line 
        NULL,          // process security attributes 
        NULL,          // primary thread security attributes 
        TRUE,          // handles are inherited 
        0,             // creation flags 
        NULL,          // use parent's environment 
        NULL,          // use parent's current directory 
        &siStartInfo,  // STARTUPINFO pointer 
        &piProcInfo);  // receives PROCESS_INFORMATION 

    CloseHandle(unusedChildStdinRead);
    CloseHandle(unusedChildStdoutWrite);

    // If an error occurs, exit the application. 
    if (!bSuccess) {
        cerr << "Failed to create the process: " << engineName << endl;
        CloseHandle(writeToEngine);
        writeToEngine = NULL;
        CloseHandle(readFromEngine);
        readFromEngine = NULL;
        return false;
    }
    else
    {
        // Close handles to the child process and its primary thread.
        CloseHandle(piProcInfo.hThread);
        // Keep piProcInfo.hProcess for the monitor thread; will close in quitEngine.
        m_hEngineProcess = piProcInfo.hProcess;
    }

   m_bEngineClosed = false;
   m_bWaitingForResponse = false;
   m_iResponseCount = 0;
   m_responseBuffer.clear();

   SECURITY_ATTRIBUTES saThreadAttrib;
   saThreadAttrib.bInheritHandle = TRUE;
   saThreadAttrib.nLength = sizeof(SECURITY_ATTRIBUTES);
   saThreadAttrib.lpSecurityDescriptor = NULL;
   DWORD dwThreadID;
   hEngineMonitor = CreateThread(&saThreadAttrib, 0, &EngineMonitorStatic, this, 0, &dwThreadID);

#ifdef _DEBUG
   communications = "";
#endif

#endif
   return true;
}

DWORD WINAPI Engine::EngineMonitorStatic(_In_ LPVOID lpParameter)
{
    Engine* pEngine = static_cast<Engine*>(lpParameter);
    int iPrevResponseCount = 0;
    int iEngineLockedCount = 0;
    while (!pEngine->m_bEngineClosed)
    {
        if (WaitForSingleObject(pEngine->m_hEngineProcess, 5000) == WAIT_OBJECT_0)
        {
            // Sleep for two seconds in case the engine closed legitimately.
            Sleep(2000);
            break;
        }
        else if (pEngine->m_bWaitingForResponse && iPrevResponseCount == pEngine->m_iResponseCount)
        {
            // If we've been waiting for a response, wait until locked for ~100 seconds
            // before assuming unresponsive and killing the engine.
            iEngineLockedCount++;
            if (iEngineLockedCount > 20)
            {
                if (!pEngine->m_bEngineClosed)
                    pEngine->quitEngine();
            }
        }
        else
        {
            // Engine is responding normally, reset locked count.
            iEngineLockedCount = 0;
        }
        iPrevResponseCount = pEngine->m_iResponseCount;

        // Check stdin for "cancel" command.
        HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);
        DWORD dwBytesAvailable = 0;
        PeekNamedPipe(hStdIn, NULL, NULL, NULL, &dwBytesAvailable, NULL);
        if (dwBytesAvailable > 0)
        {
            char buffer[500] = "";
            DWORD dwBytesToRead = (dwBytesAvailable > 500) ? 500 : dwBytesAvailable;
            DWORD bytesRead;
            DWORD success = ReadFile(hStdIn, buffer, dwBytesToRead, &bytesRead, NULL);
            if (success && strstr(buffer, "cancel") != NULL)
            {
                if (!pEngine->m_bEngineClosed)
                    TerminateProcess(pEngine->m_hEngineProcess, 0);
                ExitProcess(0);
            }
        }
    }
    // Close the engine process handle now owned by this thread
    // (if quitEngine hasn't already closed it).
    if (pEngine->m_hEngineProcess != NULL) {
        CloseHandle(pEngine->m_hEngineProcess);
        pEngine->m_hEngineProcess = NULL;
    }
    if (!pEngine->m_bEngineClosed)
    {
        // Engine closed unexpectedly; exit process instead of hanging.
        ExitProcess(1);
    }
    return 0;
}
