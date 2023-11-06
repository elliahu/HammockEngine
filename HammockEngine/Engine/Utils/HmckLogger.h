#pragma once

#include <string>
#include <iostream>
#include <chrono>
#include <sstream>

namespace Hmck
{

	class Logger
	{
		enum LogLevel
		{
			HMCK_LOG_LEVEL_DEBUG,
			HMCK_LOG_LEVEL_WARN,
			HMCK_LOG_LEVEL_ERROR
		};

	public:
		Logger();
		~Logger();

		static LogLevel hmckMinLogLevel;

		static void log(LogLevel level, std::string message);
		static void debug(std::string message);
		static void warn(std::string message);
		static void error(std::string message);

	private:

		static std::string createLogMessage(LogLevel level, std::string message);
	};
}