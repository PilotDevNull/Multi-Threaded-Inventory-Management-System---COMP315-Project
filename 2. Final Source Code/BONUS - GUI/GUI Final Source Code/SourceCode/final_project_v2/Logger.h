#ifndef LOGGER_H
#define LOGGER_H

#include <mutex>
#include <iostream>

/*

Global mutex used by all threads for console output

Prevents messed up and poorly formated output when multiple threads print simultaneously

'inline' ensures only one instance exists across all translation units

Very useful for debugging

*/

inline std::mutex printMutex;

//Convenience macro, locks, prints, unlocks atomically
#define LOG(msg) { std::lock_guard<std::mutex> _log_lock(printMutex); std::cout << msg << "\n"; }

#endif
