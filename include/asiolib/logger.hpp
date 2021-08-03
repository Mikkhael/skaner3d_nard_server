#pragma once

#include "utils.hpp"
#include <chrono>
#include <optional>
#include <fstream>
#include <mutex>

#include <ctime>
#include <iomanip>

#ifdef DEBUG
    #include <iostream>
#endif

class Logger
{
    
    asio::io_context* ioContextPtr;
    std::optional<asio::io_context::strand> ioStrand;
    
    std::ofstream infoLogFile;
    std::ofstream errorLogFile;
    
    
    std::mutex offlineWriteMutex;
    
    template <class ...T>
    bool writeToStream(std::ofstream& stream, T&& ...args){
        if(!stream.is_open() || stream.fail())	{
			return false;
		}
        
        if(ioStrand->context().stopped()){
            const std::lock_guard<std::mutex> lock{offlineWriteMutex};
            (stream << "(stopped) " << ... << args);
            stream.flush(); // Possibly delete
            
            #ifdef DEBUG
                (std::cout << "(stopped) " << ... << args);
            #endif //DEBUG
        }else{
            asio::post(*ioStrand, [&stream, args...]{ 
                (stream << ... << args);
                stream.flush(); // Possibly delete
                
                #ifdef DEBUG
                    (std::cout << ... << args);
                #endif //DEBUG
            });
        }
		return true;
    }
    
public:
    
	auto getTimestamp() { 
        const auto time_now = std::time(nullptr);
        return std::put_time(std::localtime(&time_now), "%d.%m.%Y %H:%M:%S");
        //return time_now;
    }
    
    auto& getIoContext() const { return *ioContextPtr; }
    auto& getIoStrand() const {return *ioStrand; }
    void setIoContext(asio::io_context& ioContext){ ioContextPtr = &ioContext; ioStrand.emplace(ioContext); }
    
    bool setLogFiles (const fs::path& infoLogFilePath, const fs::path& errorLogFilePath){
        infoLogFile.open(infoLogFilePath, std::fstream::out | std::fstream::app);
        if(!infoLogFile.is_open()){
            return false;
        }
        errorLogFile.open(errorLogFilePath, std::fstream::out | std::fstream::app);
        if(!errorLogFile.is_open()){
            return false;
        }
        return true;
    }
    
    template <class ...T>
    auto logInfoLine(T&& ...args) { return writeToStream(infoLogFile,  '[', getTimestamp(), "] ",  args..., '\n'); }
    
    template <class ...T>
    auto logErrorLine(T&& ...args){ return writeToStream(errorLogFile, '[', getTimestamp(), "]E ",  args..., '\n'); }
    
};

inline Logger logger;