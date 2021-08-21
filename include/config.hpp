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
        
        std::string snapStreamPath = DEV_NULL;
        std::string fake_framesPath = DEV_NULL;
        int fake_framesCount = 1;
        int fake_frameDuration = 1;
        
        std::string snapDirectory = DEV_NULL;
        
        std::string ipSettingsPath = DEV_NULL;
        std::string ipBootSettingsPath = DEV_NULL;
        std::string customSettingsPath = DEV_NULL;
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
    
    bool loadOptionAsInt(const std::string& name, std::string& errors, int& res){
        const auto temp = getOption(name);
        if(temp){
            try{
                res = std::stoi(*temp);
            }
            catch(...){
                errors += "Cannot parse the value '";
                errors += *temp;
                errors += "' of ";
                errors += name;
                errors += '\n';
                return false;
            }
        }
        return true;
    }
    bool loadOptionAsString(const std::string& name, std::string& errors, std::string& res){
        const auto temp = getOption(name);
        if(temp){
            res = *temp;
        }
        return true;
    }
    
    bool parseOptions(){
        bool good = true;
        good = good && loadOptionAsString("infoLogFilePath", optionsParsingError, options.infoLogFilePath);
        good = good && loadOptionAsString("errorLogFilePath", optionsParsingError, options.errorLogFilePath);
        good = good && loadOptionAsInt("threads", optionsParsingError, options.threads);
        good = good && loadOptionAsInt("udpDiagPort", optionsParsingError, options.udpDiagPort);
        good = good && loadOptionAsInt("tcpTransPort", optionsParsingError, options.tcpTransPort);
        good = good && loadOptionAsString("snapStreamPath", optionsParsingError, options.snapStreamPath);
        good = good && loadOptionAsString("snapDirectory", optionsParsingError, options.snapDirectory);
        good = good && loadOptionAsString("framesPath", optionsParsingError, options.fake_framesPath);
        good = good && loadOptionAsInt("framesCount", optionsParsingError, options.fake_framesCount);
        good = good && loadOptionAsInt("frameDuration", optionsParsingError, options.fake_frameDuration);
        good = good && loadOptionAsString("ipSettingsPath", optionsParsingError, options.ipSettingsPath);
        good = good && loadOptionAsString("ipBootSettingsPath", optionsParsingError, options.ipBootSettingsPath);
        good = good && loadOptionAsString("customSettingsPath", optionsParsingError, options.customSettingsPath);
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