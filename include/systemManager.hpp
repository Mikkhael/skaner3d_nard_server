#pragma once
#include <string>
#include <asiolib/utils.hpp>

class SystemManager{
    
    asio::io_context *ioContext;
    std::string ipSettingsPath;
    std::string ipBootSettingsPath;
    std::string customSettingsPath;
    
public:
    bool shouldReboot = false;
    
    void setConfig(asio::io_context& ioCotext,  std::string& ipSettingsPath, const std::string& ipBootSettingsPath, const std::string& cusotmSettingsPath){
        this->ioContext = ioContext;
        this->ipSettingsPath = ipSettingsPath;
        this->ipBootSettingsPath = ipBootSettingsPath;
        this->customSettingsPath = cusotmSettingsPath;
    }

    bool setNetworkSettings(std::string ip, std::string mask, std::string gateway, std::string dns1, std::string dns2, bool isDynamic);
    bool setDeviceConfig(std::string config);
    std::string getDeviceConfig();
    void reboot();
    
};


extern SystemManager systemManager;