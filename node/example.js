
const readline = require("readline");
const skanernet = require("./skaner3d_networking.js");
const fs = require("fs");

const rl = readline.createInterface({
    input: process.stdin,
    output: process.stdout
});

//// Udp

const udp = new skanernet.UdpSocket();

udp.handlers.socket.listening = () => {
    console.log('UDP Socket started listening');
};
udp.handlers.socket.close = () => {
    console.log('UDP Socket closed');
};

udp.handlers.socket.error = (err) => {
    console.log(`UDP Socket error occured: ${err}`);
};
udp.handlers.other_error.unknown_id = (id, rinfo) => {
    console.log(`Unrecognized datagram ID received from ${rinfo.address}:${rinfo.port} - ${id}`);
};

udp.handlers.ping.response = (rinfo) => {
    console.log(`Received Pong from ${rinfo.address}:${rinfo.port}`);
};
udp.handlers.echo.response = (message, rinfo) => {
    console.log(`Received Echo response from ${rinfo.address}:${rinfo.port} with message: ${message}`);
}

/// Tcp

const tcp = new skanernet.TcpConnection();

tcp.handlers.socket.connect = () => {
    console.log(`TCP Socket connected to ${tcp.getAddress()}:${tcp.getPort()}`);
}
tcp.handlers.socket.close = () => {
    console.log(`TCP Socket closed.`);
}

tcp.handlers.socket.error = (err) => {
    console.log(`TCP Socket error occured: ${err}`);
}

tcp.handlers.echo.response = (message) => {
    console.log(`Received Echo response with message: ${message}`);
}

const FileTransmissionContext = {
    file_handle: null
};

tcp.handlers.file.start = (totalFileSize, next) => {
    console.log(`Begining to download file with total size: ${totalFileSize}`);
    if(totalFileSize > 999999999999999999999){
        console.log(`File size is to big. Closing session.`);
        tcp.disconnect();
    }else{
        next();
    }
};
tcp.handlers.file.part = (data, next) => {
    console.log(`Recived file part with size: ${data.length}`);
    fs.write(FileTransmissionContext.file_handle, data, (err)=>{
        if(err){
            console.log(`File system error occured: ${err}`);
        }else{
            next();
        }
    });
};
tcp.handlers.file.complete = () => {
    console.log(`Completed file receive`);
    fs.closeSync(FileTransmissionContext.file_handle);
};
tcp.handlers.file.server_error = () => {
    console.log(`Server file-system error occured during file transmission`);
}
tcp.handlers.file.format_error = () => {
    console.log(`Ill-formatted packet received. Closing session.`);
    tcp.disconnect();
}


// Exemple user interface

function getOption(){
    
    console.log("==== Menu ==== ");
    console.log("dp - Udp Ping | <address> <port>");
    console.log("de - Udp Echo | <address> <port> <message>");
    console.log("tc - TCP Connect to server | <address> <port>");
    console.log("tx - TCP Disconnect");
    console.log("te - TCP Echo | <message>");
    console.log("tf - TCP File download | <server_filepath> <local_filepath>");
    console.log("q  - Quit");
    rl.question(">", handleOption);
    
}

function handleOption(prompt, test = false){
    let args = prompt.trim().split(/\s+/);
    if(args[0] == 'q'){
        udp.close()
        tcp.disconnect();
        rl.close();
        return;
    }else if(args[0] == "dp"){
        console.log(`Sending UDP Ping request to ${args[1]}:${args[2]}`)
        udp.sendPing( args[1], parseInt(args[2]) );
    } else if (args[0] == "de"){
        console.log(`Sending UDP Echo request to ${args[1]}:${args[2]} with message ${args[3]}`)
        udp.sendEcho( args[1], parseInt(args[2]), args[3] );
    } else if (args[0] == "tc"){
        console.log(`Connecting to TCP Server to ${args[1]}:${args[2]}`);
        tcp.connect( args[1], parseInt(args[2]) );
    } else if (args[0] == "tx"){
        console.log(`Disconnecting from TCP Server`);
        tcp.disconnect();
    } else if (args[0] == "te"){
        console.log(`Sending TCP Echo request with message ${args[1]}`);
        tcp.sendEcho( args[1] );
    } else if (args[0] == "tf"){
        console.log(`TCP Downloading file ${args[1]} to ${args[2]}`);
        fs.open(args[2], "w", (err, fd) => {
            if(err){
                console.log(`Cannot open file ${args[2]} : ${err}`);
                return;
            }
            FileTransmissionContext.file_handle = fd;
            tcp.sendCustomFileRequest(args[1]);
        });
    } else if (args[0] == "A"){
        handleOption("tc 127.0.0.1 8888", true);
        setTimeout(() => handleOption("tf AAA BBB"), 1000);
    }
    
    if(!test)
        setTimeout(getOption, 300);
}

setTimeout(getOption, 1000);