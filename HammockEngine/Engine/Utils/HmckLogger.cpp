#include "HmckLogger.h"
#pragma once

Hmck::Logger::Logger(){}

Hmck::Logger::~Logger(){}

Hmck::Logger::LogLevel Hmck::Logger::hmckMinLogLevel = Hmck::Logger::HMCK_LOG_LEVEL_DEBUG;

void Hmck::Logger::log(LogLevel level, std::string message)
{
    if(level >= hmckMinLogLevel)
        std::cout << createLogMessage(level, message) << std::endl;
}

void Hmck::Logger::debug(std::string message)
{
    log(LogLevel::HMCK_LOG_LEVEL_DEBUG, message);
}

void Hmck::Logger::warn(std::string message)
{
    log(LogLevel::HMCK_LOG_LEVEL_WARN, message);
}

void Hmck::Logger::error(std::string message)
{
    log(LogLevel::HMCK_LOG_LEVEL_ERROR, message);
}

std::string Hmck::Logger::createLogMessage(LogLevel level, std::string message)
{
    std::time_t t = std::time(nullptr);
    std::stringstream ss;
    ss << t;
    std::string ts = ss.str();

    std::string lvl = "";

    switch (level)
    {
    case Hmck::Logger::HMCK_LOG_LEVEL_DEBUG:
        lvl = "DEBUG";
        break;
    case Hmck::Logger::HMCK_LOG_LEVEL_WARN:
        lvl = "WARN";
        break;
    case Hmck::Logger::HMCK_LOG_LEVEL_ERROR:
        lvl = "ERROR";
        break;
    default:
        lvl = "LOG";
        break;
    }

    std::string logMessage = "[" + lvl + std::string("]:") + "(" + ts + ")" + " - " + message;
    return logMessage;
}
