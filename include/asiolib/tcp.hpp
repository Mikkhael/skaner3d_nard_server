#pragma once

#include <memory>
#include <mutex>

#include "utils.hpp"
#include "buffer.hpp"

template<class Session>
class BasicTcpServer {
protected:
    
    asio::io_context& ioContext;
    tcp::endpoint endpoint;
    tcp::acceptor acceptor;
    
    bool isRunning = false;
    int port = 0;
    
    Error err;
    
    virtual void HandleError_Connection(const Error& err) {};
    virtual void HandleError_CannotOpenAcceptor(const Error& err) {};
    virtual void HandleError_CannotBindAcceptor(const Error& err) {};
    virtual void HandleError_GettingRemoteEndpoint(const Error& err) {};
    
    
public:
    
    auto getError() {return err;}
    auto& getSocket() {return socket;}
    
    
    void stopServer(){
        Error ignored;
        isRunning = false;
        acceptor.cancel(ignored);
        acceptor.close(ignored);
    }
    
    bool startServer(const int port){
        stopServer();
        acceptor.open(tcp::v4(), err);
        if(err){
            HandleError_CannotOpenAcceptor(err);
            return false;
        }
        acceptor.bind(tcp::endpoint(tcp::v4(), port), err);
        if(err){
            HandleError_CannotBindAcceptor(err);
            return false;
        }
        
        awaitNewConnection();
        return true;
    }
    
    BasicTcpServer(asio::io_context& ioContext)
        : ioContext(ioContext), acceptor(acceptor)
    {
    }
    
private:
    
    void awaitNewConnection(){
        auto newSession = std::make_shared<Session>(ioContext);
        
		acceptor.async_accept( newSession->getSocket(), [this, newSession](const Error& err){
			handleNewConnection(*newSession, err);
		});
    }
    
    bool handleNewConnection(const Session& session, const Error& err)
	{
		if(err)
		{
            HandleError_Connection(err);
            this->err = err;
			return false;
		}
		
		const auto remoteEndpoint = session.getSocket().remote_endpoint(this->err);
        if(this->err){
            HandleError_GettingRemoteEndpoint(this->err);
            return false;
        }
        session.getSocket().remoteEndpoint = remoteEndpoint;
        asio::post( ioContext, 
            [me = session.shared_from_this()]{
                me->startSession();
            }
        );
		awaitNewConnection();
        return true;
	}
    
    virtual ~BasicTcpServer(){
        stopServer();
    }
};


template <class Session>
class BasicTcpSession : public std::enable_shared_from_this<Session> {
protected:

    asio::io_context& ioContext;
    tcp::socket socket;
    
    tcp::resolver resolver;
    
    Error err;
    
    virtual void HandleError_CannotResolve(const Error& err) {};
    virtual void HandleError_CannotConnect(const Error& err) {};
    virtual void HandleError_CannotGetRemoteEndpoint(const Error& err) {};
    
public:
    
    auto getError() {return err;}
    auto& getSocket() {return socket;}
    
    tcp::endpoint remoteEndpoint;
    
    BasicTcpSession(asio::io_context& ioContext)
        : ioContext(ioContext), socket(ioContext), resolver(ioContext)
    {}
    
    virtual void startSession() = 0;
    
    void closeSession(){
        Error ignored;
        socket.cancel(ignored);
        socket.shutdown(udp::socket::shutdown_both, ignored);
        socket.close(ignored);
    }
    
    bool resolveAndConnect(const std::string_view hostname, const std::string_view port)
	{
		closeSession();
		
		auto resolvedRemotes = this->resolver.resolve(hostname, port, err);
		if(err){
			HandleError_CannotResolve(err);
			return false;
		}
		
		asio::connect(socket, resolvedRemotes, err);
		if(err){
			HandleError_CannotConnect(err);
			return false;
		}
		
		remoteEndpoint = socket.remote_endpoint(err);
		if(err){
			HandleError_CannotGetRemoteEndpoint(err);
			return false;
		}
		
		return startSession();
	}
    
    virtual ~BasicTcpSession(){
        closeSession();
    }
};