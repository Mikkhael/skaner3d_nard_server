#pragma once

#include <udp_diag.hpp>

#include <vector>
#include <thread>

class UI{
    
    std::optional<std::thread> threadOptional;
    
    asio::io_context& ioContext;
    
    bool stopped = false;
    
    
    template<class Protocol>
    static typename Protocol::endpoint cinEndpoint(){
        std::string address;
        unsigned short port;
        std::cin >> address >> port;
        return typename Protocol::endpoint{asio::ip::address_v4::from_string(address), port};
    }
    

    struct MenuEntry{
        std::string activator;
        std::string name;
        std::function<void()> handler;
    };
    
    std::vector<MenuEntry> menuEntries{
        MenuEntry{"ds", "Start UDP DIAG", [this](){
            if(!udpDiagService.start(ioContext)){
                logger.logErrorLine("Cannot start UDP DIAG Client: ", udpDiagService.getError());
            }
            else{
                logger.logInfoLine("Started UDP DIAG Client on port ", udpDiagService.server().getPort());
            }
        }},
        MenuEntry{"dx", "Stop UDP DIAG", [this](){
            udpDiagService.stop();
            logger.logInfoLine("Stopped UDP DAIG service");
        }},
        MenuEntry{"dp", "Send UDP DIAG PING | <address> <port>", [this](){
            auto endpoint = cinEndpoint<udp>();
            if(!udpDiagService.sendPingRequest(endpoint)){
                logger.logErrorLine("Error: ", udpDiagService.getError());
            }
        }},
        MenuEntry{"de", "Send UDP DIAG ECHO | <address> <port> <message>", [this](){
            auto endpoint = cinEndpoint<udp>();
            std::string message;
            while(std::cin.peek() == ' ' || std::cin.peek() == '\n')
                std::cin.get();
            std::getline(std::cin, message);
            if(!udpDiagService.sendEchoRequest(endpoint, message)){
                logger.logErrorLine("Error: ", udpDiagService.getError());
            }
        }},
        MenuEntry{"dm", "Send UDP DIAG ECHO | <address> <port> <op1> <op2>", [this](){
            auto endpoint = cinEndpoint<udp>();
            int op1, op2;
            std::cin >> op1 >> op2;
            if(!udpDiagService.sendMultRequest(endpoint, op1, op2)){
                logger.logErrorLine("Error: ", udpDiagService.getError());
            }
        }},
        
        MenuEntry{"q", "Quit", [this]{stopped = true;}}
    };
    
    void showMenu(){
        logger.lockMutex([this]{
            std::cout << "=== MENU ===========\n";
            for(const auto& entry : menuEntries){
                std::cout << entry.activator << "\t- " << entry.name << '\n';
            }
            std::cout << "====================\n";
        });
    }
    
    bool getAndPerformOption(){
        std::string option;
        std::cin >> option;
        
        for(auto& entry : menuEntries){
            if(option == entry.activator){
                entry.handler();
                return true;
            }
        }
        return false;
    }
    
    
public:
    
    template<typename T>
    void start(T& workGuard){
        stopped = false;
        threadOptional.emplace([this, &workGuard]{
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            while(!stopped){
                showMenu();
                if(!getAndPerformOption()){
                    std::cout << "Unrecognized command.\n";
                }
            }
            udpDiagService.stop();
            workGuard.reset();
        });
    }
    
    void join(){
        if(threadOptional && threadOptional->joinable())
            threadOptional->join();
    }
    
    UI(asio::io_context& ioContext)
        : ioContext(ioContext)
    {}
    
};