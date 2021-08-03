#pragma once

#include <memory>
#include <mutex>

#include "utils.hpp"
#include "buffer.hpp"

class UdpSafeSocket : public udp::socket
{
    std::mutex mutex;
public:
    
    template<typename ...Ts>
    UdpSafeSocket(Ts&& ...args)
        : udp::socket(std::forward<Ts>(args)...)
    {
    }
    
    template<class ...Args>
    auto async_receive_from_safe(Args&& ...args)
    {
        std::lock_guard lock{mutex};
        return udp::socket::async_receive_from(std::forward<Args>(args)...);
    }
    
    template<class ...Args>
    auto async_send_to_safe(Args&& ...args)
    {
        std::lock_guard lock{mutex};
        return udp::socket::async_send_to(std::forward<Args>(args)...);
    }    
};

template<class Handler>
class BasicUdpServer {
protected:
    
    asio::io_context& ioContext;
    UdpSafeSocket socket;
    udp::endpoint endpoint;
    
    bool isRunning = false;
    int port = 0;
    
    Error err;
    
    virtual void HandleError_UnableToOpenSocket(const Error& err) {};
    virtual void HandleError_UnableToBindSocket(const Error& err) {};
    virtual void HandleError_NewDatagramReceive(const Error& err) {};
    
    
public:
    
    auto getError() {return err;}
    auto& getSocket() {return socket;}
    
    
    void stopServer(){
        Error ignored;
        isRunning = false;
        socket.cancel(ignored);
        socket.shutdown(udp::socket::shutdown_both, ignored);
        socket.close(ignored);
    }
    
    bool startServer(const int port){
        
        stopServer();
        
        socket.open(udp::v4(), err);
        if(err){
            HandleError_UnableToOpenSocket(err);
            return false;
        }
        
        endpoint = udp::endpoint(udp::v4(), port);
        socket.bind(endpoint, err);
        if(err){
            HandleError_UnableToBindSocket(err);
            return false;
        }
        
        this->port = port;
        isRunning = true;
        awaitNewDatagram();
        return true;
    }
    
    BasicUdpServer(asio::io_context& ioContext)
        : ioContext(ioContext), socket(ioContext)
    {
    }
    
private:
    
    void awaitNewDatagram(){
        //std::cout << "Awaiting new datagram, Endpoint: " << socket.local_endpoint() << '\n';
        auto newHandler = std::make_shared<Handler>();
		socket.async_receive_from_safe(newHandler->buffer.get(), newHandler->remoteEndpoint, [this, newHandler](const Error& err, const size_t bytesTransfered){
			if(err)
			{
                this->err = err;
				HandleError_NewDatagramReceive(err);
			}
			else
			{
				asio::post( ioContext,
                    [newHandler,bytesTransfered]{ 
                        newHandler->handle(bytesTransfered);
                    }
                );
			}
			awaitNewDatagram();
		});
    }
};


template <class Handler, size_t BufferSize>
class BasicUdpDatagramHandler : public std::enable_shared_from_this<Handler> {
protected:
public:
    ArrayBuffer<BufferSize> buffer;
    udp::endpoint remoteEndpoint;
    
    virtual void handle(const size_t bytesTransfered) = 0;
};