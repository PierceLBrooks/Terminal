#pragma once

#include <cstdlib>
#include <cstddef>
#include <string>
#include <atomic>

#include <SFML/System.hpp>
#include <SFML/Network.hpp>


// Derivatives should have a construcor that does all the init work
class Frontend :public sf::NonCopyable {
public:

    static constexpr std::size_t npos = static_cast<size_t>(-1);

public:

	Frontend() {}
	virtual ~Frontend() {}

	virtual void stop() { running = false; }

	bool isRunning() { return running; }

public:

	// This function blocks; return value of 0 means failure
	// Optional; return value of npos means unimplemented
	virtual std::size_t read(void* data, std::size_t maxlen) { return npos; }

	// This function does not block; return value of 0 means empty buffer
	virtual std::size_t tryRead(void* data, std::size_t maxlen) = 0;
	virtual std::size_t getBufferedSize() = 0;

	// This function should never block
	// Return false means failure
	virtual bool write(const void* data, std::size_t len) = 0;

public:

	virtual void resizeTerminal(int rows, int cols) {}

protected:

	std::atomic_bool running;

};
