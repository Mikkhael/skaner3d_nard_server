#include <tcp_transmission.hpp>
#include <datagrams.hpp>

#include "snap.hpp"

#ifdef SERVER

// Main

void TcpTransSession::awaitNewRequest(){
    if(operationCompletion.setBusy()){
        logErrorLine("Preparing to receive new request without completing hendling previous!");
        return;
    }
    logInfoLine("Awaiting new request.");
    //std::cout << "COUNT BEFORE: " << this->shared_from_this().use_count() << std::endl;
    
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
        //std::cout << "COUNT AFTER: " << me->shared_from_this().use_count() << std::endl;
        me->logInfoLine("Received request with id: ", int(id));
        switch(id){
            case Trans::Echo::Request::Id :         me->receiveEchoRequestHeader(); break;
            case Trans::CustomFile::Request::Id :   me->receiveCustomFileRequestHeader(); break;
            case Trans::SnapFrame::Request::Id :    me->prepareStreamSnapFrame(); break;
            case Trans::DownloadSnap::Request::Id : me->receiveDownloadSnapRequest(); break;
            case Trans::DownloadSnap::Request::Id_CheckIfExists : me->receiveDownloadSnapCheckRequest(); break;
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
    //std::cout << "COUNT RECEIVE: " << shared_from_this().use_count() << std::endl;
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
    
    //std::cout << "COUNT HANDLE: " << shared_from_this().use_count() << std::endl;
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
        buffers_array(
            to_buffer(response),
            asio::buffer(tempBuffer.message)
        ),
        err
    );
    if(err){
        handleError(err, "While sending Echo Request Response.");
        return;
    }
    completeOperation();
}

// File

static uint32_t getRandomId(){
    return 0xFFFF;
} 

void TcpTransSession::startSendFile(){
    if(!tempBufferCollection.is<TempBufferCollection::File>()){
        logErrorLine("Attempted to send File without specifying which file to send. Closing session.");
        operationCompletion.setResult(CustomError::Critical);
        closeSession();
        return;
    }
    auto& tempBuffer = tempBufferCollection.get<TempBufferCollection::File>();
    if(!tempBuffer.istream){
        logErrorLine("Attempted to send File without creating an istream. Closing session.");
        operationCompletion.setResult(CustomError::Critical);
        closeSession();
        return;
    }
    if(tempBuffer.istream->tellg() >= tempBuffer.end){
        logErrorLine("In file send setup: End is not after Begin (", tempBuffer.istream->tellg(), "-", tempBuffer.end, "). Closing session.");
        operationCompletion.setResult(CustomError::Critical);
        closeSession();
        return;
    }
    if(tempBuffer.istream->fail()){
        logErrorLine("Faild accessing istream used for File Send.");
        operationCompletion.setResult(CustomError::FileOpen);
        tempBuffer.fileId = 0;
        completeSendFile(false);
        return;
    }
    
    auto totalSize = tempBuffer.end - tempBuffer.istream->tellg();
    
    Trans::FilePart header;
    tempBuffer.fileId = header.fileId = getRandomId();
    header.partSize = totalSize;
    
    
    logInfoLine("Starting Send File with id ", header.fileId, " and total size ", totalSize);
    Error err;
    asio::write(getSocket(), const_buffers_array(
            to_buffer_const(Trans::FilePart::Id_Start),
            to_buffer_const(header)
        ), err);
    if(err){
        handleError(err, "While starting Send File.");
        return;
    }
    sendFilePart();
}
void TcpTransSession::sendFilePart(){
    auto& tempBuffer = tempBufferCollection.get<TempBufferCollection::File>();
    bool canBeCompleted = true;
    size_t bytesToSend = tempBuffer.end - tempBuffer.istream->tellg();
    if(bytesToSend > buffer.getTotalBufferSize()){
        bytesToSend = buffer.getTotalBufferSize();
        canBeCompleted = false;
    }
    if(!tempBuffer.istream->read(buffer.data(), bytesToSend)){
        logErrorLine("Error while reading from file designated to send.");
        operationCompletion.setResult(CustomError::FileRead);
        completeSendFile(false);
        return;
    }
    
    Trans::FilePart header;
    header.fileId = tempBuffer.fileId;
    header.partSize = bytesToSend;
    logInfoLine("Sending File Part with id ", header.fileId, " and size ", header.partSize);
    Error err;
    asio::write(getSocket(), const_buffers_array(
            to_buffer_const(Trans::FilePart::Id_Part),
            to_buffer_const(header),
            buffer.get(bytesToSend)
        ), err );
    if(err){
        handleError(err, "While sending File Part.");
        return;
    }
    if(canBeCompleted){
        completeSendFile(true);
    }else{
        sendFilePart();
    }
}

void TcpTransSession::completeSendFile(bool successful){
    logInfoLine("Completing File Send with status: ", successful);
    auto& tempBuffer = tempBufferCollection.get<TempBufferCollection::File>();
    Trans::FilePart header;
    header.fileId = tempBuffer.fileId;
    header.partSize = 0;
    Error err;
    asio::write(getSocket(), const_buffers_array(
            to_buffer_const(successful ? 
                                Trans::FilePart::Id_Success :
                                Trans::FilePart::Id_Fail
                            ),
            to_buffer_const(header)
        ), err );
    if(err){
        handleError(err, "While completing File Send");
        return;
    }
    tempBuffer.callback(successful);
}
    
// File Custom

void TcpTransSession::receiveCustomFileRequestHeader(){
    logInfoLine("Receiving Custom File Request Header.");
    asio::async_read(getSocket(), buffer.get(sizeof(Trans::CustomFile::Request)), ASYNC_CALLBACK{
        if(err){
            me->handleError(err, "While receiving Custom File Reqeust Header");
            return;
        }
        me->receiveCustomFileRequestFilepath();
    });
    
}
void TcpTransSession::receiveCustomFileRequestFilepath(){
    Trans::CustomFile::Request request;
    buffer.loadObject(request);
    auto& tempBuffer = tempBufferCollection.set<TempBufferCollection::CustomFile>();
    tempBuffer.path.resize(request.filepathLength);
    tempBuffer.fromEnd = request.fromEnd;
    asio::async_read(getSocket(), asio::buffer(tempBuffer.path), ASYNC_CALLBACK{
       if(err){
           me->handleError(err, "While receiving Custom File Request Filepath");
           return;
       }
       me->prepareCustomFileToSend();
    });
    
}

static std::shared_ptr<std::ifstream> openFileForSend(std::string path, std::streampos& end){
    auto istream = std::make_shared<std::ifstream>();
    istream->open(path, std::fstream::in | std::fstream::binary);
    if(!istream->is_open() || istream->fail()){
        return nullptr;
    }
    istream->seekg(0, std::ifstream::end);
    end = istream->tellg();
    istream->seekg(0, std::ifstream::beg);
    return istream;
}

void TcpTransSession::prepareCustomFileToSend(){
    auto oldTempBuffer = std::move(tempBufferCollection.get<TempBufferCollection::CustomFile>());
    logInfoLine("Preparing to send file ", oldTempBuffer.path, " with fromEnd=", oldTempBuffer.fromEnd);
    
    auto& tempBuffer = tempBufferCollection.set<TempBufferCollection::File>();
    tempBuffer.callback = [me=shared_from_this()](bool){me->completeOperation();};
    auto istream = openFileForSend(oldTempBuffer.path, tempBuffer.end);
    if(!istream){
        logErrorLine("Unable to open specified file to send");
        operationCompletion.setResult(CustomError::FileOpen);
        tempBuffer.fileId = 0;
        completeSendFile(false);
        return;
    }
    if(oldTempBuffer.fromEnd > 0 && oldTempBuffer.fromEnd < tempBuffer.end){
        istream->seekg(tempBuffer.end - static_cast<std::streampos>(oldTempBuffer.fromEnd), std::ifstream::beg);
    }else{
        istream->seekg(0, std::ifstream::beg);
    }
    tempBuffer.istream = istream;
    startSendFile();
}


// Snap Frame

void TcpTransSession::prepareStreamSnapFrame(){
    
    auto stream = std::make_shared<std::stringstream>();
    snapper.snapStream(*stream, [stream, me = shared_from_this()](bool success, std::string error_message){
        if(!success){
            me->logErrorLine("Error while snapping: ", error_message);
            me->operationCompletion.setResult(CustomError::File);
            Trans::FilePart header;
            header.fileId = 0;
            header.partSize = 0;
            Error err;
            asio::write(me->getSocket(), const_buffers_array(
                    to_buffer_const(Trans::FilePart::Id_Fail),
                    to_buffer_const(header)
                ), err);
            if(err){
                me->handleError(err, "While completing failed Snap Frame");
                return;
            }
            me->completeOperation();
            return;
        }
        auto& tempBuffer = me->tempBufferCollection.set<TempBufferCollection::File>();
        tempBuffer.callback = [me](bool){me->completeOperation();};
        stream->seekg(0, std::ios::end);
        tempBuffer.end = stream->tellg();
        stream->seekg(0, std::ios::beg);
        tempBuffer.istream = stream;
        me->startSendFile();
    });
}

// Download Snap

void TcpTransSession::receiveDownloadSnapCheckRequest(){
    logInfoLine("Receiving Download Snap Request Check");
    asio::async_read(getSocket(), buffer.get(sizeof(Trans::DownloadSnap::Request)), ASYNC_CALLBACK{
        if(err){
            me->handleError(err, "While receiving Download Snap Request");
            return;
        }
        Trans::DownloadSnap::Request header;
        me->buffer.loadObject(header);
        me->logInfoLine("Preparing snap file with seriesid ", header.seriesid);
        std::ifstream snapFile(snapper.getSnapFilePathForId(header.seriesid), std::fstream::binary);
        if(!snapFile.good()){
            me->logInfoLine("Snap file with seriesid ", header.seriesid, " not found");
            Error err;
            asio::write(me->getSocket(), to_buffer_const(Trans::DownloadSnap::Response::Id_NotFound), err);
            if(err){
                me->handleError(err, "While sending NotFound response for Snap Download");
                return;
            }
            me->completeOperation();
            return;
        }else{
            me->logInfoLine("Snap file with seriesid ", header.seriesid, " found");
            Error err;
            asio::write(me->getSocket(), to_buffer_const(Trans::DownloadSnap::Response::Id_Success), err);
            if(err){
                me->handleError(err, "While sending Success response for Snap Download");
                return;
            }
            me->completeOperation();
            return;
        }
    });
}


void TcpTransSession::receiveDownloadSnapRequest(){
    logInfoLine("Receiving Download Snap Request");
    asio::async_read(getSocket(), buffer.get(sizeof(Trans::DownloadSnap::Request)), ASYNC_CALLBACK{
        if(err){
            me->handleError(err, "While receiving Download Snap Request");
            return;
        }
        me->prepareSnapToSend();
    });
}

void TcpTransSession::prepareSnapToSend(){
    Trans::DownloadSnap::Request header;
    buffer.loadObject(header);
    logInfoLine("Preparing snap file with seriesid ", header.seriesid);
    std::ifstream snapFile(snapper.getSnapFilePathForId(header.seriesid), std::fstream::binary);
    if(!snapFile.good()){
        logInfoLine("Snap file with seriesid ", header.seriesid, " not found");
        Error err;
        asio::write(getSocket(), to_buffer_const(Trans::DownloadSnap::Response::Id_NotFound), err);
        if(err){
            handleError(err, "While sending NotFound response for Snap Download");
            return;
        }
        completeOperation();
        return;
    }
    snapFile.close();
    Error err;
    asio::write(getSocket(), to_buffer_const(Trans::DownloadSnap::Response::Id_Success), err);
    if(err){
        handleError(err, "While sending Success response for Snap Download");
        return;
    }
    auto& tempBuffer = tempBufferCollection.set<TempBufferCollection::File>();
    tempBuffer.callback = [me = shared_from_this()](bool){me->completeOperation();};
    auto istream = openFileForSend(snapper.getSnapFilePathForId(header.seriesid), tempBuffer.end);
    if(!istream){
        logErrorLine("Unable to open specified file to send");
        operationCompletion.setResult(CustomError::FileOpen);
        tempBuffer.fileId = 0;
        completeSendFile(false);
        return;
    }
    tempBuffer.istream = istream;
    startSendFile();
}


#endif // SERVER
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
#ifdef CLIENT

// Echo

void TcpTransSession::sendEchoRequest(const std::string& message){
    operationCompletion.setBusy();
    Trans::Echo::Request request;
    request.length = message.size();
    Error err;
    logInfoLine("Sending Echo Request with length ", request.length, " and message: ", message);
    asio::write(getSocket(), const_buffers_array(
        to_buffer(Trans::Echo::Request::Id),
        to_buffer_const(request),
        asio::buffer(message)
    ), err);
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
    asio::async_read(getSocket(), asio::buffer(tempBuffer.message), ASYNC_CALLBACK{
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

// File

// void TcpTransSession::startReceiveFile(){
//     logInfoLine("Starting receiving File");
//     asio::async_read(getSocket(), buffer.get(sizeof(DatagramId) + sizeof(Trans::FilePart)), ASYNC_CALLBACK{
//         if(err){
//             me->handleError(err, "While receiving start File Part");
//             return;
//         }
//         DatagramId id;
//         Trans::FilePart header;
//         me->buffer.loadMultiple(id, header);
//         me->logInfoLine("Received File Part Start with id ", int(id), ", fileid ", header.fileId, ", filesize ", header.partSize);
//         if(id == Trans::FilePart::Id_Fail){
//             me->completeReceiveFile(false);
//             return;
//         }
//         if(id != Trans::FilePart::Id_Start){
//             me->logErrorLine("Unexpected Id during start of file transmission: ", int(id), ". Closing session.");
//             me->operationCompletion.setResult(CustomError::Illformated);
//             me->closeSession();
//             return;
//         }
        
//         auto& tempBuffer = me->tempBufferCollection.get<TempBufferCollection::File>();
//         tempBuffer.fileId = header.fileId;
        
//         me->receiveFilePart();
//     });
// }
// void TcpTransSession::receiveFilePart(){
//     logInfoLine("Receiving File Part");
//     asio::async_read(getSocket(), buffer.get(sizeof(DatagramId) + sizeof(Trans::FilePart)), ASYNC_CALLBACK{
//         if(err){
//             me->handleError(err, "While receiving File Part Header");
//             return;
//         }
//         DatagramId id;
//         Trans::FilePart header;
//         me->buffer.loadMultiple(id, header);
//         me->logInfoLine("Received File Part with id ", int(id), ", fileId ", header.fileId, ", partSize ", header.partSize);
        
//         auto& tempBuffer = me->tempBufferCollection.get<TempBufferCollection::File>();
//         if(header.fileId != tempBuffer.fileId){
//             me->logErrorLine("Unexpected fileId during file transmission: ", header.fileId, " (expected ", tempBuffer.fileId, "). Closing session.");
//             me->operationCompletion.setResult(CustomError::Illformated);
//             me->closeSession();
//             return;
//         }
//         if(id == Trans::FilePart::Id_Fail){
//             me->completeReceiveFile(false);
//             return;
//         }
//         if(id == Trans::FilePart::Id_Success){
//             me->completeReceiveFile(true);
//             return;
//         }
//         if(id != Trans::FilePart::Id_Part){
//             me->logErrorLine("Unexpected Id during file transmission: ", int(id), ". Closing session.");
//             me->operationCompletion.setResult(CustomError::Illformated);
//             me->closeSession();
//             return;
//         }
//         if(header.partSize > me->buffer.getTotalBufferSize()){
//             me->logErrorLine("Cannot receive File Part because the part length is bigger than the internal buffer (",
//                                 header.partSize, " > ", me->buffer.getTotalBufferSize(), "). Closing session.");
            
//             me->operationCompletion.setResult(CustomError::Critical);
//             me->closeSession();
//             return;
//         }
//         asio::async_read(me->getSocket(), me->buffer.get(header.partSize), ASYNC_CALLBACK_NESTED{
//             if(err){
//                 me->handleError(err, "While receiving File Part Data");
//                 return;
//             }
//             auto& tempBuffer = me->tempBufferCollection.get<TempBufferCollection::File>();
//             if(!tempBuffer.file.write(me->buffer.data(), bytesTransfered)){
//                 me->logErrorLine("Cannot save received data to local file. Closing session.");
//                 me->operationCompletion.setResult(CustomError::FileWrite);
//                 me->closeSession();
//                 return;
//             }
//             me->receiveFilePart();
//         });
//     });
// }
// void TcpTransSession::completeReceiveFile(bool successful){
//     auto& tempBuffer = tempBufferCollection.get<TempBufferCollection::File>();
//     if(successful){
//         logInfoLine("File Receive completed successfully.");
//     }else{
//         logInfoLine("File Receive failed due to sender's error.");
//     }
//     tempBuffer.callback(successful);
// }


// File Custom

void TcpTransSession::sendCustomFileRequest(const std::string& filepath, uint32_t fromEnd, const std::string& save_filepath, std::fstream::openmode opm){
    
    // operationCompletion.setBusy();
    // auto& tempBuffer = tempBufferCollection.set<TempBufferCollection::File>();
    // tempBuffer.file.open(save_filepath, std::fstream::out | std::fstream::binary | opm);
    // if(!tempBuffer.file.is_open() || !tempBuffer.file){
    //     logErrorLine("Cannot open specified file: ", save_filepath);
    //     operationCompletion.setResult(CustomError::FileOpen);
    //     completeOperation();
    //     return;
    // }
    
    // logInfoLine("Sending Custom File Request for file", filepath, " with fromEnd=", fromEnd);
    // Trans::CustomFile::Request request;
    // request.filepathLength = filepath.size();
    // request.fromEnd = fromEnd;
    // asio::write(getSocket(), const_buffers_array(
    //     to_buffer_const(Trans::CustomFile::Request::Id),
    //     to_buffer_const(request),
    //     asio::buffer(filepath)
    // ), err);
    // if(err){
    //     handleError(err, "While sending Custom File Request");
    //     return;
    // }
    // tempBuffer.callback = [this](bool){completeOperation();};
    // startReceiveFile();
}




#endif // CLIENT




#ifdef SERVER

TcpTransServerService tcpTransServerService;

#endif // SERVER
