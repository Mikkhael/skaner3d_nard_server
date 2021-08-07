#pragma once

#include "asiolib/all.hpp"
#include "datagrams.hpp"

#include <optional>
#include <functional>

class UdpDiagHandler : public BasicUdpDatagramHandler<UdpDiagHandler, 1500>
{
protected:
    void defaultErrorHandler(const Error& err){
        if(err == asio::error::operation_aborted)
            return;
        logErrorLine("Asio Error: ", err);
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
        logger.logErrorLine(Headline, "Cannot open socket. | ", err);
    };
    virtual void HandleError_UnableToBindSocket(const Error& err) {
        logger.logErrorLine(Headline, "Cannot bind socket. | ", err);
    };
    virtual void HandleError_NewDatagramReceive(const Error& err) {
        if(err == asio::error::connection_refused) return;
        logger.logErrorLine(Headline, "Error receiving new datagram. | ", err);
        //stopServer();
    };
    virtual void HandleError_GettingLocalEndpoint(const Error& err) {
        logger.logErrorLine(Headline, "Error getting local endpoint of a socket. | ", err);
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
    
    bool start(asio::io_context& ioContext, const int port = 0){
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
    
    
    
    ////////    
    
    template< typename ConstBufferSequence >
    bool sendCustomRequest(
        const ConstBufferSequence & buffers,
        const udp::endpoint & destination
    ){
        socket().send_to_safe(
            asio::buffer(buffers),
            destination,
            0,
            err
        );
        if(err){
            return false;
        }
        return true;
    }
    
    bool sendPingRequest(const udp::endpoint destination){
        logger.logInfoLine("UDP DIAG Sending Ping Request to endpoint ", destination);
        std::vector<char> buffer(
            sizeof(DatagramId)
        );
        std::memcpy(buffer.data(), &Diag::Ping::Request::Id, sizeof(DatagramId));
        
        return sendCustomRequest(buffer, destination);
    }
    
    bool sendEchoRequest(const udp::endpoint destination, std::string_view message){
        logger.logInfoLine("UDP DIAG Sending Echo Request to endpoint ", destination, " with message: ", message);
        std::vector<char> buffer(
            sizeof(DatagramId) + 
            message.size()
        );
        std::memcpy(buffer.data(), &Diag::Echo::Request::Id, sizeof(DatagramId));
        std::memcpy(buffer.data() + sizeof(DatagramId), message.data(), message.size());
        
        return sendCustomRequest(buffer, destination);
    }
    
    bool sendMultRequest(const udp::endpoint destination, int op1, int op2){
        logger.logInfoLine("UDP DIAG Sending Mult Request to endpoint ", destination, " with operands: ", op1, " and ", op2);
        std::vector<char> buffer(
            sizeof(DatagramId) + 
            sizeof(Diag::Mult::Request)
        );
        Diag::Mult::Request request;
        request.operands[0] = op1;
        request.operands[1] = op2;
        
        std::memcpy(buffer.data(), &Diag::Mult::Request::Id, sizeof(DatagramId));
        std::memcpy(buffer.data() + sizeof(DatagramId), &request, sizeof(request));
        
        return sendCustomRequest(buffer, destination);
    }
};

inline UdpDiagService udpDiagService;