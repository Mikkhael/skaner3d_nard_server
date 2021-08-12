//////////// UDP
const dgram = require("dgram");

class UdpSocket{
    constructor(port = 0){
        this.node_socket = dgram.createSocket("udp4");
        
        this.handlers = {
            socket: {
                error: function(err){},
                close: function(){},
                listening: function(){}
            },
            other_error: {
                unknown_id: function(id, rinfo){}
            },
            ping: {
                response: function(rinfo){}
            },
            echo: {
                response: function(message, rinfo){}
            }
        };
        
        this.node_socket.on("error", (err) => {
            this.handlers.socket.error(err);
        });
        this.node_socket.on("listening", () => {
            this.handlers.socket.listening();
        });
        this.node_socket.on("close", () => {
            this.handlers.socket.close();
        });
        this.node_socket.on("message", (buffer, rinfo) => {
            let id = buffer.readUint8(0);
            switch(id){
                case 11: {
                    this.handlers.ping.response(rinfo);
                    break;
                }
                case 13: {
                    const message = buffer.toString('utf8', 1);
                    this.handlers.echo.response(message, rinfo);
                    break;
                }
                default: {
                    this.handlers.other_error.unknown_id(id, rinfo);
                }
            }
        });
        this.setPort(port);
    }
    
    sendPing(address, port){
        let buffer = new Uint8Array([10]);
        this.node_socket.send(buffer, port, address);
    }
    
    sendEcho(address, port, message){
        let buffer = new Uint8Array([12, ...message.split("").map(x => x.charCodeAt(x))]);
        this.node_socket.send(buffer, port, address);
    }
    
    setPort(port){
        this.node_socket.bind(port);
    }
    
    close(){
        this.node_socket.close();
    }
};

//////////// TCP

const net = require("net");

class TcpRead {
    constructor(socket){
        this.socket = socket;
        this.socket.pause();
        
        this.buffer = Buffer.allocUnsafe(0);
        this.bytesRead = 0;
        this.bytesTotal = 0;
        this.isReady = true;
        
        this.leftover_buffer = Buffer.allocUnsafe(0);
        
        this.callback = function(buffer){};
    }
    
    read_some(some_buffer){
        if(this.isReady){
            this.socket.pause();
            this.leftover_buffer = Buffer.concat(this.leftover_buffer, some_buffer);
        }
        const toRead = this.bytesTotal - this.bytesRead;
        this.isReady = (toRead <= some_buffer.length);
        //console.log(`B---1 ${this.bytesRead}, ${this.bytesTotal}, ${this.buffer.length}, ${this.leftover_buffer.length}, ${some_buffer.length}`);
        if(this.isReady){
            this.socket.pause();
        }
        
        some_buffer.copy(this.buffer, this.bytesRead, 0, toRead);
        this.leftover_buffer = some_buffer.slice(toRead);
        
        this.bytesRead += toRead;
        //console.log(`B---2 ${this.bytesRead}, ${this.bytesTotal}, ${this.buffer.length}, ${this.leftover_buffer.length}, ${some_buffer.length}`);
        
        if(this.isReady){
            this.callback(this.buffer);
        }
    }
    
    read(length, callback) {
        this.callback = callback;
        this.bytesRead = 0;
        this.bytesTotal = length;
        this.isReady = false;
        //console.log(`B---0 ${length}`);
        if(this.buffer.length < length){
            this.buffer = Buffer.allocUnsafe(length);
        }
        if(this.leftover_buffer.length > 0){
            //console.log("B--- using leftover");
            this.read_some(this.leftover_buffer);
            if(this.isReady){
                return;
            }
        }
        this.socket.resume();
    }
};

class TcpConnection{
    constructor(){
        this.node_socket = net.Socket();
        
        this.handlers = {
            socket: {
                error: function(err){},
                close: function(){},
                connect: function(){}
            },
            other_error: {
            },
            echo: {
                response: function(message, rinfo){}
            },
            file: {
                start: function(totalSize, next){},
                part: function(buffer, next){},
                complete: function(){},
                server_error: function(){},
                format_error: function(){}
            }
        };
        
        this.node_socket.on("error", (err) => {
            this.handlers.socket.error(err);
        });
        this.node_socket.on("connect", () => {
            this.handlers.socket.connect();
        });
        this.node_socket.on("close", () => {
            this.handlers.socket.close();
        });
        
        this._tcpReadBuffer = new TcpRead(this.node_socket);
        
        this.node_socket.on("data", (buffer) => {
            this._tcpReadBuffer.read_some(buffer);
        });
    }
    
    getAddress(){
        return this.node_socket.remoteAddress;
    }
    getPort(){
        return this.node_socket.remotePort;
    }
    
    
    connect(host, port){
        this.node_socket.connect(port, host);
    }
    disconnect(){
        this.node_socket.destroy();
    }
    
    sendEcho(message){
        let buffer = Buffer.allocUnsafe(1+4+message.length);
        buffer.writeUInt8(12, 0);
        buffer.writeUInt32LE(message.length, 1);
        buffer.write(message, 5);
        this.node_socket.write(buffer);
        this._receiveEchoLength();
    }
    sendCustomFileRequest(server_file){
        let buffer = Buffer.allocUnsafe(1+4+4+server_file.length);
        buffer.writeUInt8(130, 0);
        buffer.writeUInt32LE(server_file.length, 1);
        buffer.writeUInt32LE(0, 5);
        buffer.write(server_file, 9);
        this.node_socket.write(buffer);
        this._receiveFilePart(0);
    }
    
    _receiveEchoLength(){
        this._tcpReadBuffer.read(4, buffer => {
            let length = buffer.readUInt32LE(0);
            this._receiveEchoMessage(length);
        });
    }
    _receiveEchoMessage(length){
        this._tcpReadBuffer.read(length, buffer => {
            let message = buffer.toString();
            this.handlers.echo.response(message);
        });
    }
    
    _receiveFilePart(expectedFileid){
        this._tcpReadBuffer.read(1+4+4, buffer => {
            let id = buffer.readUint8(0);
            let fileid = buffer.readUInt32LE(1);
            let size = buffer.readUInt32LE(5);
            
            let next = () => {
                this._receiveFilePart(fileid);
            };
            
            if(id == 120){
                this.handlers.file.start(size, next);
                return;
            }
            if(id == 123){
                this.handlers.file.server_error();
                return;
            }
            
            if(expectedFileid != fileid){
                console.log(`E: ${expectedFileid}, ${fileid}`);
                this.handlers.file.format_error();
                return;
            }
            
            if(id == 121){
                this._tcpReadBuffer.read(size, data => {
                    this.handlers.file.part(data, next);
                });
                return;
            }else if(id == 122){
                this.handlers.file.complete();
                return;
            }else{
                console.log(`E2: ${id}`);
                this.handlers.file.format_error();
                return;
            }
        });
    }
    
}

////////////
module.exports = {
    UdpSocket,
    TcpConnection
};


