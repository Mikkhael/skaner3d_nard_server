#include <asiolib/all.hpp>
#include <chrono>
#include <iostream>
#include <vector>
#include <thread>

#include <config.hpp>

int main(const int argc, const char** argv)
{
    
    // Setup
    
    asio::io_context ioContext;
    asio::steady_timer timer(ioContext);
    
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
    
    // Running ioContext
    
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
    
    return 0;
}
