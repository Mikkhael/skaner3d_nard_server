#pragma once

#include <fstream>
#include <filesystem>
#include <unordered_map>
#include <string>
#include <optional>

namespace fs = std::filesystem;

class Config{
    
    std::unordered_map<std::string, std::string> raw_options;
    
    class ParsedOptions{
        #ifdef WIN32
            const char* DEV_NULL = "NUL";
        #else
            const char* DEV_NULL = "/dev/null";
        #endif // WIN32
        
    public: 
        fs::path infoLogFilePath  = DEV_NULL;
        fs::path errorLogFilePath = DEV_NULL;
        int threads = 1;
    };
    
public:

    ParsedOptions options;
    std::string optionsParsingError;
    
    bool loadConfigFile(const fs::path path){
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
    
    std::optional<std::string> getOption(const std::string& option){
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
        }
        
        optionsParsingError = std::move(parsingErrors);
        return good;
    }
    
    auto& listOptions(std::ostream& os) const {
        for(const auto& [key, value] : raw_options){
            os << key << "=" << value << '\n';
        }
        return os;
    }
};

inline Config config;