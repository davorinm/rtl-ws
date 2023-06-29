let pos = 0;
let started = false;
let down = 0;
let no_last = 1;
let last_x = 0, last_y = 0;
let spectrumCtx;
let spectrumGridCtx;
let bandCtx;
let waterfallCtx;
let socket_lm;
let color = "#000000";
let y_offset = 256;
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
for (var idx = 0; idx < 200; idx++) {
    waterafallData[idx] = new Array();
    for (var idx2; idx2 < 1024; idx2++) {
        waterafallData[idx][idx2] = 0;
    }
}
let waterafallDataIndex = -1;
let waterfallImage;
let sound_on = false;
let samplesProcessorNode;
let audioContext;

window.onload = function (event) {
    initialize();
};

window.onresize = function (event) {

};

function initialize() {
    try {
        socket_lm = new WebSocket(get_appropriate_ws_url() + "/websocket");
        socket_lm.binaryType = "arraybuffer";
        console.log("WebSocket instantiated");

        socket_lm.onopen = function () {
            document.getElementById("ws_status").src = "connect2.png";
        }

        socket_lm.onmessage = function got_packet(msg) {
            // binary frame
            const view = new DataView(msg.data);
            var firstChar = String.fromCharCode(view.getUint8(0));

            if (firstChar == 'S') {
                // Spectrum
                const bytearray = new Uint8Array(msg.data, 1);

                updateSpectrum(bytearray);

                updateWaterfall(bytearray);
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

    // Spectrum
    var spectrumCanvas = document.getElementById('spectrumCanvas');
    spectrumCtx = spectrumCanvas.getContext("2d");
    spectrumCtx.canvas.width = spectrumCanvas.clientWidth;
    spectrumCtx.canvas.height = spectrumCanvas.clientHeight;

    console.log("spectrumCtx", spectrumCtx.canvas.clientWidth, spectrumCtx.canvas.clientHeight);

    // Spectrum grid
    var spectrumGridCanvas = document.getElementById('spectrumGridCanvas');
    spectrumGridCtx = spectrumGridCanvas.getContext("2d");
    spectrumGridCtx.canvas.width = spectrumGridCanvas.clientWidth;
    spectrumGridCtx.canvas.height = spectrumGridCanvas.clientHeight;

    // Band
    var bandCanvas = document.getElementById('bandCanvas');
    bandCtx = bandCanvas.getContext("2d");
    bandCtx.canvas.width = bandCanvas.clientWidth;
    bandCtx.canvas.height = bandCanvas.clientHeight;

    console.log("bandCtx", bandCtx.canvas.clientWidth, bandCtx.canvas.clientHeight);

    // Waterfall
    var waterfallCanvas = document.getElementById('waterfallCanvas');
    waterfallCtx = waterfallCanvas.getContext("2d");
    waterfallCtx.canvas.width = waterfallCanvas.clientWidth;
    waterfallCtx.canvas.height = waterfallCanvas.clientHeight;
    waterfallImage = waterfallCtx.createImageData(1024, 315);

    console.log("waterfallCtx", waterfallCtx.canvas.clientWidth, waterfallCtx.canvas.clientHeight);

    // other
    document.getElementById("toggle_sound").disabled = true;


    drawGrid();
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

function updateSpectrum(bytearray) {
    const spectrumCtxWidth = spectrumCtx.canvas.clientWidth;
    const spectrumCtxHeight = spectrumCtx.canvas.clientHeight;

    spectrumCtx.clearRect(0, 0, spectrumCtxWidth, spectrumCtxHeight);
    spectrumCtx.beginPath();
    spectrumCtx.strokeStyle = "#ff000";
    spectrumCtx.lineWidth = 2;

    for (var idx = 0; idx < bytearray.length; idx++) {
        const value = bytearray[idx];
        const yPos = -value * 4 + y_offset;
        if (idx == 0) {
            spectrumCtx.moveTo(0, yPos);
        }
        spectrumCtx.lineTo(idx, yPos);
    }

    spectrumCtx.stroke();
    spectrumCtx.closePath();
}

function drawGrid() {
    const bw = spectrumGridCtx.canvas.clientWidth;
    const bh = spectrumGridCtx.canvas.clientHeight;

    // Padding
    const p = 0;

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

    var tmp = ampl / 255.0;  // 0.0~1.0
    if (tmp < 0.50) {
        r = 0.0;
    } else if (tmp > 0.75) {
        r = 1.0;
    } else {
        r = 4.0 * tmp - 2.0;
    }

    if (tmp < 0.25) {
        g = 4.0 * tmp;
    } else if (tmp > 0.75) {
        g = -4.0 * tmp + 4.0;
    } else {
        g = 1.0;
    }

    if (tmp < 0.25) {
        b = 1.0;
    } else if (tmp > 0.50) {
        b = 0.0;
    } else {
        b = -4.0 * tmp + 2.0;
    }

    r *= 255;
    g *= 255;
    b *= 255;

    return [r, g, b];
}

function updateWaterfall(bytearray) {
    // Fill data
    waterafallDataIndex++;
    if (waterafallDataIndex >= waterafallData.length) {
        waterafallDataIndex = 0;
    }

    for (var idx = 0; idx < bytearray.length; idx++) {
        const value = bytearray[idx];
        waterafallData[waterafallDataIndex][idx - 1] = value;
    }

    // Draw waterfall
    for (var y = 0; y < waterafallData.length; y++) {
        var s_y = waterafallDataIndex + y;
        s_y = ((s_y >= waterafallData.length) ? (s_y - waterafallData.length) : s_y);
        for (var x = 0; x < 1024; x++) {
            var idx = (((waterafallData.length - 1) - y) * 1024 * 4) + x * 4;
            var ampl = 0;
            try {
                ampl = waterafallData[s_y][x];
            } catch (exception) {
                ampl = 0;
            }

            let color = colorFromAmplitude(ampl);

            waterfallImage.data[idx + 0] = color[0];
            waterfallImage.data[idx + 1] = color[1];
            waterfallImage.data[idx + 2] = color[2];
            waterfallImage.data[idx + 3] = 255;
        }
    }

    waterfallCtx.putImageData(waterfallImage, 0, 0);
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

async function initialize_sound() {
    audioContext = new AudioContext({ sampleRate: 48000 });
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