#include "asiolib/all.hpp"
#include "datagrams.hpp"

class UdpDiagHandler : public BasicUdpDatagramHandler<UdpDiagHandler, 1500>
{
protected:
    void defaultErrorHandler(const Error& err){
       logErrorLine("Asio Error: ", err.message());
    }
    
    size_t bytesTransfered;
    
    void handleUdpPingRequest();
    void handleUdpPingResponse();
    
    void handleUdpEchoRequest();
    void handleUdpEchoResponse();
    
    void handleUdpMultRequest();
    void handleUdpMultResponse();
        
public:
    
    template <class ...Ts>
    void logInfoLine(const Ts& ...args){
        logger.logInfoLine("UDP DIAG ", remoteEndpoint, ' ', args...);
    }
    template <class ...Ts>
    void logErrorLine(const Ts& ...args){
        logger.logErrorLine("UDP DIAG ", remoteEndpoint, ' ', args...);
    }
    
    void handle(const size_t bytesTransfered){
        this->bytesTransfered = bytesTransfered;
        
        buffer.resetCarret();
        
        DatagramId id;
        buffer.loadObjectCarret(id);
        logInfoLine("Received datagram | id :", int(id), " | bytesTransfered: ", bytesTransfered);
        
        switch(id){
            case Diag::Ping::Request::Id:  handleUdpPingRequest();  break;
            case Diag::Ping::Response::Id: handleUdpPingResponse(); break;
            
            case Diag::Echo::Request::Id:  handleUdpEchoRequest();  break;
            case Diag::Echo::Response::Id: handleUdpEchoResponse(); break;
            
            case Diag::Mult::Request::Id:  handleUdpMultRequest();  break;
            case Diag::Mult::Response::Id: handleUdpMultResponse(); break;
            
            default:{
                logErrorLine("Unknown id");
                break;
            }
        }
    }
};


class UdpDiagServer : public BasicUdpServer<UdpDiagHandler>
{
protected:
    std::string Headline = "UDP DIAG SERVER: ";
    
    virtual void HandleError_UnableToOpenSocket(const Error& err) {
        logger.logErrorLine(Headline, "Cannot open socket. | ", err.message());
    };
    virtual void HandleError_UnableToBindSocket(const Error& err) {
        logger.logErrorLine(Headline, "Cannot bind socket. | ", err.message());
    };
    virtual void HandleError_NewDatagramReceive(const Error& err) {
        logger.logErrorLine(Headline, "Error receiving new datagram. Stopping server. | ", err.message());
        stopServer();
    };
public:
    
    UdpDiagServer(asio::io_context& ioContext)
        : BasicUdpServer(ioContext)
    {
    }
};



class UdpDiagService{
    
    std::optional<UdpDiagServer> m_server;
    
    Error err;
    
public:
    
    auto getError() const {return err;}
    
    auto& server() {return *m_server;}
    auto& socket() {return m_server->getSocket();}
    
    bool start(asio::io_context& ioContext, int port){
        m_server.emplace(ioContext);
        if(!m_server->startServer(port)){
            err = m_server->getError();
            return false;
        }
        return true;
    }
};

inline UdpDiagService udpDiagService;