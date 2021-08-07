#pragma once

#include <boost/asio.hpp>
#include <filesystem>
#include <type_traits>
#include <ostream>


namespace asio = boost::asio;
using Error = boost::system::error_code;

inline std::ostream& operator<<(std::ostream& os, const Error& error){
    return os << error.category().name() << ':' << error.value() << " - " << error.message();
}

namespace fs = std::filesystem;

using udp = asio::ip::udp;
using tcp = asio::ip::tcp;


template<class Object, class Function, class ...Args>
static auto invoke_potential_member(Object* objectPtr, Function&& function, Args&& ...args){
    if constexpr( !std::is_invocable_v< Function, Args...> ){
        return (objectPtr->*function)(std::forward<Args>(args)...);
    }
    else{
        return function(std::forward<Args>(args)...);
    }
}

template<
    class Session,
    class Callback,
    class ErrorCallback
>
auto asio_safe_callback(Session* sessionPtr, Callback&& callback, ErrorCallback&& errorCallback){
    return ([me = sessionPtr->shared_from_this(), &sessionPtr, &callback, &errorCallback](const Error& err){
        if(err){
            invoke_potential_member(sessionPtr, errorCallback, err);
        }
        else{
            invoke_potential_member(sessionPtr, callback);
        }
    });
}

template< class Session, class Callback, class ErrorCallback>
auto asio_safe_io_callback(Session* sessionPtr, Callback&& callback, ErrorCallback&& errorCallback){
    return ([me = sessionPtr->shared_from_this(), &sessionPtr, &callback, &errorCallback](const Error& err, const size_t bytesTransfered){
        if(err){
            invoke_potential_member(sessionPtr, errorCallback, err);
        }
        else{
            invoke_potential_member(sessionPtr, callback);
        }
    });
}