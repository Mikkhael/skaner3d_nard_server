var Comm = {
    messageCenter: {handlers: {}},
    socket: undefined,
    timeout: false,
    tmp: undefined
};
Comm.init = function() {
    this.socket = new WebSocket("ws://"+window.location.host,"connect");
    this.socket.onopen = function() {
        //this.send("control");
    }
    this.socket.onmessage = (message) => {
        this.messageCenter.incoming( JSON.parse(message.data) );
    }
}

Comm.messageCenter.incoming = function(obj) {
    this.tmp = obj;
    if (obj == undefined) {
        console.info("Got non-JSON message");
    } else if (obj.msg == undefined) {
        console.info("Got unknown object");
    } else if (this.handlers[obj.msg] == undefined) {
        console.groupCollapsed("Got unhandled object");
        console.dir(obj);
        console.groupEnd();
    }else {
        this.handlers[obj.msg](obj);
    }
}

Comm.messageCenter.register = function(msg, callback) {
    this.handlers[msg] = callback;
}

Comm.messageCenter.unregister = function(msg) {
    delete(this.handlers[msg]);
}

Comm.send = function(obj) {
    this.socket.send( JSON.stringify(obj) );
}