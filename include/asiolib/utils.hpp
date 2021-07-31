#pragma once

#include <boost/asio.hpp>
#include <filesystem>

namespace asio = boost::asio;
using Error = boost::system::error_code;

namespace fs = std::filesystem;

using udp = asio::ip::udp;
using tcp = asio::ip::tcp;