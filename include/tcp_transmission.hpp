#pragma once

#include "asiolib/all.hpp"

#include "async_request_queue.hpp"
#include <variant.hpp>
#include <fstream>
#include <istream>
#include <functional>

#include "operationCompletion.hpp"


class TcpTransSession : public BasicTcpSession<TcpTransSession>
{
protected:

    template <class ...Ts>
    void logInfoLine(const Ts& ...args){
        logger.logInfoLine("TCP TRANS ", remoteEndpoint, ' ', args...);
    }
    template <class ...Ts>
    void logErrorLine(const Ts& ...args){
        logger.logErrorLine("TCP TRANS ", remoteEndpoint, ' ', args...);
    }
    
    virtual void HandleError_CannotResolve(const Error& err) {
        logErrorLine("Cannot resolve specified address | ", err);
    };
    virtual void HandleError_CannotConnect(const Error& err) {
        logErrorLine("Cannot connect to specified address | ", err);
    };
    virtual void HandleError_CannotGetRemoteEndpoint(const Error& err) {
        logErrorLine("Cannot read remote endpoint of socket | ", err);
    };
    
    
    OperationCompletion operationCompletion;
    
    void handleError(const Error& err, const std::string& message = ""){
        operationCompletion.setResult(err);
        if(err == asio::error::eof){
            logErrorLine("Asio Error | ", message, " | ", "Connection unexpectedly closed.");
        }else{
            logErrorLine("Asio Error | ", message, " | ", err);
        }
        tempBufferCollection.reset();
        operationCompletion.complete();
    }
    
    ArrayBuffer<30*1024> buffer;
    
    // Buffers
    class TempBufferCollection{
    public:
        struct Empty{};
        struct Echo{ std::string message; };
        struct File{ std::shared_ptr<std::istream> istream; uint32_t fileId; std::streampos end; std::function<void(bool)> callback; };
        struct CustomFile {std::string path; uint32_t fromEnd;};
    private:
        boost::variant<
            Empty,
            Echo,
            File,
            CustomFile
        > buffer;
    public:
        template<typename T>
        bool is(){
            return buffer.type() == typeid(T);
        }
        template<typename T>
        auto& set(){
            buffer = T{};
            return get<T>();
        }
        template<typename T>
        auto& get(){
            return boost::get<T>(buffer);
        }
        void reset(){
            set<Empty>();
        }
    };
    
    TempBufferCollection tempBufferCollection;
    
    #ifdef SERVER
    
    // Main
    
    void awaitNewRequest();
    
    // Echo
    
    void receiveEchoRequestHeader();
    void handleEchoRequestHeader();
    void sendEchoResponse();
    
    
    // File
    
    void startSendFile();
    void sendFilePart();
    void completeSendFile(bool);
    
    // File Custom
    
    void receiveCustomFileRequestHeader();
    void receiveCustomFileRequestFilepath();
    void prepareCustomFileToSend();
    
    // Stream Snap Frame
    
    void prepareStreamSnapFrame();
    
    // Download Snap
    
    void receiveDownloadSnapRequest();
    void prepareSnapToSend();
    
    
    #endif // SERVER
    #ifdef CLIENT
    
    // Echo
    
    public: 
        void sendEchoRequest(const std::string& message);
    protected:
    void receiveEchoResponseHeader();
    void handleEchoResponseHeader();
    void handleEchoResponseMessage();
    
    
    // File
    
    void startReceiveFile();
    void receiveFilePart();
    void completeReceiveFile(bool successful);
    
    // File Custom
    public:
        void sendCustomFileRequest(const std::string& filepath, uint32_t fromEnd, const std::string& save_filepath , std::fstream::openmode opm = std::fstream::out);
    protected:
    
    #endif // CLIENT
    
    void completeOperation(){
        logInfoLine("Completed Operation.");
        buffer.resetCarret();
        tempBufferCollection.reset();
        operationCompletion.complete();
        #ifdef SERVER
            awaitNewRequest();
        #endif // SERVER
    }
    
public:
    
    virtual bool startSession() override{
        #ifdef SERVER
            awaitNewRequest();
        #endif // SERVER
        return true;
    }
    
    template <typename T>
    void setOperationCompletionHandler(T&& handler){
        operationCompletion.setCompletionHandler(handler);
    }
    
    TcpTransSession(asio::io_context& ioContext)
        : BasicTcpSession<TcpTransSession>(ioContext)
    {}
    
};


class TcpTransServer : public BasicTcpServer<TcpTransSession>
{    
protected:
    
    virtual void HandleError_Connection(const Error& err) override {
        logger.logErrorLine("TCP TRANS SERVER ", "Cannot connect to client | ", err );
    };
    virtual void HandleError_CannotOpenAcceptor(const Error& err) override {
        logger.logErrorLine("TCP TRANS SERVER ", "Cannot open acceptor | ", err  );
    };
    virtual void HandleError_CannotBindAcceptor(const Error& err) override {
        logger.logErrorLine("TCP TRANS SERVER ", "Cannot bind acceptor | ", err );
    };
    virtual void HandleError_GettingRemoteEndpoint(const Error& err) override {
        logger.logErrorLine("TCP TRANS SERVER ", "Cannot get remote endpoint of client | ", err );
    };
public:
    
    TcpTransServer(asio::io_context& ioContext)
        : BasicTcpServer<TcpTransSession>(ioContext)
    {}
};

class TcpTransServerService{
    
    asio::io_context *ioContext = nullptr;
    boost::optional<TcpTransServer> m_server;
    
    Error err;
    
public:
    auto& server() {return *m_server;}
    
    //auto& client() {return *m_client;}
    
    auto getError() {return err;}
    
    void setContext(asio::io_context& ioContext){
        this->ioContext = &ioContext;
    }
    
    bool start(const int port){
        m_server.emplace(*ioContext);
        if(!m_server->startServer(port)){
            err = m_server->getError();
            return false;
        }
        return true;
    }
    
    bool stop(){
        if(m_server){
            m_server->stopServer();
            m_server.reset();
        }
        return true;
    }
};

#ifdef SERVER

extern TcpTransServerService tcpTransServerService;

#endif // SERVER