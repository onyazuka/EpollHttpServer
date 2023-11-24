#include "ProjLogger.hpp"

void initLogger(LogLevel lvl) {
	std::unique_ptr<util::StreamLogger> pslog = std::make_unique<util::StreamLogger>(lvl, util::Logger::Options(true, true, true, true, true), std::cout);
	Logger::init(std::move(pslog));
}