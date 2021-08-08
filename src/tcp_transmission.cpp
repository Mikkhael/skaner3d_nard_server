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
    
    asio::async_read(getSocket(), buffer.get(sizeof(DatagramId)), ASYNC_CALLBACK{
        if(err){
            if(err == asio::error::eof){
                me->logInfoLine("Connection closed.");
                return;
            }
            me->handleError(err, "While awaiting New Request");
            return;
        }
        DatagramId id;
        me->buffer.loadObject(id);
        std::cout << "COUNT AFTER: " << me->weak_from_this().use_count() << std::endl;
        me->logInfoLine("Received request with id: ", int(id));
        switch(id){
            case Trans::Echo::Request::Id : me->receiveEchoRequestHeader(); break;
            default: {
                me->logErrorLine("Unknown request id: ", int(id));
                me->awaitNewRequest();
            }
        }
    });
}

// Echo

void TcpTransSession::receiveEchoRequestHeader(){
    logInfoLine("Receiving Echo Request Header");
    //std::cout << "COUNT RECEIVE: " << weak_from_this().use_count() << std::endl;
    tempBufferCollection.set<TempBufferCollection::Echo>();
    asio::async_read(getSocket(), buffer.get(sizeof(Trans::Echo::Request)), ASYNC_CALLBACK{
        if(err){
            me->handleError(err, "While receiving Echo Request Header.");
            return;
        }
        me->handleEchoRequestHeader();
    });
}

void TcpTransSession::handleEchoRequestHeader(){
    
    //std::cout << "COUNT HANDLE: " << weak_from_this().use_count() << std::endl;
    auto& tempBuffer = tempBufferCollection.get<TempBufferCollection::Echo>();
    
    Trans::Echo::Request request;
    buffer.loadObject(request);
    tempBuffer.message.resize(request.length);
    
    logInfoLine("Received Echo Request Header with length: ", int(request.length));
    
    asio::async_read(getSocket(), asio::buffer(tempBuffer.message), ASYNC_CALLBACK{
        if(err){
            me->handleError(err, "While receiving Echo Request message.");
            return;
        }
        me->sendEchoResponse();
    });
}

void TcpTransSession::sendEchoResponse(){
    auto& tempBuffer = tempBufferCollection.get<TempBufferCollection::Echo>();
    Trans::Echo::Response response;
    response.length = tempBuffer.message.size();
    logInfoLine("Sending Echo Response with message: ", tempBuffer.message);
    Error err;
    asio::write(getSocket(), 
        std::array{
            to_buffer(response),
            asio::buffer(tempBuffer.message)
        },
        err
    );
    if(err){
        handleError(err, "While sending Echo Request Response.");
        return;
    }
    completeOperation();
}


#endif // SERVER
#ifdef CLIENT

// Echo

void TcpTransSession::sendEchoRequest(std::string_view message){
    Trans::Echo::Request request;
    request.length = message.size();
    Error err;
    logInfoLine("Sending Echo Request with length ", request.length, " and message: ", message);
    asio::write(getSocket(), std::array{
        to_buffer(Trans::Echo::Request::Id),
        to_buffer_const(request),
        asio::buffer(message)
    }, err);
    if(err){
        handleError(err, "While sending Echo Request.");
        return;
    }
    receiveEchoResponseHeader();
}
void TcpTransSession::receiveEchoResponseHeader(){
    logInfoLine("Receiving Echo Response Header.");
    asio::async_read(getSocket(),buffer.get(sizeof(Trans::Echo::Response)), ASYNC_CALLBACK{
        if(err){
            me->handleError(err, "While receiving Echo Response Header");
            return;
        }
        me->handleEchoResponseHeader();
    });
}

void TcpTransSession::handleEchoResponseHeader(){
    Trans::Echo::Response response;
    buffer.loadObject(response);
    
    logInfoLine("Received Echo Response Header with length ", response.length);
    auto& tempBuffer = tempBufferCollection.set<TempBufferCollection::Echo>();
    tempBuffer.message.resize(response.length);
    asio::async_read(getSocket(), asio::buffer(tempBuffer.message.data(), response.length), ASYNC_CALLBACK{
        if(err){
            me->handleError(err, "While receiving Echo Response Message");
            return;
        }
        me->handleEchoResponseMessage();
    });
    
}

void TcpTransSession::handleEchoResponseMessage(){
    auto& tempBuffer = tempBufferCollection.get<TempBufferCollection::Echo>();
    logInfoLine("Received Echo Response with message: ", tempBuffer.message);
    completeOperation();
}

#endif // CLIENT

