const skanernet = require("../skaner3d_networking.js");

/////////////////////////////////////
// Flop Class

class Flop {
    constructor() {
        this.buffer;
        this.index = 0;
        this.callback;
    }
    
    start(_self, _totalFileSize, _next) {
        //console.log(`Received file start.`);
        this.buffer = Buffer.alloc(_totalFileSize);
        this.index = 0;
        _next();
    }
    
    part(_self, _data, _next) {
        //console.log(`Received file part.`);
        _data.copy(this.buffer, this.index);
        this.index += _data.length;
        _next();
    }
    
    complete(_self) {
        //console.log(`Completed file transfer.`);
        if (this.callback != undefined) {
            this.callback(true, this.buffer);
        }
    }
    
    errorWithLog(message, _self){
        console.log(`File error: `, message);
        if (this.callback != undefined) {
            this.callback(false);
        }
    }
    
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
    constructor(_address, _port) {
        this.address = _address;
        this.port = _port;
        this.sockets = {
            stream: this.createSocket(_address, _port),
            file: this.createSocket(_address, _port)
        }
        this.sockets.stream.flop.callback = this.frameTransferComplete.bind(this);
        this.sockets.file.flop.callback = this.fileTransferComplete.bind(this);
        
        this.fileQueue = [];
        this.streamList = [];
        this.currentFileRequest;
        
        this.streamDone = true;
    }
    
    fileTransferComplete(_success, _data) {
        this.fileQueue.shift();
        if (_success) {
            this.currentFileRequest.callback(this, this.currentFileRequest.id, _success, _data);
        } else {
            this.currentFileRequest.callback(this, this.currentFileRequest.id, _success);
        }
    }
    
    processFileQueue() {
        if (this.fileQueue.length > 0) {
            this.currentFileRequest = this.fileQueue[0];
            this.sockets.file.sendDownloadSnapRequest(this.currentFileRequest.id);
            return true;
        } else {
            return false;
        }
    }
    
    requestFile(_id, _callback) {
        console.log(`File request id[${_id}]`);
        setTimeout(()=>{
            this.fileQueue.push({id: _id, callback: _callback});
        },500);
    }
    
    
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
    
    subscribeStream(_viewer) {
        this.streamList.push(_viewer);
    }
    
    unsubscribeStream(_viewer) {
        if (this.streamList.indexOf(_viewer) != -1) {
            this.streamList.splice(this.streamList.indexOf(_viewer), 1);
        }
    }
    
    createSocket(_address, _port) {
        let socket = new skanernet.TcpConnection();
        socket.flop = new Flop();
        socket.isConnected = false;
        socket.handlers.socket.error      = (_self, _err) => {console.log(`TCP Socket error occured: ${_err}`)};
        socket.handlers.socket.close      = (_self) => {console.log(`TCP Socket closed.`); _self.isConnected = false};
        socket.handlers.socket.connect    = (_self) => {_self.isConnected = true};
        socket.handlers.echo.response     = (_self, _message) => {console.log(`Received Echo response from ${_self.getAddress()}:${_self.getPort()} with message: ${_message}`)};
        socket.handlers.file.start        = socket.flop.start.bind(socket.flop);
        socket.handlers.file.part         = socket.flop.part.bind(socket.flop);
        socket.handlers.file.complete     = socket.flop.complete.bind(socket.flop);
        socket.handlers.file.server_error = socket.flop.errorWithLog.bind(socket.flop, "Server Error.");
        socket.handlers.file.format_error = socket.flop.errorWithLog.bind(socket.flop, "SnapStream Format Error.");
        socket.handlers.downloadSnap.response = (_self, _found, _next)=>{
            console.log(`Download snap response: found[${_found}].`);
            if (!_found) {
                console.log(`Snap not found`);
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

    constructor() {
        this.cameras = {};
        this.cameraList = [];
        this.downloading = [];
        this.downloadQueue = [];

        this.maxDownloads = 1;

        this.address = "192.168.0.255";
        this.ports = [8888, 8889];

        this.udp = new skanernet.UdpSocket();

        this.udp.handlers.ping.response = (rinfo) => {
            //console.log(`Received Pong from ${rinfo.address}:${rinfo.port}`);
            this.register(rinfo.address, rinfo.port);
        };
    }

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

    register(_address, _port) {
        if (this.cameras[`${_address}:${_port}`] == undefined) {
            let camera = new Camera(_address, _port);
            this.cameras[`${_address}:${_port}`] = camera;
            this.cameraList.push(camera);
        }
    }

    shoot(_id) {
        this.cameraList.forEach((x)=>{
            x.requestFile(_id, this.fileDownloadComplete.bind(this));
            this.downloadQueue.push(x);
        });
        this.ports.forEach((x)=>{this.udp.sendSnap(this.address, x, _id)});
        setTimeout(()=>{ this.processDownloadQueue() }, 600);
    }

    fileDownloadComplete(_camera, _id, _success, _data) {
        console.log(`Downloaded file with id [${_id}] form Camera[${this.cameraList.indexOf(_camera)}]. Success [${_success}].`);
        this.downloadQueue.splice(this.downloadQueue.indexOf(_camera), 1);
        this.downloading.splice(this.downloading.indexOf(_camera), 1);
        this.processDownloadQueue();
    }

    processDownloadQueue() {
        const maxDownloads = 1;
        let candidateQueue = this.downloadQueue.filter((x, index, self)=>(this.downloading.indexOf(x)==-1 && self.indexOf(x) === index));
        let newDownloads = candidateQueue.splice(0, maxDownloads-this.downloading.length );
        newDownloads.forEach(x=>x.processFileQueue());
        this.downloading = this.downloading.concat( newDownloads );
    }
    
    requestStreamFrame() {
        this.cameraList.forEach((x)=>{
            x.requestStreamFrame();
        });
    }
    
    unsubscribeAll(_socket) {
        this.cameraList.forEach((x)=>{x.unsubscribeStream(_socket)});
    }
    
    unregister(_camera) {
        console.log(`Camera at ${_camera.address}:${_camera.port} has disconnected.`);
        this.cameraList.splice(this.cameraList.indexOf(_camera), 1);
        delete( this.cameras[`${_camera.address}:${_camera.port}`] );
    }
}
cameraCenter = new CameraCenter();

/////////////////////////////////////
module.exports = cameraCenter;