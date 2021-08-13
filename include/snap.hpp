#pragma once
#include <string>
#include <mutex>
#include <fstream>

class Snapper{
    
    //std::mutex mutex;
    std::string snapPath;
    
    #ifdef FAKECAMERA
    
    std::string framesPath;
    int framesCount = 1;
    int frameDuration = 1;
    
    #endif // FAKECAMERA
    
public:
    
    #ifdef FAKECAMERA
    
    void setFakeCameraConfig(const std::string& framesPath, int framesCount, int frameDuration){
        this->framesPath = framesPath;
        this->framesCount = framesCount;
        this->frameDuration = frameDuration;
    }
    
    #endif // FAKECAMERA
    
    void setSnapPath(const std::string& path){
        snapPath = path;
    }
    std::string getSnapFilePath(){
        return snapPath;
    }
    
    template<typename Callback> 
    void snap(Callback callback){
        
        #ifdef FAKECAMERA
        
        //std::lock_guard<std::mutex> lock{mutex};
        std::ofstream snapFile(snapPath, std::fstream::trunc | std::fstream::out | std::fstream::binary);
        //std::cout << "Snap path: " << snapPath << std::endl;
        if(!snapFile.is_open() || snapFile.fail()){
            callback(false, "Cannot open Snap File");
            return;
        }
        
        auto time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
        int frame = (time % (frameDuration * framesCount)) / frameDuration;
        
        std::string framePath = framesPath;
        framePath += std::to_string(frame);
        
        std::ifstream frameFile(framePath, std::fstream::in | std::fstream::binary);
        //std::cout << "Frame path: " << framePath << std::endl;
        if(!frameFile.is_open() || frameFile.fail()){
            callback(false, "Cannot open Frame File");
            return;
        }
        
        snapFile << frameFile.rdbuf();
        
        if(!snapFile || !frameFile){
            callback(false, "Unsucessful copy");
        }
        snapFile.flush();
        
        callback(true, "");
        
        #else
        
        callback(false, "");
        
        #endif // FAKECAMERA
        
    }
    
};

extern Snapper snapper;