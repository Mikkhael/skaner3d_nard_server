#pragma once
#include <string>
#include <mutex>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;
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
            callback(false, std::string("Cannot open Frame File with path: ") + framePath);
            return;
        }
        
        destStream << frameFile.rdbuf();
        
        if(!destStream || !frameFile){
            callback(false, std::string("Unsucessful copy of Frame File: ") + framePath);
            frameFile.close();
            return;
        }
        destStream.flush();
        frameFile.close();
        
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
        fs::create_directories(snapDirectory);
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
    
    template<typename Callback>
    void deleteAllSnaps(Callback callback){
        Error err;
        fs::remove_all(snapDirectory, err);
        if(err){
            callback(false, err);
            return;
        }
        fs::create_directories(snapDirectory, err);
        if(err){
            callback(false, err);
            return;
        }
        callback(true, "");
    }
    
    template<typename Callback>
    void listSnaps(Callback callback){
        Error err;
        std::vector<uint32_t> foundSeriesids;
        auto directory = fs::directory_iterator(snapDirectory, err);
        if(err){
            callback(true, err, foundSeriesids);
            return;
        }
        for(const auto entry : directory){
            std::string path = entry.path().filename().string();
            path.erase(path.begin(), path.begin() + 5);
            try{
                uint32_t seriesid = std::stoul(path);
                foundSeriesids.push_back(seriesid);
            }catch(...){
                callback(false, "Invalid file name inside snaps folder", foundSeriesids);
                return;
            }
        }
        callback(true, err, foundSeriesids);
    }
};

extern Snapper snapper;