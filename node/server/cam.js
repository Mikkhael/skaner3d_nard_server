// @ts-check

const WebSocket = require("ws");
const skanernet = require("../skaner3d_networking.js");
const cfg = require("./config.js");
const fs = require("fs");

/////////////////////////////////////
// Flop Class

class Flop {
    constructor() {
        this.buffer;
        this.index = 0;
        this.callback = (/**@type {Boolean} */_success, /**@type {Buffer} */_data) => {};
    }
    
    /**
     * @param {Flop} _self 
     * @param {number} _totalFileSize 
     * @param {function():void} _next 
     */
    start(_self, _totalFileSize, _next) {
        //console.log(`Received file start.`);
        this.buffer = Buffer.alloc(_totalFileSize);
        this.index = 0;
        _next();
    }
    
    /**
     * @param {Flop} _self 
     * @param {Buffer} _data 
     * @param {function():void} _next 
     */
    part(_self, _data, _next) {
        //console.log(`Received file part.`);
        _data.copy(this.buffer, this.index);
        this.index += _data.length;
        _next();
    }
    
    /**
     * @param {Flop} _self 
     */
    complete(_self) {
        //console.log(`Completed file transfer.`);
        if (this.callback != undefined) {
            this.callback(true, this.buffer);
        }
    }
    
    /**
     * @param {string} message
     * @param {Flop} _self 
     */
    errorWithLog(message, _self){
        console.log(`File error: `, message);
        if (this.callback != undefined) {
            this.callback(false);
        }
    }
    
    /**
     * @param {Flop} _self 
     */
    error(_self) {
        console.log(`File error.`);
        if (this.callback != undefined) {
            this.callback(false);
        }
    }
}

/////////////////////////////////////
// Camera Class

class Camera {
        
    /** @typedef {function(Camera, number, boolean, Buffer=):void} FileCallback */
    /** @typedef {{id: number, callback: FileCallback}} FileRequest */
    
    constructor(/**@type {string} */ _address, /**@type {number} */ _port) {
        this.address = _address;
        this.port = _port;
        this.sockets = {
            stream: this.createSocket(_address, _port),
            file:this.createSocket(_address, _port)
        }
        this.sockets.stream.flop.callback = this.frameTransferComplete.bind(this);
        this.sockets.file.flop.callback = this.fileTransferComplete.bind(this);

        this.triedReconnecting = false;
        
        ///**@type {FileRequest[]} */
        //this.fileQueue = [];
        /**@type {WebSocket[]} */
        this.streamList = [];
        /**@type {FileRequest} */
        this.currentFileRequest;
        
        /** @type {boolean} */
        this.streamDone = true;
    }
    
    /**
     * @param {boolean} _success 
     * @param {Buffer} _data 
     */
    fileTransferComplete(_success, _data) {
        //this.fileQueue.shift();
        /** @type {FileRequest} */
        let request = this.currentFileRequest
        this.currentFileRequest = undefined;
        request.callback(this, request.id, _success, _data);
        
    }
    
    /**
     * @param {number} _id 
     * @param {FileCallback} _callback 
     */
    requestFile(_id, _callback) {
        this.currentFileRequest = {id: _id, callback: _callback};
        this.sockets.file.sendDownloadSnapRequest(_id);
    }
    
    /**
     * @param {boolean} _success 
     * @param {Buffer} _data 
     */
    frameTransferComplete(_success, _data) {
        this.streamDone = true;
        if (_success) {
            let packet = JSON.stringify({msg: "frame", buffer: _data.toString("binary")});
            this.streamList.forEach((x)=>{
                if (x.readyState == 1) {
                    x.send(packet);
                } else {
                    this.streamList.splice(this.streamList.indexOf(x), 1);
                }
            });
        }
    }
    
    requestStreamFrame() {
        if (this.streamList.length > 0) {
            if(!this.streamDone){
                return;
            }
            this.streamDone = false;
            this.sockets.stream.sendSnapFrameRequest();
        }
    }
    
    /** @param {WebSocket} _viewer */
    subscribeStream(_viewer) {
        this.streamList.push(_viewer);
    }
    
    /** @param {WebSocket} _viewer */
    unsubscribeStream(_viewer) {
        if (this.streamList.indexOf(_viewer) != -1) {
            this.streamList.splice(this.streamList.indexOf(_viewer), 1);
        }
    }

    reconnect(_bypass) {
        if (this.triedReconnecting && !_bypass) return;
        console.log(`Trying to reconnect sockets[]`);
        if (!this.sockets.file.isConnected)
            this.sockets.file.connect(this.address, this.port);
        if (!this.sockets.stream.isConnected)
            this.sockets.stream.connect(this.address, this.port);
        this.triedReconnecting = true;
        this.streamDone = true;
    }
    
    /**
     * @param {string} _address 
     * @param {number} _port
     */
    createSocket(_address, _port) {
        /**@type {skanernet.TcpConnection & {flop?: Flop}} */
        let socket = new skanernet.TcpConnection();
        socket.flop = new Flop();
        socket.isConnected = false;
        socket.handlers.socket.error      = (_self, _err) => {console.log(`TCP Socket error occured: ${_err}`)};
        socket.handlers.socket.close      = (_self) => {console.log(`TCP Socket closed.`); _self.isConnected = false; this.reconnect();};
        socket.handlers.socket.connect    = (_self) => {/*console.log(`TCP Socket connected.`);*/ this.triedReconnecting = false; _self.isConnected = true; if (this.currentFileRequest != undefined) {this.sockets.file.sendDownloadSnapRequest(this.currentFileRequest.id);}};
        socket.handlers.echo.response     = (_self, _message) => {console.log(`Received Echo response from ${_self.getAddress()}:${_self.getPort()} with message: ${_message}`)};
        socket.handlers.file.start        = socket.flop.start.bind(socket.flop);
        socket.handlers.file.part         = socket.flop.part.bind(socket.flop);
        socket.handlers.file.complete     = socket.flop.complete.bind(socket.flop);
        socket.handlers.file.server_error = socket.flop.errorWithLog.bind(socket.flop, "Server Error.");
        socket.handlers.file.format_error = socket.flop.errorWithLog.bind(socket.flop, "SnapStream Format Error.");
        socket.handlers.downloadSnap.response = (_self, _found, _next)=>{
            console.log(`Download snap response: found[${_found}].`);
            if (!_found) {
                socket.handlers.file.server_error(_self);
            }
            _next();
        };
        socket.handlers.downloadSnap.format_error = socket.flop.errorWithLog.bind(socket.flop, "DownloadSnap Format Error.");
        socket.connect(_address, _port);
        return socket;
    }
    
    get info() {
        return {
            address: this.address,
            port: this.port,
            status:
            {
                stream: this.sockets.stream.isConnected,
                file: this.sockets.file.isConnected
            }
        };
    }
}

/////////////////////////////////////
// Camera Center

class CameraCenter {
    /** @typedef {{id: number, cameraId: string}} DownloadRequest */

    constructor() {
        /**@type {Object.<string, Camera>} */
        this.cameras = {};
        /**@type {Camera[]} */
        this.cameraList = [];
        /**@type {DownloadRequest[]} */
        this.downloadQueue = [];
        
        this.maxDownloads = 3;

        this.address = cfg.ip;
        this.ports = [8888, 8889];

        this.udp = new skanernet.UdpSocket();

        this.udp.handlers.ping.response = (rinfo) => {
            //console.log(`Received Pong from ${rinfo.address}:${rinfo.port}`);
            this.register(rinfo.address, rinfo.port);
        };
    }

    /**@param {function(string):void} _callback */
    updateCameras(_callback) {
        this.ports.forEach((x)=>{this.udp.sendPing(this.address, x)});
        if (_callback != undefined) {
            setTimeout( ()=>{
                let packet = {msg: "cameras", cameras: []};
                this.cameraList.forEach((x)=>{packet.cameras.push(x.info)});
                _callback(JSON.stringify(packet));
            }, 100);
        }
    }
    /**
     * @param {string} _address 
     * @param {number} _port 
     */
    register(_address, _port) {
        if (this.cameras[`${_address}:${_port}`] == undefined) {
            let camera = new Camera(_address, _port);
            this.cameras[`${_address}:${_port}`] = camera;
            this.cameraList.push(camera);
        } else {
            this.cameras[`${_address}:${_port}`].reconnect(true);
        }
    }
    
    /**@param {number} _id */
    shoot(_id) {
        this.ports.forEach((x)=>{this.udp.sendSnap(this.address, x, _id)});
        setTimeout(()=>{
            for (let i in this.cameraList) {
                this.downloadQueue.push({id: _id, cameraId: i});
            }
            this.processDownloadQueue();
        }, 500);
    }
    
    /**
     * @param {Camera} _camera 
     * @param {number} _id 
     * @param {boolean} _success 
     * @param {Buffer} _data 
     */
    fileDownloadComplete(_camera, _id, _success, _data) {
        console.log(`Downloaded file with id [${_id}] form Camera[${this.cameraList.indexOf(_camera)}]. Success [${_success}].`);

        if (_success) {
            let filepath = `snaps\\${_id}_${this.cameraList.indexOf(_camera)}.png`;
            fs.open(filepath, "w", (err, fd) => {
                if(err){
                    console.log(`Cannot open file ${filepath}.`);
                } else {
                    fs.write(fd, _data, (err)=>{
                        if(err){
                            console.log(`File system error occured: ${err}`);
                        } else {
                            fs.closeSync(fd);
                        }
                    });
                }
            });
        }

        this.processDownloadQueue();
    }

    processDownloadQueue() {
        let candidateQueue = this.downloadQueue.filter((x)=>(this.cameraList[x.cameraId].currentFileRequest === undefined && this.downloadQueue.find(y => y.cameraId == x.cameraId) === x));
        let limit = this.maxDownloads - this.cameraList.filter((x)=>(x.currentFileRequest !== undefined)).length;
        let newDownloads = candidateQueue.splice(0, limit );
        //this.downloadQueue = this.downloadQueue.filter((x)=>(newDownloads.indexOf(x) === -1));
        newDownloads.forEach((x)=>{
            this.downloadQueue.splice(this.downloadQueue.indexOf(x), 1);
            this.cameraList[x.cameraId].requestFile(x.id, this.fileDownloadComplete.bind(this));
        });
    }
    
    requestStreamFrame() {
        this.cameraList.forEach((x)=>{
            x.requestStreamFrame();
        });
    }
    
    /**@param {WebSocket} _socket */
    unsubscribeAll(_socket) {
        this.cameraList.forEach((x)=>{x.unsubscribeStream(_socket)});
    }
    
    /**@param {Camera} _camera */
    unregister(_camera) {
        console.log(`Camera at ${_camera.address}:${_camera.port} has disconnected.`);
        this.cameraList.splice(this.cameraList.indexOf(_camera), 1);
        delete( this.cameras[`${_camera.address}:${_camera.port}`] );
    }
}

/////////////////////////////////////
module.exports = {
    CameraCenter
};