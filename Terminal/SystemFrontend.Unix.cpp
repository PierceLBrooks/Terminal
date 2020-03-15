
#include "SystemFrontend.hpp"

using namespace std;

#ifndef SFML_SYSTEM_WINDOWS

SystemFrontend::SystemFrontend(const string& shell, int rows, int cols) {
	struct winsize ws;
	ws.ws_row = rows;
	ws.ws_col = cols;
	int ret = forkpty(&pty, 0, 0, &ws);
	if (ret == 0) {
		setenv("TERM", "xterm-256color", 1);
		execlp(shell.c_str(), shell.c_str(), NULL);
	} else if (ret == -1) {
		fprintf(stderr, "Unix SystemFrontend::Constructor: forkpty() failed\n");
		return;
	} else {
		child = ret;
		printf("childid = %d\n", child);
	}

	thReader = new thread(
		[](int child, atomic_bool& running) {
			int stat;
			waitpid(child, &stat, 0);
			running = false;
		}
	, child, ref(running));

	running = true;
}


SystemFrontend::~SystemFrontend() {
	close(pty);
	if (running) {
		kill(child, SIGTERM);
		waitpid(child, NULL, 0);
	}
	if (thReader) {
		if (thReader->joinable())
			thReader->join();
		delete thReader;
	}
}


size_t SystemFrontend::read(void* data, size_t len) {
	ssize_t ret = ::read(pty, data, len);
	if (ret == -1)
		return 0;
	else
		return ret;
}


bool SystemFrontend::write(const void* data, size_t len) {
	return ::write(pty, data, len) != -1;
}


void SystemFrontend::resizeTerminal(int rows, int cols) {
	struct winsize ws;
	ws.ws_row = rows;
	ws.ws_col = cols;
	ioctl(pty, TIOCSWINSZ, &ws);
}


#endif // ifndef SFML_SYSTEM_WINDOWS
