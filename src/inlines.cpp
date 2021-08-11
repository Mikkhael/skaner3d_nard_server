#include <asiolib/logger.hpp>
#include <config.hpp>
#include <datagrams.hpp>

Config config;
Logger logger;

// DIAG

constexpr DatagramId Diag::Ping::Request::Id;
constexpr DatagramId Diag::Ping::Response::Id;

constexpr DatagramId Diag::Echo::Request::Id;
constexpr DatagramId Diag::Echo::Response::Id;

constexpr DatagramId Diag::Mult::Request::Id;
constexpr DatagramId Diag::Mult::Response::Id;

// TRANS

constexpr DatagramId Trans::Echo::Request::Id;

constexpr DatagramId Trans::FilePart::Id_Start;
constexpr DatagramId Trans::FilePart::Id_Part;
constexpr DatagramId Trans::FilePart::Id_Success;
constexpr DatagramId Trans::FilePart::Id_Fail;

constexpr DatagramId Trans::CustomFile::Request::Id;