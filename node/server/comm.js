const http = require('http');
const express = require('express');
const app = express();
const httpServer = http.createServer(app);
const websocket = require('ws');

/////////////////////////////////////
// WebSocket

const wss = new websocket.Server({
    server: httpServer
});
wss.on('connection', function(_socket) {
    console.log(`Socket[${messageCenter.sockets.push(_socket)-1}] connected.`);
    _socket.details = {};
    
    _socket.on('message', (_data) => {
        messageCenter.incoming( _socket, JSON.parse(_data) );
    });
    _socket.on('close', (_reasonCode, _description) => {
        console.log(`Socket[${messageCenter.sockets.indexOf(_socket)}] disconnected.`)
        delete( messageCenter.sockets[messageCenter.sockets.indexOf(_socket)] );
    });
});

/////////////////////////////////////
// HTTP Server

app.use(express.static(__dirname + '/website'));
httpServer.listen(80, function() {
    console.log("Server listening at port 80!");
});

/////////////////////////////////////
// Message Center

messageCenter = {
    handlers: {},
    sockets: []
};

messageCenter.incoming = function(_socket, _obj) {
    this.last = _obj;
    if (_obj == undefined) {
        console.info("Got non-JSON message");
    } else if (_obj.msg == undefined) {
        console.info("Got unknown object");
    } else if (this.handlers[_obj.msg] == undefined) {
        console.groupCollapsed("Got unhandled object");
        console.dir(_obj);
        console.groupEnd();
    }else {
        this.handlers[_obj.msg](_socket, _obj);
    }
}

messageCenter.register = function(msg, callback) {
    this.handlers[msg] = callback;
}

messageCenter.unregister = function(msg) {
    delete( this.handlers[msg] );
}

messageCenter.send = function(socket, obj) {
    socket.send( JSON.stringify(obj) );
}

/////////////////////////////////////
module.exports = messageCenter;
