#include <asiolib/logger.hpp>
#include <config.hpp>
#include <datagrams.hpp>
#include <snap.hpp>

Config config;
Logger logger;
Snapper snapper;

// DIAG

constexpr DatagramId Diag::Ping::Request::Id;
constexpr DatagramId Diag::Ping::Response::Id;

constexpr DatagramId Diag::Echo::Request::Id;
constexpr DatagramId Diag::Echo::Response::Id;

constexpr DatagramId Diag::Mult::Request::Id;
constexpr DatagramId Diag::Mult::Response::Id;

constexpr DatagramId Diag::Config::Network::Set::Request::Id;
constexpr DatagramId Diag::Config::Network::Set::Response::Id_Success;
constexpr DatagramId Diag::Config::Network::Set::Response::Id_Fail;

constexpr DatagramId Diag::Config::Device::Set::Request::Id;
constexpr DatagramId Diag::Config::Device::Set::Response::Id_Success;
constexpr DatagramId Diag::Config::Device::Set::Response::Id_Fail;

constexpr DatagramId Diag::Config::Device::Get::Request::Id;
constexpr DatagramId Diag::Config::Device::Get::Response::Id;

constexpr DatagramId Diag::Reboot::Id;

constexpr DatagramId Diag::Snap::Id;

// TRANS

constexpr DatagramId Trans::Echo::Request::Id;

constexpr DatagramId Trans::FilePart::Id_Start;
constexpr DatagramId Trans::FilePart::Id_Part;
constexpr DatagramId Trans::FilePart::Id_Success;
constexpr DatagramId Trans::FilePart::Id_Fail;

constexpr DatagramId Trans::CustomFile::Request::Id;

constexpr DatagramId Trans::SnapFrame::Request::Id;

constexpr DatagramId Trans::DownloadSnap::Request::Id;
constexpr DatagramId Trans::DownloadSnap::Response::Id_Success;
constexpr DatagramId Trans::DownloadSnap::Response::Id_NotFound;