#pragma once
#include <string>
#include <mutex>
#include <fstream>
#include <sstream>
#include <cstdlib>
class Snapper{
    
    //std::mutex mutex;
    std::string snapDirectory;
    
    #ifdef FAKECAMERA
    
    std::string framesPath;
    int framesCount = 1;
    int frameDuration = 1;
    
    #endif // FAKECAMERA
    
    
    template<typename Callback>
    void snap_impl(Callback callback, std::ostream& destStream){
        #ifdef FAKECAMERA
        
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
        
        destStream << frameFile.rdbuf();
        
        if(!destStream || !frameFile){
            callback(false, "Unsucessful copy");
            return;
        }
        destStream.flush();
        
        callback(true, "");
        
        #else
        
        callback(false, "");
        
        #endif // FAKECAMERA
    }
    
public:
    
    #ifdef FAKECAMERA
    
    void setFakeCameraConfig(const std::string& framesPath, int framesCount, int frameDuration){
        this->framesPath = framesPath;
        this->framesCount = framesCount;
        this->frameDuration = frameDuration;
    }
    
    #endif // FAKECAMERA
    
    void setConfig(const std::string& snapDirectory){
        this->snapDirectory = snapDirectory;
    }
    std::string getSnapFilePathForId(uint32_t seriesid){
        return snapDirectory + "/snap_" + std::to_string(seriesid);
    }
    
    template<typename Callback, typename Ostream> 
    void snapStream(Ostream& ostream, Callback&& callback){
        snap_impl(callback, ostream);
    }
    template<typename Callback> 
    void snap(Callback&& callback, uint32_t seriesid){
        std::string snapFilePath = getSnapFilePathForId(seriesid);
        std::ofstream snapFile(snapFilePath, std::fstream::trunc | std::fstream::out | std::fstream::binary);
        if(!snapFile.is_open() || snapFile.fail()){
            callback(false, "Cannot open Snap File");
            return;
        }
        snap_impl(callback, snapFile);
    }
    
    
    
    void deleteAllSnaps(){
        #ifdef WINDOWS
            std::string fixedDirectory = snapDirectory;
            for(auto& c : fixedDirectory){
                if(c == '/'){
                    c = '\\';
                }
            }
            std::string command = "rmdir /S /Q ";
            command += fixedDirectory;
            command += " & mkdir ";
            command += fixedDirectory;
        #else
            std::string command = "rm -r ";
            command += snapDirectory;
            command += "/*";
        #endif // WINDOWS
        //std::cout << "C: " << command << std::endl;
        std::system(command.c_str());
    }
};

extern Snapper snapper;