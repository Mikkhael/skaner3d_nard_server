#pragma once

#include "utils.hpp"



class UdpServer
{
    
    asio::io_context& ioContext;
    udp::socket socket;
    
    
    
public:
    
    UdpServer(asio::io_context& ioContext)
        : ioContext(ioContext), socket(ioContext)
    {
    }
    
};