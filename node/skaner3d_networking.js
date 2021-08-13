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
        
        this.clear();
    }
    
    clear(){
        this.buffer = Buffer.allocUnsafe(0);
        this.bytesRead = 0;
        this.bytesTotal = 0;
        this.isReady = true;
        
        this.leftover_buffer = Buffer.allocUnsafe(0);
        
        this.callback = function(buffer){};
    }
    
    read_some(some_buffer){
        if(this.isReady){
            console.log("BUFF slow");
            this.socket.pause();
            this.leftover_buffer = Buffer.concat(this.leftover_buffer, some_buffer);
            return;
        }
        const bytesLeft = this.bytesTotal - this.bytesRead;
        const canBeCompleted = (bytesLeft <= some_buffer.length);
        console.log("BUFF left ", bytesLeft, some_buffer.length, canBeCompleted);
        if(canBeCompleted){
            console.log("BUFF pausing ");
            this.socket.pause();
        }
        //console.log("BUFF before ", this.leftover_buffer.length, this.bytesRead);
        if(bytesLeft > some_buffer.length){
            some_buffer.copy(this.buffer, this.bytesRead);
            this.bytesRead += some_buffer.length;
        }else{
            some_buffer.copy(this.buffer, this.bytesRead, 0, bytesLeft);
            this.leftover_buffer = some_buffer.slice(bytesLeft);
            this.bytesRead += bytesLeft;
        }
        console.log("BUFF after readsome ", this.bytesRead, this.leftover_buffer.length);
        if(canBeCompleted){
            console.log("BUFF completing ");
            this.isReady = true;
            this.callback(this.buffer.slice(0, this.bytesTotal));
        }
    }
    
    read(length, callback) {
        console.log("BUFF toRead ", length);
        this.isReady = false;
        this.bytesRead = 0;
        this.bytesTotal = length;
        if(this.buffer.length < length){
            console.log("BUFF resize ", this.buffer.length, length);
            this.buffer = Buffer.allocUnsafe(length);
        }
        this.callback = callback;
        
        if(this.leftover_buffer.length >= length){
            console.log("BUFF leftover all");
            this.read_some(this.leftover_buffer);
            //this.leftover_buffer = this.leftover_buffer.slice(length);
            return;
        }
        if(this.leftover_buffer.length > 0){
            console.log("BUFF leftover");
            this.read_some(this.leftover_buffer);
            this.leftover_buffer = Buffer.allocUnsafe(0);
        }
        console.log("BUFF after init read ", this.bytesRead, this.leftover_buffer.length);
        
        console.log("BUFF resume");
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
        
        this._tcpReadBuffer = new TcpRead(this.node_socket);
        
        this.node_socket.on("error", (err) => {
            this._tcpReadBuffer.clear();
            this.handlers.socket.error(err);
        });
        this.node_socket.on("connect", () => {
            this.node_socket.pause();
            this.handlers.socket.connect();
        });
        this.node_socket.on("close", () => {
            this._tcpReadBuffer.clear();
            this.handlers.socket.close();
        });
        
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
        this._tcpReadBuffer.clear();
        this.node_socket.connect(port, host);
    }
    disconnect(){
        this.node_socket.destroy();
        this._tcpReadBuffer.clear();
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
    sendSnapFrameRequest(){
        this.node_socket.write(new Uint8Array([140]));
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
                console.log(`E: ${expectedFileid}, ${fileid}, (${id})`);
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


