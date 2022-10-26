#pragma once

#include <string>
#include <iostream>
#include <chrono>
#include <sstream>

namespace Hmck
{
	enum HmckLogLevel
	{
		HMCK_LOG_LEVEL_DEBUG,
		HMCK_LOG_LEVEL_WARN,
		HMCK_LOG_LEVEL_ERROR
	};

	class HmckLogger
	{
	public:
		HmckLogger();
		~HmckLogger();

		static HmckLogLevel hmckMinLogLevel;

		static void log(HmckLogLevel level, std::string message);
		static void debug(std::string message);
		static void warn(std::string message);
		static void error(std::string message);

	private:

		static std::string createLogMessage(HmckLogLevel level, std::string message);
	};
}