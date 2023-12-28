let pos = 0;
let started = false;
let down = 0;
let no_last = 1;
let last_x = 0, last_y = 0;
let socket_lm;
let color = "#000000";
let spectrogram_offset = 315;
let frequency = 100000;
let spectrumgain = 0;
let real_spectrumgain = 0;
let real_frequency = 100000;
let bandwidth = 2048;
let real_bandwidth = 2048;
let conn_counter = 0;
let conn_inc = -1;
let waterafallData = new Array();
let sound_on = false;
let samplesProcessorNode;
let audioContext;

let spectrumCanvas;
let spectrumCtx;

let spectrumGridCanvas;
let spectrumGridCtx;

let bandCanvas;
let bandCtx;

let waterfallCanvas;
let waterfallCtx;
let waterfallMaxData = 350;

window.onload = function (event) {
    initialize();
    drawGrid();
    drawSpectrum();
    drawWaterfall();

    connect();
};

window.onresize = function (event) {
    initialize();
    drawGrid();
    drawSpectrum();
    drawWaterfall();
};

function connect() {
    let url = get_appropriate_ws_url() + "/websocket"
    try {
        socket_lm = new WebSocket(url);
        socket_lm.binaryType = "arraybuffer";
        console.log("WebSocket instantiated");

        socket_lm.onopen = function () {
            document.getElementById("ws_status").src = "connect2.png";
        }

        socket_lm.onmessage = function got_packet(msg) {
            // binary frame
            const view = new DataView(msg.data);
            let firstChar = String.fromCharCode(view.getUint8(0));

            if (firstChar == 'S') {
                // Spectrum data
                let spectrumData = new Int8Array(msg.data, 1);
                waterafallData.unshift(spectrumData);
    
                if (waterafallData.length > waterfallMaxData) {
                    waterafallData.length = waterfallMaxData;
                }

                drawSpectrum();
                drawWaterfall();
            }
            else if (firstChar == 'A') {
                // Audio
                const audioArray = new Float32Array(msg.data, 4);
                samplesProcessorNode.port.postMessage(audioArray);
            }
            else if (firstChar == 'T') {
                // Tuner
                const bytearray = new Uint8Array(msg.data, 1);
                const strData = String.fromCharCode(...bytearray);
                const j = strData.split(';');

                let f = 0;
                let redraw_hz_axis = false;

                while (f < j.length) {
                    const i = j[f++].split(' ');

                    if (i[0] == 'b') {
                        let bw_element = document.getElementById("bandwidth");
                        if ((bw_element.value * 1000) != i[1]) {
                            bw_element.style.color = "lightgray";
                        } else {
                            bw_element.style.color = "black";
                            real_bandwidth = parseInt(bandwidth);
                            redraw_hz_axis = true;
                        }
                    } else if (i[0] == 'f') {
                        let freq_element = document.getElementById("frequency");
                        if ((freq_element.value * 1000) != i[1]) {
                            freq_element.style.color = "lightgray";
                        } else {
                            freq_element.style.color = "black";
                            real_frequency = parseInt(frequency);
                            redraw_hz_axis = true;
                        }
                    } else if (i[0] == 's') {
                        let spectrumgain_element = document.getElementById("spectrumgain");
                        if (spectrumgain_element.value != i[1]) {
                            spectrumgain_element.style.color = "lightgray";
                        } else {
                            spectrumgain_element.style.color = "black";
                            real_spectrumgain = parseInt(spectrumgain);
                        }
                    } else if (i[0] == 'y') {
                        console.log("number of samples", i[1]);
                    }
                }

                if (redraw_hz_axis) {
                    upateFrequencyBands()
                }
            }
        }

        socket_lm.onclose = function () {
            document.getElementById("ws_status").src = "connect2_close.png";
        }

        socket_lm.onerror = function () {
            document.getElementById("ws_status").src = "connect2_close.png";
        }
    } catch (e) {
        console.error('Sorry, the web socket at "%s" is un-available', url);
    }
}

function initialize() {
    const dpr = window.devicePixelRatio;

    // Spectrum
    spectrumCanvas = document.getElementById('spectrumCanvas');
    spectrumCtx = spectrumCanvas.getContext("2d");

    // Spectrum grid
    spectrumGridCanvas = document.getElementById('spectrumGridCanvas');
    spectrumGridCtx = spectrumGridCanvas.getContext("2d");

    // Band
    bandCanvas = document.getElementById('bandCanvas');
    bandCtx = bandCanvas.getContext("2d");

    // Waterfall
    waterfallCanvas = document.getElementById('waterfallCanvas');
    waterfallCtx = waterfallCanvas.getContext("2d");

    // other
    document.getElementById("toggle_sound").disabled = true;
}

function get_appropriate_ws_url() {
    var pcol;
    var u = document.URL;

    if (u.substring(0, 5) == "https") {
        pcol = "wss://";
        u = u.substr(8);
    } else {
        pcol = "ws://";
        if (u.substring(0, 4) == "http")
            u = u.substr(7);
    }

    u = u.split('/');

    return pcol + u[0];
}

function drawSpectrum() {
    const spectrumCtxWidth = spectrumCanvas.clientWidth;
    const spectrumCtxHeight = spectrumCanvas.clientHeight;

    if (waterafallData.length <= 0) {
        return;
    }

    const dataCount = waterafallData[0].length;

    spectrumCanvas.width = dataCount;

    var maxValue = Math.max.apply(Math, waterafallData[0]);
    var minValue = Math.min.apply(Math, waterafallData[0]);
    console.log("maxValue", maxValue, "minValue", minValue);

    spectrumCtx.clearRect(0, 0, spectrumCtxWidth, spectrumCtxHeight);
    spectrumCtx.beginPath();
    spectrumCtx.strokeStyle = "#ff000";
    spectrumCtx.lineWidth = 1;

    let spectrumCtxPosHorizontal = 0
    for (var idx = 0; idx < dataCount; idx++) {
        const value = waterafallData[0][idx];
        const yPos = -value;
        if (idx == 0) {
            spectrumCtx.moveTo(0, yPos);
        }
        spectrumCtx.lineTo(spectrumCtxPosHorizontal, yPos);
        spectrumCtxPosHorizontal += 1;
    }

    spectrumCtx.stroke();
    spectrumCtx.closePath();
}

function drawGrid() {
    const bw = spectrumGridCanvas.clientWidth;
    const bh = spectrumGridCanvas.clientHeight;

    // Padding
    const p = 0;

    spectrumGridCtx.clearRect(0, 0, bw, bh);

    // Horizontal lines
    for (var x = 40; x <= bw; x += 40) {
        spectrumGridCtx.moveTo(0.5 + x + p, p);
        spectrumGridCtx.lineTo(0.5 + x + p, bh + p);
    }

    // Vertical lines
    for (var x = 40; x <= bh; x += 40) {
        spectrumGridCtx.moveTo(p, 0.5 + x + p);
        spectrumGridCtx.lineTo(bw + p, 0.5 + x + p);
    }

    spectrumGridCtx.strokeStyle = "black";
    spectrumGridCtx.lineWidth = 0.5;
    spectrumGridCtx.stroke();
}

function upateFrequencyBands() {
    const bandCtxWidth = bandCtx.canvas.clientWidth;
    const bandCtxHeight = bandCtx.canvas.clientHeight;

    const textVerticalPosition = bandCtxHeight / 2;

    bandCtx.clearRect(0, 0, bandCtxWidth, bandCtxHeight);

    bandCtx.font = "14px Georgia";
    bandCtx.fillStyle = "black";

    var freqtext = (real_frequency - real_bandwidth / 2) / 1000 + " MHz";
    var w = bandCtx.measureText(freqtext).width;
    bandCtx.fillText(freqtext, 0, textVerticalPosition);

    freqtext = (real_frequency) / 1000 + " MHz";
    w = bandCtx.measureText(freqtext).width;
    bandCtx.fillText(freqtext, (bandCtxWidth / 2) - (w / 2), textVerticalPosition);

    freqtext = (real_frequency + real_bandwidth / 2) / 1000 + " MHz";
    w = bandCtx.measureText(freqtext).width;
    bandCtx.fillText(freqtext, bandCtxWidth - w, textVerticalPosition);

    bandCtx.beginPath();
    bandCtx.lineWidth = "1";
    bandCtx.fillStyle = "lightgray";
    bandCtx.arc(conn_counter * 5 + 5, 5, 4, 0, 2 * Math.PI);
    bandCtx.fill();
    bandCtx.closePath();
}

function colorFromAmplitude(ampl) {
    let r, g, b;

    var tmp = (ampl + 127);
    
    r = tmp;
    g = tmp;
    b = tmp;

    return [r, g, b];
}

function drawWaterfall() {
    if (waterafallData.length <= 0) {
        return;
    }
    
    const imageWidth = waterafallData[0].length;
    const imageHeight = waterafallData.length;

    waterfallCanvas.width = imageWidth;
    waterfallCanvas.height = waterfallMaxData;

    console.log("waterfallWidth", imageWidth, "waterfallHeight", imageHeight);
    
    let waterfallImage = new ImageData(imageWidth, imageHeight);

    // Draw waterfall
    let idx = 0;
    for (var y = 0; y < waterafallData.length; y++) {
        for (var x = 0; x < waterafallData[y].length; x++) {
            var ampl = waterafallData[y][x];

            let color = colorFromAmplitude(ampl);

            waterfallImage.data[idx + 0] = color[0];
            waterfallImage.data[idx + 1] = color[1];
            waterfallImage.data[idx + 2] = color[2];
            waterfallImage.data[idx + 3] = 255;
            idx += 4;
        }
    }
          
    waterfallCtx.putImageData(waterfallImage, 0, 0);
}

async function resizeImageData (imageData, width, height) {
    const resizeWidth = width >> 0
    const resizeHeight = height >> 0
    const ibm = await window.createImageBitmap(imageData, 0, 0, imageData.width, imageData.height, {
      resizeWidth, resizeHeight
    })
    const canvas = document.createElement('canvas')
    canvas.width = resizeWidth
    canvas.height = resizeHeight
    const ctx = canvas.getContext('2d')
    ctx.scale(resizeWidth / imageData.width, resizeHeight / imageData.height)
    ctx.drawImage(ibm, 0, 0)
    return ctx.getImageData(0, 0, resizeWidth, resizeHeight)
  }

function frequency_change() {
    frequency = document.getElementById("frequency").value;
    socket_lm.send("freq " + frequency);
}

function bw_change() {
    bandwidth = document.getElementById("bandwidth").value;
    socket_lm.send("bw " + bandwidth);
}

function spectrumgain_change() {
    spectrumgain = document.getElementById("spectrumgain").value;
    socket_lm.send("spectrumgain " + spectrumgain);
}

function start_or_stop() {
    console.log("start_or_stop");

    let value = document.getElementById("start_or_stop").value;
    socket_lm.send(value);
    if (value == "start") {
        started = true;
        document.getElementById("start_or_stop").value = "stop";
    } else {
        started = false;
        document.getElementById("start_or_stop").value = "start";
    }

    // Toggle Audio button
    document.getElementById("toggle_sound").disabled = !started;
}

function kill() {
    socket_lm.send("kill");
}

async function initialize_sound() {
    audioContext = new AudioContext({ sampleRate: 44100 });
    await audioContext.audioWorklet.addModule("samples-processor.js");

    samplesProcessorNode = new AudioWorkletNode(audioContext, "samples-processor");
}

async function toggle_sound() {
    var value = document.getElementById("toggle_sound").value;
    socket_lm.send(value);

    if (value == "start_audio") {
        sound_on = true;
        document.getElementById("toggle_sound").value = "stop_audio";
        await initialize_sound();
        samplesProcessorNode.connect(audioContext.destination);
    } else {
        sound_on = false;
        document.getElementById("toggle_sound").value = "start_audio";
        samplesProcessorNode.disconnect();
    }
}

function demo() {
    const hz = 440;
    for (j = 0; j < 350; j++) {
        // fill all 44100 elements of array with Math.sin() values
        const sineWaveArray = new Int8Array(1024 * 4);
        for (i = 0; i < sineWaveArray.length; i++) {
            let t = (Math.sin(i * Math.PI * 2 / hz) - 1) * 50;
            sineWaveArray[i] = t;
        }
        waterafallData.unshift(sineWaveArray);
    }

    initialize();
    drawGrid();
    drawSpectrum();
    drawWaterfall();
}