#pragma once

#include <deque>
#include <thread>
#include <mutex>
#include <condition_variable>

#include <SFML/Config.hpp>

#ifdef SFML_SYSTEM_WINDOWS
#include <windows.h>
#else
#ifdef __APPLE__
#include <util.h>
#else
#include <pty.h>
#endif
#include <errno.h>
#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <signal.h>
#endif

#include "Frontend.hpp"


class SystemFrontend :public Frontend {
public:

	SystemFrontend(const std::string& shell, int rows, int cols);
	~SystemFrontend() override;

public:

	virtual std::size_t tryRead(void* data, std::size_t maxlen) override;
	virtual std::size_t getBufferedSize() override;

	virtual bool write(const void* data, std::size_t len) override;

public:

	virtual void resizeTerminal(int rows, int cols) override;

private:

	std::deque<char> bufRead;
	std::mutex bufReadLock;
	std::thread* thReader = 0;

#ifdef SFML_SYSTEM_WINDOWS
	HANDLE childStdInPipeRead = 0, childStdInPipeWrite = 0;
	HANDLE childStdOutPipeRead = 0, childStdOutPipeWrite = 0;
	HANDLE childProcessHandle = 0;
	std::thread* processRunningChecker = 0;
#else
	int pty, child;
#endif

};

