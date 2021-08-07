// #include "asiolib/all.hpp"


// class TcpTransmissionSession : public BasicTcpSession<TcpTransmissionSession>
// {
// protected:

//     template <class ...Ts>
//     void logInfoLine(const Ts& ...args){
//         logger.logInfoLine("TCP TRANS ", remoteEndpoint, ' ', args...);
//     }
//     template <class ...Ts>
//     void logErrorLine(const Ts& ...args){
//         logger.logErrorLine("TCP TRANS ", remoteEndpoint, ' ', args...);
//     }
    
//     virtual void HandleError_CannotResolve(const Error& err) {
//         logErrorLine("Cannot resolve specified address: ", err);
//     };
//     virtual void HandleError_CannotConnect(const Error& err) {
//         logErrorLine("Cannot connect to specified address", err);
//     };
//     virtual void HandleError_CannotGetRemoteEndpoint(const Error& err) {
//         logErrorLine("Cannot read remote endpoint of socket: ", err);
//     };
    
//     void defaultErrorHandler(const Error& err){
//        logErrorLine("Asio Error: ", err);
//        closeSession();
//     }
    
//     ArrayBuffer<512> buffer;
    
// public:
    
    
//     void awaitNewRequest();
    
//     virtual void startSession(){
//         awaitNewRequest();
//     }
    
// };


// class TcpTransmissionServer : public BasicTcpServer<TcpTransmissionSession>
// {    
// protected:
    
//     virtual void HandleError_Connection(const Error& err) override {
//         logger.logErrorLine("TCP TRANS ", "Cannot connect to client: ", err );
//     };
//     virtual void HandleError_CannotOpenAcceptor(const Error& err) override {
//         logger.logErrorLine("TCP TRANS ", "Cannot open acceptor: ", err  );
//     };
//     virtual void HandleError_CannotBindAcceptor(const Error& err) override {
//         logger.logErrorLine("TCP TRANS ", "Cannot bind acceptor: ", err );
//     };
//     virtual void HandleError_GettingRemoteEndpoint(const Error& err) override {
//         logger.logErrorLine("TCP TRANS ", "Cannot get remote endpoint of client: ", err );
//     };
// };

// class TcpTransmissionService{
//     std::optional<TcpTransmissionServer> m_server;
//     std::list<TcpTransmissionSession> m_clients;
    
//     Error err;
    
// public:
//     auto& server() {return *m_server;}
//     auto& clients() {return m_clients;}
    
//     //auto& client() {return *m_client;}
    
//     auto getError() {return err;}
    
//     bool start(asio::io_context& ioContext, const int port){
//         m_server.emplace(ioContext);
//         if(!m_server->startServer(port)){
//             err = m_server->getError();
//             return false;
//         }
//         return true;
//     }
// };


// inline TcpTransmissionService tcpTransmissionService;
