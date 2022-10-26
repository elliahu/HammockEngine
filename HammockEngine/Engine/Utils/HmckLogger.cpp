#include "HmckLogger.h"
#pragma once

Hmck::HmckLogger::HmckLogger(){}

Hmck::HmckLogger::~HmckLogger(){}

void Hmck::HmckLogger::log(HmckLogLevel level, std::string message)
{
    std::cout << createLogMessage(level, message) << std::endl;
}

void Hmck::HmckLogger::debug(std::string message)
{
    log(HmckLogLevel::HMCK_LOG_LEVEL_DEBUG, message);
}

void Hmck::HmckLogger::warn(std::string message)
{
    log(HmckLogLevel::HMCK_LOG_LEVEL_WARN, message);
}

void Hmck::HmckLogger::error(std::string message)
{
    log(HmckLogLevel::HMCK_LOG_LEVEL_ERROR, message);
}

std::string Hmck::HmckLogger::createLogMessage(HmckLogLevel level, std::string message)
{
    std::time_t t = std::time(nullptr);
    std::stringstream ss;
    ss << t;
    std::string ts = ss.str();

    std::string lvl = "";

    switch (level)
    {
    case Hmck::HMCK_LOG_LEVEL_DEBUG:
        lvl = "DEBUG";
        break;
    case Hmck::HMCK_LOG_LEVEL_WARN:
        lvl = "WARN";
        break;
    case Hmck::HMCK_LOG_LEVEL_ERROR:
        lvl = "ERROR";
        break;
    default:
        lvl = "LOG";
        break;
    }

    std::string logMessage = "[" + lvl + std::string("]:") + "(" + ts + ")" + " - " + message;
    return logMessage;
}
