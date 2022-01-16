#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <string>
#include <fstream>
#include <memory>
#include <sys/resource.h>
#include "easyloggingpp/easylogging++.h"

class Logger{

public:
    static std::string fileName;

    static void init(const std::string& pathToConfigure){
        el::Configurations config(pathToConfigure);
        el::Loggers::reconfigureAllLoggers(config);
    }

    static void logInfo(const std::string &&message) {
        LOG(INFO) << message;
    }
    static void logWarning(const std::string &&message) {
        LOG(WARNING) << message;
    }
    static void logError(const std::string &&message) {
        LOG(ERROR) << message;
    }
};
#endif
