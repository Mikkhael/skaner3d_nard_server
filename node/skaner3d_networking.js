// @ts-check

//////////// UDP
const dgram = require("dgram");

/**
 * 
 * @param {string} string 
 */
function StringToIp(string){
    const base = 256;
    let parts = string.split('.').map(x => parseInt(x));
    let res = 0;
    let pow = 1;
    res += parts[3] * pow; pow *= base;
    res += parts[2] * pow; pow *= base;
    res += parts[1] * pow; pow *= base;
    res += parts[0] * pow;
    if(!Number.isInteger(res)){
        throw "ayaya";
    }
    return res;
}

class UdpSocket{
    constructor(port = 0){
        /** @type dgram.Socket */
        this.node_socket = dgram.createSocket("udp4");
        
        
        this.handlers = {
            socket: {
                /** @param {Error} err  */
                error: function(err){},
                close: function(){},
                listening: function(){}
            },
            other_error: {
                /** @param {number} id  @param {dgram.RemoteInfo} rinfo */
                unknown_id: function(id, rinfo){}
            },
            ping: {
                /** @param {dgram.RemoteInfo} rinfo */
                response: function(rinfo){}
            },
            echo: {
                /** @param {string} message  @param {dgram.RemoteInfo} rinfo */
                response: function(message, rinfo){}
            },
            config: {
                network: {
                    set: {
                        /** @param {boolean} status  @param {dgram.RemoteInfo} rinfo */
                        response: function(status, rinfo){}
                    }
                },
                device: {
                    set: {
                        /** @param {boolean} status  @param {dgram.RemoteInfo} rinfo */
                        response: function(status, rinfo){}
                    },
                    get: {
                        /** @param {string} value  @param {dgram.RemoteInfo} rinfo */
                        response: function(value, rinfo){}
                    }
                }
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
            let id = buffer.readUInt8(0);
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
                case 101: {
                    this.handlers.config.network.set.response(true, rinfo);
                    break;
                }
                case 102:{
                    this.handlers.config.network.set.response(false, rinfo);
                    break;
                }
                case 111: {
                    this.handlers.config.device.set.response(true, rinfo);
                    break;
                }
                case 112:{
                    this.handlers.config.device.set.response(false, rinfo);
                    break;
                }
                case 121: {
                    const message = buffer.slice(1).toString();
                    this.handlers.config.device.get.response(message, rinfo);
                    break;
                }
                default: {
                    this.handlers.other_error.unknown_id(id, rinfo);
                }
            }
        });
        this.setPort(port);
    }
    
    /** @param {string} address @param {number} port */
    sendPing(address, port){
        let buffer = new Uint8Array([10]);
        this.node_socket.send(buffer, port, address);
    }
    
    /** @param {string} address @param {number} port @param {string} message*/
    sendEcho(address, port, message){
        let buffer = new Uint8Array([12, ...message.split("").map(x => x.charCodeAt(0))]);
        this.node_socket.send(buffer, port, address);
    }
    /**
     * 
     * @param {string} address 
     * @param {number} port 
     * @param {string} ip 
     * @param {string} mask 
     * @param {string} gateway 
     * @param {string} dns1 
     * @param {string} dns2 
     * @param {boolean} isDynamic 
     */
    sendConfigNetworkSet(address, port, ip, mask, gateway, dns1, dns2, isDynamic){
        let buffer = Buffer.alloc(1+6*4);
        buffer.writeUInt8(100, 0);
        buffer.writeUInt32LE(StringToIp(ip), 1);
        buffer.writeUInt32LE(StringToIp(mask), 5);
        buffer.writeUInt32LE(StringToIp(gateway), 9);
        buffer.writeUInt32LE(StringToIp(dns1), 13);
        buffer.writeUInt32LE(StringToIp(dns2), 17);
        buffer.writeUInt32LE(isDynamic ? 1 : 0, 21);
        this.node_socket.send(buffer, port, address);
    }
    
    /** @param {string} address @param {number} port @param {string} config*/
    sendConfigDeviceSet(address, port, config){
        let buffer = Buffer.alloc(1+config.length);
        buffer.writeUInt8(110, 0);
        buffer.write(config, 1);
        this.node_socket.send(buffer, port, address);
    }
    
    /** @param {string} address @param {number} port */
    sendConfigDeviceGet(address, port){
        let buffer = new Uint8Array([120]);
        this.node_socket.send(buffer, port, address);
    }
    
    /** @param {string} address @param {number} port */
    sendReboot(address, port){
        let buffer = new Uint8Array([130]);
        this.node_socket.send(buffer, port, address);
    }
    
    /** @param {string} address @param {number} port @param {number} seriesid*/
    sendSnap(address, port, seriesid){
        let buffer = Buffer.alloc(1+4);
        buffer.writeUInt8(200, 0);
        buffer.writeUInt32LE(seriesid, 1);
        this.node_socket.send(buffer, port, address);
    }
    
    /** @param {string} address @param {number} port */
    sendDeleteAllSnaps(address, port){
        let buffer = new Uint8Array([222]);
        this.node_socket.send(buffer, port, address);
    }
    
    /** @param {number} port*/
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
    /** @param {net.Socket} socket */
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
        
        /** @type {Buffer} */
        this.leftover_buffer = Buffer.allocUnsafe(0);
        
        /** @param {Buffer} buffer */
        this.callback = function(buffer){};
    }
    
    /** @param {Buffer} some_buffer */
    read_some(some_buffer){
        if(this.isReady){
            //console.log("BUFF slow");
            this.socket.pause();
            this.leftover_buffer = Buffer.concat([this.leftover_buffer, some_buffer]);
            return;
        }
        const bytesLeft = this.bytesTotal - this.bytesRead;
        const canBeCompleted = (bytesLeft <= some_buffer.length);
        //console.log("BUFF left ", bytesLeft, some_buffer.length, canBeCompleted);
        if(canBeCompleted){
            //console.log("BUFF pausing ");
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
        //console.log("BUFF after readsome ", this.bytesRead, this.leftover_buffer.length);
        if(canBeCompleted){
            //console.log("BUFF completing ");
            this.isReady = true;
            this.callback(this.buffer.slice(0, this.bytesTotal));
        }
    }
    
    /** @param {number} length @param {(Buffer)=>void} callback */
    read(length, callback) {
        //console.log("BUFF toRead ", length);
        this.isReady = false;
        this.bytesRead = 0;
        this.bytesTotal = length;
        if(this.buffer.length < length){
            //console.log("BUFF resize ", this.buffer.length, length);
            this.buffer = Buffer.allocUnsafe(length);
        }
        this.callback = callback;
        
        if(this.leftover_buffer.length >= length){
            //console.log("BUFF leftover all");
            this.read_some(this.leftover_buffer);
            //this.leftover_buffer = this.leftover_buffer.slice(length);
            return;
        }
        if(this.leftover_buffer.length > 0){
            //console.log("BUFF leftover");
            this.read_some(this.leftover_buffer);
            this.leftover_buffer = Buffer.allocUnsafe(0);
        }
        //console.log("BUFF after init read ", this.bytesRead, this.leftover_buffer.length);
        
        //console.log("BUFF resume");
        this.socket.resume();
    }
};

class TcpConnection{
    constructor(){
        /** @type {net.Socket} */
        this.node_socket = new net.Socket();
        
        this.isConnected = false;
        
        this.handlers = {
            socket: {
                
                /** @param {TcpConnection} tcp @param {Error} err*/
                error: function(tcp, err){},
                /** @param {TcpConnection} tcp*/
                close: function(tcp){},
                /** @param {TcpConnection} tcp*/
                connect: function(tcp){}
            },
            other_error: {
            },
            echo: {
                /** @param {TcpConnection} tcp @param {string} message*/
                response: function(tcp, message){}
            },
            file: {
                /** @param {TcpConnection} tcp @param {number} totalSize @param {()=>void} next*/
                start: function(tcp, totalSize, next){},
                /** @param {TcpConnection} tcp @param {Buffer} buffer @param {()=>void} next*/
                part: function(tcp, buffer, next){},
                /** @param {TcpConnection} tcp*/
                complete: function(tcp){},
                /** @param {TcpConnection} tcp*/
                server_error: function(tcp){},
                /** @param {TcpConnection} tcp*/
                format_error: function(tcp){}
            },
            downloadSnap: {
                /** @param {TcpConnection} tcp @param {boolean} found @param {()=>void} next */
                response: function(tcp, found, next){},
                /** @param {TcpConnection} tcp*/
                format_error: function(tcp){}
            }
        };
        
        /** @type {TcpRead} */
        this._tcpReadBuffer = new TcpRead(this.node_socket);
        
        this.node_socket.on("error", (err) => {
            this.isConnected = false;
            this._tcpReadBuffer.clear();
            this.handlers.socket.error(this, err);
        });
        this.node_socket.on("connect", () => {
            this.isConnected = true;
            this.node_socket.pause();
            this.handlers.socket.connect(this);
        });
        this.node_socket.on("close", () => {
            this.isConnected = false;
            this._tcpReadBuffer.clear();
            this.handlers.socket.close(this);
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
    
    /** @param {string} host @param {number} port */
    connect(host, port){
        this._tcpReadBuffer.clear();
        this.node_socket.connect(port, host);
    }
    disconnect(){
        this.isConnected = false;
        this.node_socket.end();
        this._tcpReadBuffer.clear();
    }
    
    /** @param {string} message*/
    sendEcho(message){
        let buffer = Buffer.allocUnsafe(1+4+message.length);
        buffer.writeUInt8(12, 0);
        buffer.writeUInt32LE(message.length, 1);
        buffer.write(message, 5);
        this.node_socket.write(buffer);
        this._receiveEchoLength();
    }
    /** @param {string} server_file*/
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
    
    /** @param {number} seriesid*/
    sendDownloadSnapRequest(seriesid, onlyCheck = false){
        let buffer = Buffer.alloc(1+4);
        const id = onlyCheck ? 205 : 200;
        buffer.writeUInt8(id, 0);
        buffer.writeUInt32LE(seriesid, 1);
        this.node_socket.write(buffer);
        this._receiveDownloadSnapResponse(onlyCheck);
    }
    /** @param {number} seriesid*/
    sendDownloadSnapRequestCheck(seriesid){
        return this.sendDownloadSnapRequest(seriesid, true);
    }
    
    _receiveEchoLength(){
        this._tcpReadBuffer.read(4, buffer => {
            let length = buffer.readUInt32LE(0);
            this._receiveEchoMessage(length);
        });
    }
    /** @param {number} length*/
    _receiveEchoMessage(length){
        this._tcpReadBuffer.read(length, buffer => {
            let message = buffer.toString();
            this.handlers.echo.response(this, message);
        });
    }
    
    /** @param {number} expectedFileid*/
    _receiveFilePart(expectedFileid){
        this._tcpReadBuffer.read(1+4+4, buffer => {
            let id = buffer.readUint8(0);
            let fileid = buffer.readUInt32LE(1);
            let size = buffer.readUInt32LE(5);
            
            let next = () => {
                this._receiveFilePart(fileid);
            };
            
            if(id == 120){
                this.handlers.file.start(this, size, next);
                return;
            }
            if(id == 123){
                this.handlers.file.server_error(this);
                return;
            }
            
            if(expectedFileid != fileid){
                //console.log(`E: ${expectedFileid}, ${fileid}, (${id})`);
                this.handlers.file.format_error(this);
                return;
            }
            
            if(id == 121){
                this._tcpReadBuffer.read(size, data => {
                    this.handlers.file.part(this, data, next);
                });
                return;
            }else if(id == 122){
                this.handlers.file.complete(this);
                return;
            }else{
                //console.log(`E2: ${id}`);
                this.handlers.file.format_error(this);
                return;
            }
        });
    }
    
    _receiveDownloadSnapResponse(onlyCheck = false){
        this._tcpReadBuffer.read(1, buffer => {
            const res = buffer.readUint8(0);
            if(res == 201){
                const next = onlyCheck ? (() => {}) : ( () => { this._receiveFilePart(0); } );
                this.handlers.downloadSnap.response(this, true, next);
            }else if(res == 202){
                this.handlers.downloadSnap.response(this, false, () => {});
            }else{
                this.handlers.downloadSnap.format_error(this);
            }
        });
    }
    
}

////////////
module.exports = {
    UdpSocket,
    TcpConnection
};


