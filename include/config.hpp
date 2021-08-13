#pragma once

#include <fstream>
//#include <filesystem>
#include <unordered_map>
#include <string>
#include <optional.hpp>

//namespace fs = std::filesystem;

class Config{
    
    std::unordered_map<std::string, std::string> raw_options;
    
    class ParsedOptions{
        #ifdef WINDOWS
            const char* DEV_NULL = "NUL";
        #else
            const char* DEV_NULL = "/dev/null";
        #endif // WINDOWS
        
    public: 
        std::string infoLogFilePath  = DEV_NULL;
        std::string errorLogFilePath = DEV_NULL;
        int threads = 1;
        int udpDiagPort = 1234;
        int tcpTransPort = 1234;
        std::string snapPath = DEV_NULL;
        std::string fake_framesPath = DEV_NULL;
        int fake_framesCount = 1;
        int fake_frameDuration = 1;
    };
    
public:

    ParsedOptions options;
    std::string optionsParsingError;
    
    bool loadConfigFile(const std::string path){
        std::ifstream file(path);
        if(!file.is_open()){
            return false;
        }
        std::string option;
        std::string value;
        while(!file.eof()){
            std::getline(file, option, '=');
            std::getline(file, value, '\n');
            raw_options[option] = value;
        }
        return true;
    }
    
    void setOption(const std::string& option, const std::string& value){
        raw_options[option] = value;
    }
    void setOptionNoOvverride(const std::string& option, const std::string& value){
        raw_options.insert({option, value});
    }
    
    boost::optional<std::string> getOption(const std::string& option){
        const auto it = raw_options.find(option);
        if(it == raw_options.end())
            return {};
        return it->second;
    }
    
    void parseCommandLineOptions(const int argc, const char** argv){
        for(int i=1; i<argc; i++){
            const std::string str = argv[i];
            if(str.size() < 3 || str[0] != '-' || str[1] != '-'){
                continue;
            }
            const auto itpos = str.find('=');
            if(itpos != std::string::npos){
                const auto it = str.begin() + itpos;
                setOptionNoOvverride(std::string(str.begin() + 2, it), std::string(it+1, str.end()));
            } else {
                setOptionNoOvverride(std::string(str.begin() + 2, str.end()), "");
            }
        }
    }
    
    bool parseOptions(){
        
        std::string parsingErrors;
        bool good = true;
        
        {
            const auto temp = getOption("infoLogFilePath");
            if(temp){
                options.infoLogFilePath = *temp;
            }
        }{
            const auto temp = getOption("errorLogFilePath");
            if(temp){
                options.errorLogFilePath = *temp;
            }
        }{
            const auto temp = getOption("threads");
            if(temp){
                try{
                    options.threads = std::stoi(*temp);
                    if(options.threads < 1){
                        good = false;
                        parsingErrors += "Threads '";
                        parsingErrors += std::to_string(options.threads);
                        parsingErrors += "' cannot be less then 1";
                    }
                }
                catch(...){
                    good = false;
                    parsingErrors += "Cannot parse the value '";
                    parsingErrors += *temp;
                    parsingErrors += "' of threads";
                }
            }
        }{
            const auto temp = getOption("udpDiagPort");
            if(temp){
                try{
                    options.udpDiagPort = std::stoi(*temp);
                }
                catch(...){
                    good = false;
                    parsingErrors += "Cannot parse the value '";
                    parsingErrors += *temp;
                    parsingErrors += "' of udpDiagPort";
                }
            }
        }{
            const auto temp = getOption("tcpTransPort");
            if(temp){
                try{
                    options.tcpTransPort = std::stoi(*temp);
                }
                catch(...){
                    good = false;
                    parsingErrors += "Cannot parse the value '";
                    parsingErrors += *temp;
                    parsingErrors += "' of tcpTransPort";
                }
            }
        }{
            const auto temp = getOption("snapPath");
            if(temp){
                options.snapPath = *temp;
            }
        }{
            const auto temp = getOption("framesPath");
            if(temp){
                options.fake_framesPath = *temp;
            }
        }{
            const auto temp = getOption("framesCount");
            if(temp){
                try{
                    options.fake_framesCount = std::stoi(*temp);
                }
                catch(...){
                    good = false;
                    parsingErrors += "Cannot parse the value '";
                    parsingErrors += *temp;
                    parsingErrors += "' of framesCount";
                }
            }
        }{
            const auto temp = getOption("frameDuration");
            if(temp){
                try{
                    options.fake_frameDuration = std::stoi(*temp);
                }
                catch(...){
                    good = false;
                    parsingErrors += "Cannot parse the value '";
                    parsingErrors += *temp;
                    parsingErrors += "' of frameDuration";
                }
            }
        }
        
        optionsParsingError = std::move(parsingErrors);
        return good;
    }
    
    auto& listOptions(std::ostream& os) const {
        for(const auto& o : raw_options){
            os << o.first << "=" << o.second << '\n';
        }
        return os;
    }
};

extern Config config;