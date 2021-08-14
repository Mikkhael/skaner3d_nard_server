#include <chrono>
#include <iostream>
#include <vector>
#include <thread>
#include <clocale>

#include <config.hpp>
#include <udp_diag.hpp>
#include <tcp_transmission.hpp>
#include <snap.hpp>
#include <systemManager.hpp>

#ifdef CLIENT
    #include <ui.hpp>
#endif // CLIENT

int main(const int argc, const char** argv)
{
try{        
    // Setup
    
    std::setlocale(0, "");
    
    asio::io_context ioContext;
    asio::steady_timer timer(ioContext);
    
    ioContext.stop();
    
    // Config
    
    config.parseCommandLineOptions(argc, argv); 
    auto configPath = config.getOption("configPath");
    if(configPath){
        //std::cout << "Loading config path: " << std::string(*config_path) << '\n';
        config.loadConfigFile(*configPath);
    }
    
    if(!config.parseOptions()){
        std::cerr << "Error whiel parsing config options: \n" << config.optionsParsingError << '\n';
        return -1;
    }
    
    //config.listOptions(std::cout);
    
    // Logger
    
    logger.setIoContext(ioContext);
    if(!logger.setLogFiles(config.options.infoLogFilePath, config.options.errorLogFilePath)){
        std::cerr << "Cannot open logging files: \n " << config.options.infoLogFilePath << "\n " << config.options.errorLogFilePath << '\n';
        return -1;
    }
    
    // Snapper
    
    snapper.setConfig(config.options.snapStreamPath, config.options.snapPrefix);
    #ifdef FAKECAMERA
    snapper.setFakeCameraConfig(config.options.fake_framesPath, config.options.fake_framesCount, config.options.fake_frameDuration);
    #endif // FAKECAMERA
    
    // System
    
    systemManager.setConfig(ioContext, config.options.ipSettingsPath, config.options.ipBootSettingsPath, config.options.customSettingsPath);
    
    // Threads
    
    const int additionalThreads = config.options.threads-1;
    std::vector<std::thread> threadpool;
    threadpool.reserve(additionalThreads);
    
    
    // Servers and Clients
    
    #ifdef SERVER
    
    udpDiagService.setContext(ioContext);
    tcpTransServerService.setContext(ioContext);
    
    if(!udpDiagService.start(config.options.udpDiagPort)){
        logger.logErrorLine("Cannot start UDP DIAG Server: ", udpDiagService.getError());
        return -1;
    }
    logger.logInfoLine("Started UDP DIAG Server on port ", udpDiagService.server().getPort());
    
    if(!tcpTransServerService.start(config.options.tcpTransPort)){
        logger.logErrorLine("Cannot start TCP TRANS Server: ", tcpTransServerService.getError());
        return -1;
    }
    logger.logInfoLine("Started TCP TRANS Server on port ", tcpTransServerService.server().getPort());
    
    #endif // SERVER
    #ifdef CLIENT
    
    asio::executor_work_guard<asio::io_context::executor_type> workGuard(ioContext.get_executor());
    UI userInterface(ioContext);
    userInterface.start(workGuard);
    
    #endif // CLIENT
    
    
    // Running ioContext
    
    ioContext.restart();
    
    for(int i=0; i<additionalThreads; i++){
        threadpool.emplace_back([&ioContext, i]{
            try{
                logger.logInfoLine("Starting thread ", std::this_thread::get_id());
				ioContext.run();
			}  catch(std::exception& e)	{
                logger.logErrorLine("Cought exception in thread ", std::this_thread::get_id(), '\n', e.what());
			}
            logger.logInfoLine("Closing thread ", std::this_thread::get_id());
        });
    }
    
    try{
        logger.logInfoLine("Starting thread (Main) ", std::this_thread::get_id());
        ioContext.run();
    }  catch(std::exception& e)	{
        logger.logErrorLine("Cought exception in thread (Main) ", std::this_thread::get_id(), '\n', e.what());
    }
    logger.logInfoLine("Closing thread (Main) ", std::this_thread::get_id());
    
    for(auto& t : threadpool){
        if(t.joinable())
            t.join();
    }
    
    #ifdef CLIENT
        
        userInterface.join();
        
    #endif // CLIENT
    
    
    

}catch(std::exception& e){
    std::cout << "!! Uncought exception !! : " << e.what() << std::endl;
}
    
    if(systemManager.shouldReboot){
        std::cout << "Issueing reboot...\n";
        #ifdef NARD
            system("reboot");
        #else
            system("echo Rebooting test");
        #endif // NARD
    }
    
    return 0;
}