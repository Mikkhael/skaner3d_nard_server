#pragma once

#include "asiolib/all.hpp"

#include "async_request_queue.hpp"
#include <variant>

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
    
    
    void handleError(const Error& err, std::string_view message = ""){
        if(err == asio::error::eof){
            logErrorLine("Asio Error | ", message, " | ", "Connection unexpectedly closed.");
            return;
        }
        logErrorLine("Asio Error | ", message, " | ", err);
    }
    
    ArrayBuffer<512> buffer;
    
    // Buffers
    class TempBufferCollection{
    public:
        struct Empty{};
        struct Echo{ std::string message; };
    private:
        std::variant<
            Empty,
            Echo
        > buffer;
    public:
        template<typename T>
        auto& set(){
            buffer.emplace<T>();
            return get<T>();
        }
        template<typename T>
        auto& get(){
            return std::get<T>(buffer);
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
    
    
    #endif // SERVER
    #ifdef CLIENT
    
    // Echo
    
    public: 
        void sendEchoRequest(std::string_view message);
    protected:
    void receiveEchoResponseHeader();
    void handleEchoResponseHeader();
    void handleEchoResponseMessage();
    
    #endif // CLIENT
    
    void completeOperation(){
        tempBufferCollection.reset();
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
    std::optional<TcpTransServer> m_server;
    
    Error err;
    
public:
    auto& server() {return *m_server;}
    
    //auto& client() {return *m_client;}
    
    auto getError() {return err;}
    
    bool start(asio::io_context& ioContext, const int port){
        m_server.emplace(ioContext);
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

inline TcpTransServerService tcpTransServerService;

#endif // SERVER