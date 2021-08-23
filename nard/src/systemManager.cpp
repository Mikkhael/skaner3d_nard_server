#include <systemManager.hpp>
#include <string>
#include <sstream>
#include <fstream>
#include <tcp_transmission.hpp>
#include <udp_diag.hpp>

bool SystemManager::setNetworkSettings(std::string ip, std::string mask, std::string gateway, std::string dns1, std::string dns2, bool isDynamic){
    std::fstream file(ipSettingsPath, std::fstream::app | std::fstream::out);
    if(!file.is_open() || file.fail()){
        logger.logErrorLine("Cannot open network settings file with path: ", ipSettingsPath);
        return false;
    }
    
    file << "eth0ip0=" << ip << '\n';
    file << "eth0mask0=" << mask << '\n';
    file << "eth0defgw=" << gateway << '\n';
    file << "eth0dns1=" << dns1 << '\n';
    file << "eth0dns2=" << dns2 << '\n';
    file << "eth0proto=" << (isDynamic ? "dynamic" : "static") << '\n';
    
    if(!file){
        logger.logErrorLine("Error while writing to network settings file with path: ", ipSettingsPath);
        return false;
    }
    
    file.close();
    file.open(ipSettingsPath, std::fstream::binary | std::fstream::in);
    if(!file.is_open() || file.fail()){
        logger.logErrorLine("Cannot open network settings file with path: ", ipSettingsPath);
        return false;
    }
    std::ofstream bootFile(ipBootSettingsPath, std::fstream::binary | std::fstream::trunc | std::fstream::out);
    if(!bootFile.is_open() || bootFile.fail()){
        logger.logErrorLine("Cannot open BOOT network settings file with path: ", ipBootSettingsPath);
        return false;
    }
    bootFile << file.rdbuf();
    return true;
}
bool SystemManager::setDeviceConfig(std::string config){
    std::ofstream file(customSettingsPath, std::fstream::trunc | std::fstream::out);
    if(!file.is_open() || file.fail()){
        logger.logErrorLine("Cannot open device config file with path: ", config);
        return false;
    }
    file << config;
    if(!file.is_open() || file.fail()){
        logger.logErrorLine("Cannot write config to file with path: ", config);
        return false;
    }
    return true;
}
std::string SystemManager::getDeviceConfig(){
    std::ifstream file(customSettingsPath, std::fstream::in);
    if(!file.is_open() || file.fail()){
        return "";
    }
    std::stringstream res;
    res << file.rdbuf();
    return res.str();
}

void SystemManager::reboot(){
    shouldReboot = true;
    tcpTransServerService.stop();
    udpDiagService.stop();
    
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    if(ioContext)
        ioContext->stop();
}


SystemManager systemManager;