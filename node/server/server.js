// @ts-check

const {messageCenter} = require("./comm.js");
const {CameraCenter} = require("./cam.js");

const cameraCenter = new CameraCenter();
global.cameraCenter = cameraCenter;

const fs = require("fs");
const readline = require("readline");
const rl = readline.createInterface({
    input: process.stdin,
    output: process.stdout
});

const startNumber = Date.now();
let fileCount = 0;

setInterval(cameraCenter.requestStreamFrame.bind(cameraCenter), 1000/24);

messageCenter.register("getCameras", (_socket, _obj) => {
    console.log(`Socket[${messageCenter.sockets.indexOf(_socket)}] requested list of cameras.`);
    cameraCenter.updateCameras(_socket.send.bind(_socket));
});
messageCenter.register("startStream", (_socket, _obj) => {
    console.log(`Socket[${messageCenter.sockets.indexOf(_socket)}] requested stream from camera[${_obj.address}:${_obj.port}].`);
    cameraCenter.unsubscribeAll(_socket);
    cameraCenter.cameras[`${_obj.address}:${_obj.port}`].subscribeStream(_socket);
});
messageCenter.register("stopStream", (_socket, _obj) => {
    console.log(`Socket[${messageCenter.sockets.indexOf(_socket)}] requested stream stop.`);
    cameraCenter.unsubscribeAll(_socket);
});
messageCenter.register("shoot", (_socket, _obj) => {
    console.log(`Socket[${messageCenter.sockets.indexOf(_socket)}] requested image shot.`);
    cameraCenter.shoot( startNumber+fileCount++ & 0x7FFFFFFF);
});