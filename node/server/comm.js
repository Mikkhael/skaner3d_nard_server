// @ts-check

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

/** @typedef {websocket & {details: {}}} AugmentedWebsocket */

wss.on('connection',  function(/** @type {AugmentedWebsocket}*/ _socket) {
    console.log(`Socket[${messageCenter.sockets.push(_socket)-1}] connected.`);
    _socket.details = {};
    
    _socket.on('message', (_data) => {
        messageCenter.incoming( _socket, JSON.parse(_data.toString()) );
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

class MessageCenter{
    constructor(){
        /** @type {Object.<string, function(websocket, any): void>} */
        this.handlers = {};
        /** @type {websocket[]} */
        this.sockets = [];
    }
    
    /** @param {websocket} _socket */
    incoming(_socket, _obj) {
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
    
    /** @param {string} msg @param {function(websocket, any): void} callback*/
    register(msg, callback) {
        this.handlers[msg] = callback;
    }

    /** @param {string} msg*/
    unregister(msg) {
        delete( this.handlers[msg] );
    }

    /** @param {websocket} socket @param {string} obj */
    send(socket, obj) {
        socket.send( JSON.stringify(obj) );
    }

};

const messageCenter = new MessageCenter();


/////////////////////////////////////
module.exports = {
    messageCenter // Nie mogę wyeksportować jako klasy, bo używasz obkiektu "messageCenter" w tym pliku na początku
};
