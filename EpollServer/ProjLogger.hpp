#include "Logger.hpp"
#include <iostream>

using Logger = util::TsSingletonLogger<util::StreamLogger>;
using LogLevel = util::Logger::LogLevel;

void initLogger(LogLevel lvl);

#define Log (Logger::get())