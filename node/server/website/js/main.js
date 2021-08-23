const framePath = "frame.png";
var noImage, webView, cameraList, cameras;

function init() {
    webView = document.getElementById("webView");
    noImage = document.getElementById("noImage");
    cameraList = document.getElementById("cameraList");
    initHandlers();
    
    Comm.init();
    Comm.socket.onopen = continueLoading;
}

function continueLoading() {
    
    Comm.send({msg: "getCameras"});
    
    //console.log("test");
}

function handler_getCameras(obj) {
    cameras = obj.cameras;
    cameraList.innerHTML = "";
    let opt = document.createElement("option");
    opt.value = -1;
    opt.innerHTML = "None";
    cameraList.appendChild(opt);
    let optCount = 0;
    
    for (o of obj.cameras) {
        console.log(o);
        
        opt = document.createElement("option");
        opt.value = optCount++;
        opt.innerHTML = "{ id: "+o.id+", address: "+o.address+", port: "+o.port+" }";
        cameraList.appendChild(opt);
    }
}

function handler_frame(obj) {
    URL.revokeObjectURL(webView.src);
    webView.src = URL.createObjectURL( new Blob([ new Uint8Array( obj.buffer.split("").map((x)=>x.charCodeAt(0)) ) ], {type:"image/png"}) );
}

function initHandlers() {
    document.getElementById("refresh").onclick = function() {
        Comm.send({msg:"getCameras"});
    }
    document.getElementById("shoot").onclick = function() {
        Comm.send({msg:"shoot"});
    }
    cameraList.onchange = function(e) {
        let index = parseInt(cameraList.value);
        if (typeof(index) == "number" && cameras[index] != undefined) {
            Comm.send({msg: "startStream", address: cameras[index].address, port: cameras[index].port});
        } else {
            Comm.send({msg: "stopStream"});
        }
    } 
    Comm.messageCenter.register("cameras", handler_getCameras);
    Comm.messageCenter.register("frame", handler_frame);
}
window.onload = init;