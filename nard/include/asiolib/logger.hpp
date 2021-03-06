#pragma once

#include "utils.hpp"
#include "fold.hpp"
#include <chrono>
#include <optional.hpp>
#include <fstream>
#include <mutex>

#include <ctime>
#include <iomanip>
#include <iostream>

class Logger
{
    
    asio::io_context* ioContextPtr;
    boost::optional<asio::io_context::strand> ioStrand;
    
    std::ofstream infoLogFile;
    std::ofstream errorLogFile;
    
    
    std::mutex offlineWriteMutex;
    
    template <class ...T>
    bool writeToStream(std::ostream& stream, std::ostream& debug_stream, T&& ...args){
        // if(!stream.is_open() || stream.fail())	{
		// 	return false;
		// }
        
        // const std::lock_guard<std::mutex> lock{offlineWriteMutex};
        // fold_ostream(stream, std::forward<T>(args)...);
        // stream.flush(); // Possibly delete
        
        // #ifdef DEBUG
        //     fold_ostream(std::cout, std::forward<T>(args)...);
        // #endif //DEBUG
        
        if(ioStrand->context().stopped()){
            const std::lock_guard<std::mutex> lock{offlineWriteMutex};
            fold_ostream(stream, std::forward<T>(args)...);
            stream.flush(); // Possibly delete
            
            #ifdef DEBUG
                fold_ostream(debug_stream, std::forward<T>(args)...);
            #endif //DEBUG
        }else{
            asio::post(*ioStrand, [&stream, &debug_stream, args...]{ 
                fold_ostream(stream, args...);
                stream.flush(); // Possibly delete
                
                #ifdef DEBUG
                    fold_ostream(debug_stream, args...);
                #endif //DEBUG
            });
        }
		return true;
    }
    
public:
    
    template <typename T>
    void lockMutex(T&& f){
        const std::lock_guard<std::mutex> lock{offlineWriteMutex};
        f();
    }
    
	auto getTimestamp() { 
        const auto time_now = std::time(nullptr);
        return std::put_time(std::localtime(&time_now), "%d.%m.%Y %H:%M:%S");
        //return time_now;
    }
    
    auto& getIoContext() const { return *ioContextPtr; }
    auto& getIoStrand() const {return *ioStrand; }
    void setIoContext(asio::io_context& ioContext){ ioContextPtr = &ioContext; ioStrand.emplace(ioContext); }
    
    bool setLogFiles (const std::string& infoLogFilePath, const std::string& errorLogFilePath){
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
    auto logInfoLine(T&& ...args) { return writeToStream(infoLogFile, std::cout, '[', getTimestamp(), "] ",  args..., '\n'); }
    
    template <class ...T>
    auto logErrorLine(T&& ...args){ return writeToStream(errorLogFile, std::cerr, '[', getTimestamp(), "]E ",  args..., '\n'); }
    
};

extern Logger logger;