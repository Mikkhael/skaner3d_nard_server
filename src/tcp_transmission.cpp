#include <tcp_transmission.hpp>
#include <datagrams.hpp>


#ifdef SERVER

// Main

void TcpTransSession::awaitNewRequest(){
    logInfoLine("Awaiting new request.");
    //std::cout << "COUNT BEFORE: " << this->weak_from_this().use_count() << std::endl;
    
    // asio::async_read(getSocket(), buffer.get(sizeof(DatagramId)), [me = this->shared_from_this()](const Error& err, const size_t bytesTransfered){
    //     DatagramId id;
    //     me->buffer.loadObject(id);
    //     std::cout << "NO ELO : " << me.use_count() << "  " << id << std::endl;
    //     std::cout << me.use_count();
    //     me->awaitNewRequest();
    // });
    
    asio::async_read(getSocket(), buffer.get(sizeof(DatagramId)), asio_safe_io_callback(this, 
        [this]{
            DatagramId id;
            buffer.loadObject(id);
            //std::cout << "COUNT AFTER: " << weak_from_this().use_count() << std::endl;
            logInfoLine("Received request with id: ", int(id));
            switch(id){
                case Trans::Echo::Request::Id : receiveEchoRequestHeader(); break;
                default: {
                    logErrorLine("Unknown request id: ", int(id));
                    awaitNewRequest();
                }
            }
        },
        &TcpTransSession::defaultErrorHandler
    ));
}

// Echo

void TcpTransSession::receiveEchoRequestHeader(){
    logInfoLine("Receiving Echo Request Header");
    //std::cout << "COUNT RECEIVE: " << weak_from_this().use_count() << std::endl;
    tempBufferCollection.set<TempBufferCollection::Echo>();
    asio::async_read(getSocket(), buffer.get(sizeof(Trans::Echo::Request)), asio_safe_io_callback(this,
        &TcpTransSession::handleEchoRequestHeader,
        &TcpTransSession::defaultErrorHandler
    ));
}

void TcpTransSession::handleEchoRequestHeader(){
    
    //std::cout << "COUNT HANDLE: " << weak_from_this().use_count() << std::endl;
    auto& tempBuffer = tempBufferCollection.get<TempBufferCollection::Echo>();
    
    Trans::Echo::Request request;
    buffer.loadObject(request);
    tempBuffer.message.resize(request.length);
    
    logInfoLine("Received Echo Request Header with length: ", int(request.length));
    
    asio::async_read(getSocket(), asio::buffer(tempBuffer.message), asio_safe_io_callback(this, 
        &TcpTransSession::sendEchoResponse,
        &TcpTransSession::defaultErrorHandler
    ));
}

void TcpTransSession::sendEchoResponse(){
    auto& tempBuffer = tempBufferCollection.get<TempBufferCollection::Echo>();
    Trans::Echo::Response response;
    response.length = tempBuffer.message.size();
    logInfoLine("Sending Echo Response with message: ", tempBuffer.message);
    asio::async_write(getSocket(), 
        std::array{
            to_buffer(response),
            asio::buffer(tempBuffer.message)
        },
        asio_safe_io_callback(this, 
            &TcpTransSession::completeOperation,
            &TcpTransSession::defaultErrorHandler
        )
    );
}


#endif // SERVER
#ifdef CLIENT

// Echo

void TcpTransSession::sendEchoRequest(std::string_view message){
    
}
void TcpTransSession::receiveEchoResponseHeader(){
    
}

void TcpTransSession::receiveEchoResponseMessage(){
    
}

#endif // CLIENT

