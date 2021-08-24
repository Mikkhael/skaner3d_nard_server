#include <udp_diag.hpp>
#include <systemManager.hpp>
#include <snap.hpp>

void UdpDiagHandler::handleUdpPingRequest(){
    buffer.saveObject(Diag::Ping::Response::Id);
    logInfoLine("Received Ping Request. Sending Pong");
    udpDiagService.socket().async_send_to_safe(
        buffer.getToCarret(),
        remoteEndpoint,
        ASYNC_CALLBACK{
            if(err){
                me->defaultErrorHandler(err);
            }
        }
    );
}
void UdpDiagHandler::handleUdpPingResponse(){
    logInfoLine("Received Pong from ", remoteEndpoint);
}

void UdpDiagHandler::handleUdpEchoRequest(){
    std::string message;
    message.resize(bytesTransfered - sizeof(DatagramId));
    buffer.loadBytesCarret(&(message[0]), message.size());
    
    logInfoLine("Received Echo Request with message: ", message);
    
    buffer.saveObject(Diag::Echo::Response::Id);
    
    udpDiagService.socket().async_send_to_safe(
        buffer.getToCarret(),
        remoteEndpoint,
        ASYNC_CALLBACK{
            if(err){
                me->defaultErrorHandler(err);
            }
        }
    );
}
void UdpDiagHandler::handleUdpEchoResponse(){
    std::string message;
    message.resize(bytesTransfered - sizeof(DatagramId));
    buffer.loadBytesCarret(&(message[0]), message.size());
    
    logInfoLine("Received Echo Response with message: ", message);
}

void UdpDiagHandler::handleUdpMultRequest(){
    Diag::Mult::Request request;
    buffer.loadObjectCarret(request);
    
    logInfoLine("Received Mult Request with parameters: ", request.operands[0], " ", request.operands[1]);
    
    Diag::Mult::Response response;
    response.result = request.operands[0] * request.operands[1];
    
    buffer.resetCarret();
    buffer.saveObjectCarret(Diag::Mult::Response::Id);
    buffer.saveObjectCarret(response);
    
    logInfoLine("Sending Mult Response with result: ", response.result);
    
    udpDiagService.socket().async_send_to_safe(
        buffer.getToCarret(),
        remoteEndpoint,
        ASYNC_CALLBACK{
            if(err){
                me->defaultErrorHandler(err);
            }
        }
    );
}

void UdpDiagHandler::handleUdpMultResponse(){
    Diag::Mult::Response response;
    buffer.loadObjectCarret(response);
    
    logInfoLine("Received Mult Response with result: ", response.result);
}


static std::string ipToString(uint32_t ip){
    std::string res;
    constexpr uint32_t base = 256;
    uint32_t subs[4];
    subs[3] = ip % base; ip /= base;
    subs[2] = ip % base; ip /= base;
    subs[1] = ip % base; ip /= base;
    subs[0] = ip % base;
    res += std::to_string(subs[0]); res += '.';
    res += std::to_string(subs[1]); res += '.';
    res += std::to_string(subs[2]); res += '.';
    res += std::to_string(subs[3]);
    return res;
}

void UdpDiagHandler::handleUdpConfigNetworkSet(){
    Diag::Config::Network::Set::Request request;
    buffer.loadObjectCarret(request);
    logInfoLine("Received Config Network Set Request with parameters: \n\t",
        ipToString(request.ip), "\n\t",
        ipToString(request.mask), "\n\t",
        ipToString(request.gateway), "\n\t",
        ipToString(request.dns1), "\n\t",
        ipToString(request.dns2), "\n\t",
        request.isDynamic == 0 ? "static" : "dynamic"
    );
    bool res = systemManager.setNetworkSettings(
        ipToString(request.ip),
        ipToString(request.mask),
        ipToString(request.gateway),
        ipToString(request.dns1),
        ipToString(request.dns2),
        request.isDynamic == 0 ? false : true
    );
    logInfoLine("Setting new network config status: ", res);
    udpDiagService.socket().async_send_to_safe(
        to_buffer_const(res ? 
            Diag::Config::Network::Set::Response::Id_Success :
            Diag::Config::Network::Set::Response::Id_Fail),
        remoteEndpoint,
        ASYNC_CALLBACK{
            if(err){
                me->defaultErrorHandler(err);
            }
        }
    );
}

void UdpDiagHandler::handleUdpConfigDevicekSet(){
    //Diag::Config::Device::Set::Request request;
    //buffer.loadObjectCarret(request);
    std::string value;
    value.resize(bytesTransfered - 1);
    buffer.loadBytesCarret(&value[0], value.size());
    logInfoLine("Received Config Device Set Request with value:\n", value);
    bool res = systemManager.setDeviceConfig(value);
    logInfoLine("Setting new device config status: ", res);
    udpDiagService.socket().async_send_to_safe(
        to_buffer_const(res ? 
            Diag::Config::Device::Set::Response::Id_Success :
            Diag::Config::Device::Set::Response::Id_Fail),
        remoteEndpoint,
        ASYNC_CALLBACK{
            if(err){
                me->defaultErrorHandler(err);
            }
        }
    );
}

void UdpDiagHandler::handleUdpConfigDevicekGet(){
    //Diag::Config::Device::Get::Request request;
    //buffer.loadObjectCarret(request);
    logInfoLine("Received Config Device Get Request");
    std::string value = systemManager.getDeviceConfig();
    logInfoLine("Responding woth Config Device value: ", value);
    udpDiagService.socket().async_send_to_safe(
        const_buffers_array(
            to_buffer_const( Diag::Config::Device::Get::Response::Id ),
            asio::buffer(value)
        ),
        remoteEndpoint,
        ASYNC_CALLBACK{
            if(err){
                me->defaultErrorHandler(err);
            }
        }
    );
}

void UdpDiagHandler::handleUdpReboot(){
    logInfoLine("Received Reboot Request");
    systemManager.reboot();
}


void UdpDiagHandler::handleUdpSnap(){
    Diag::Snap header;
    buffer.loadObjectCarret(header);
    snapper.snap([me=shared_from_this(), header](bool successful, std::string err_message){
        if(successful){
            me->logInfoLine("Snapping image with seriesid ", header.seriesid, " successful.");
        } else {
            me->logErrorLine("Snapping image with seriesid ", header.seriesid, " not successful: ", err_message);
        }
        
    }, header.seriesid);
}

void UdpDiagHandler::handleUdpDeleteAllSnaps(){
    logInfoLine("Deleting all snaps.");
    snapper.deleteAllSnaps([me = shared_from_this()](bool success, auto err){
        if(!success){
            me->logErrorLine("Error during deleting all snaps: ", err);
            return;
        }
        me->logInfoLine("Successfully deleted all snaps");
    });
}


UdpDiagService udpDiagService;