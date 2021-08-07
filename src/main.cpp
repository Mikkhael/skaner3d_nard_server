#include <chrono>
#include <iostream>
#include <vector>
#include <thread>
#include <clocale>

#include <config.hpp>
#include <udp_diag.hpp>

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
        //std::cout << "Loading config path: " << fs::path(*config_path) << '\n';
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
    
    // Threads
    
    const int additionalThreads = config.options.threads-1;
    std::vector<std::thread> threadpool;
    threadpool.reserve(additionalThreads);
    
    
    // Servers and Clients
    
    #ifdef SERVER
    
    if(!udpDiagService.start(ioContext, 1234)){
        logger.logErrorLine("Cannot start UDP DIAG Server: ", udpDiagService.getError());
        return -1;
    }
    logger.logInfoLine("Started UDP DIAG Server on port ", udpDiagService.server().getPort());
    
    
    // Running ioContext
    
    #endif // SERVER
    #ifdef CLIENT
    
    //std::optional<asio::executor_work_guard<asio::io_context::executor_type> > workGuard;
    //workGuard.emplace(ioContext.get_executor());
    
    asio::executor_work_guard<asio::io_context::executor_type> workGuard(ioContext.get_executor());
    
    UI userInterface(ioContext);
    userInterface.start(workGuard);
    
    #endif // CLIENT
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
    return 0;
}