#include <udp_diag.hpp>

void UdpDiagHandler::handleUdpPingRequest(){
    buffer.saveObject(Diag::Ping::Response::Id);
    logInfoLine("Received Ping Request. Sending Pong");
    udpDiagService.socket().async_send_to_safe(
        buffer.get(sizeof(DatagramId)),
        remoteEndpoint,
        asio_safe_io_callback( this,
            []{},
            &UdpDiagHandler::defaultErrorHandler
        )
    );
}
void UdpDiagHandler::handleUdpPingResponse(){
    logInfoLine("Received Pong from ", remoteEndpoint);
}

void UdpDiagHandler::handleUdpEchoRequest(){
    std::string message;
    message.resize(bytesTransfered - sizeof(DatagramId));
    buffer.loadBytesAt(sizeof(DatagramId), message.data(), message.size());
    
    logInfoLine("Received Echo Request with message: ", message);
    
    buffer.saveObject(Diag::Echo::Response::Id);
    
    udpDiagService.socket().async_send_to_safe(
        buffer.get(bytesTransfered),
        remoteEndpoint,
        asio_safe_io_callback( this, 
            []{},
            &UdpDiagHandler::defaultErrorHandler
        )
    );
}
void UdpDiagHandler::handleUdpEchoResponse(){
    std::string message;
    message.resize(bytesTransfered - sizeof(DatagramId));
    buffer.loadBytesAt(sizeof(DatagramId), message.data(), message.size());
    
    logInfoLine("Received Echo Response with message: ", message);
}

void UdpDiagHandler::handleUdpMultRequest(){
    Diag::Mult::Request request;
    buffer.loadObjectAt(sizeof(DatagramId), request);
    
    logInfoLine("Received Mult Request with parameters: ", request.operands[0], " ", request.operands[1]);
    
    buffer.saveObject(Diag::Echo::Response::Id);
    
    Diag::Mult::Response response;
    response.result = request.operands[0] * request.operands[1];
    
    logInfoLine("Sending Mult Response with result: ", response.result);
    
    udpDiagService.socket().async_send_to_safe(
        buffer.get(sizeof(DatagramId) + sizeof(response)),
        remoteEndpoint,
        asio_safe_io_callback( this, 
            []{},
            &UdpDiagHandler::defaultErrorHandler
        )
    );
}

void UdpDiagHandler::handleUdpMultResponse(){
    Diag::Mult::Response response;
    buffer.loadObjectAt(sizeof(DatagramId), response);
    
    logInfoLine("Received Mult Response with result: ", response.result);
}