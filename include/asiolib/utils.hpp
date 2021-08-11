#pragma once

#include <boost/asio.hpp>
//#include <filesystem>
#include <type_traits>
#include <ostream>


namespace asio = boost::asio;
using Error = boost::system::error_code;

inline std::ostream& operator<<(std::ostream& os, const Error& error){
    return os << error.category().name() << ':' << error.value() << " - " << error.message();
}

//namespace fs = std::filesystem;

using udp = asio::ip::udp;
using tcp = asio::ip::tcp;


template<typename T>
auto to_buffer(T& object){
    return asio::buffer(&object, sizeof(T));
}
template<typename T>
auto to_buffer_const(const T& object){
    return asio::buffer(&object, sizeof(T));
}

template<typename ...Ts>
auto buffers_array(Ts&& ...ts){
    return std::array<asio::mutable_buffer, sizeof...(ts)>{ts...};
}
template<typename ...Ts>
auto const_buffers_array(Ts&& ...ts){
    return std::array<asio::const_buffer, sizeof...(ts)>{ts...};
}


#define ASYNC_CALLBACK_NESTED [me](const Error& err, const size_t bytesTransfered)
#define ASYNC_CALLBACK [me = shared_from_this()](const Error& err, const size_t bytesTransfered)
#define ASYNC_CALLBACK_CAPTURE(...) [me = shared_from_this(), __VA_ARGS__](const Error& err, const size_t bytesTransfered)