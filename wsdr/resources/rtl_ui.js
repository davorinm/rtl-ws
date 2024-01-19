let pos = 0;
let started = false;
let last_x = 0, last_y = 0;
let socket_lm;


let real_frequency = 100000;
let real_bandwidth = 2048;
let real_samplerate = 1024;
let real_rfgain = 0;
let real_spectrumSamples = 2048;

let sound_on = false;
let samplesProcessorNode;
let audioContext;

let spectrumFilterCanvas;
let spectrumFilterCtx;
let spectrumFilterStart;
let spectrumFilterEnd;

let spectrumCanvas;
let spectrumCtx;

let spectrumGridCanvas;
let spectrumGridCtx;

let bandCanvas;
let bandCtx;

let waterfallCanvas;
let waterfallCtx;
let waterfallData = new Array();
let waterfallMaxData = 350;

let spectrumChartValuesCanvas;
let spectrumChartValuesCtx;

let spectrumTimeValuesCanvas;
let spectrumTimeValuesCtx;

var colormap = [];
for (i = 0; i < 256; i++) {
    if ((i < 43))
        colormap[i] = new Uint8ClampedArray([0, 0, Math.round(255 * (i / 43)), 255]);
    if ((i >= 43) && (i < 87))
        colormap[i] = new Uint8ClampedArray([0, Math.round(255 * (i - 43) / 43), 255, 255]);
    if ((i >= 87) && (i < 120))
        colormap[i] = new Uint8ClampedArray([0, 255, Math.round(255 - (255 * (i - 87) / 32)), 255]);
    if ((i >= 120) && (i < 154))
        colormap[i] = new Uint8ClampedArray([Math.round((255 * (i - 120) / 33)), 255, 0, 255]);
    if ((i >= 154) && (i < 217))
        colormap[i] = new Uint8ClampedArray([255, Math.round(255 - (255 * (i - 154) / 62)), 0, 255]);
    if ((i >= 217))
        colormap[i] = new Uint8ClampedArray([255, 0, Math.round(128 * (i - 217) / 38), 255]);
}

window.onload = function (event) {
    initialize();
    drawGrid();
    drawSpectrum();
    drawFrequencyBands()
    drawWaterfall();
    drawSpectrumChartValuesCanvas();
    drawSpectrumTimeValuesCanvas();

    connect();
};

window.onresize = function (event) {
    initialize();
    drawGrid();
    drawSpectrum();
    drawFrequencyBands()
    drawWaterfall();
    drawSpectrumChartValuesCanvas();
    drawSpectrumTimeValuesCanvas();
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
                waterfallData.unshift(spectrumData);

                if (waterfallData.length > waterfallMaxData) {
                    waterfallData.length = waterfallMaxData;
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

                    if (i[0] == 'f') {
                        let frequency = parseInt(i[1]);
                        if (real_frequency != frequency) {
                            real_frequency = frequency;
                            document.getElementById("frequency").value = real_frequency;
                            redraw_hz_axis = true;
                        }
                    } else if (i[0] == 'b') {
                        let bandwidth = parseInt(i[1]);
                        if (real_bandwidth != bandwidth) {
                            real_bandwidth = bandwidth;
                            document.getElementById("bandwidth").value = real_bandwidth;
                            redraw_hz_axis = true;
                        }
                    } else if (i[0] == 's') {
                        let samplerate = parseInt(i[1]);
                        if (real_samplerate != samplerate) {
                            real_samplerate = samplerate;
                            document.getElementById("samplerate").value = real_samplerate;
                            redraw_hz_axis = true;
                        }
                    } else if (i[0] == 'g') {
                        let rfgain = parseInt(i[1]);
                        if (real_rfgain != rfgain) {
                            real_rfgain = rfgain;
                            document.getElementById("rfgain").value = real_rfgain;
                            redraw_hz_axis = true;
                        }
                    } else if (i[0] == 'y') {
                        let spectrumSamples = parseInt(i[1]);
                        if (real_spectrumSamples != spectrumSamples) {
                            real_spectrumSamples = spectrumSamples;
                            document.getElementById("samples_buffer").innerHTML = real_spectrumSamples;
                            redraw_hz_axis = true;
                        }
                    }
                }

                if (redraw_hz_axis) {
                    drawFrequencyBands();
                    drawGrid();
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

//// Spectrum listeners

let spectrumMouseDown = false;

function spectrumDownListener(e) {
    spectrumMouseDown = true;

    const elementRelativeX = e.offsetX;
    const elementRelativeY = e.offsetY;
    const canvasRelativeX = elementRelativeX * spectrumCanvas.width / spectrumCanvas.clientWidth;
    const canvasRelativeY = elementRelativeY * spectrumCanvas.height / spectrumCanvas.clientHeight;

    console.log("downListener", "elementRelativeX", elementRelativeX, "elementRelativeY", elementRelativeY, "canvasRelativeX", canvasRelativeX, "canvasRelativeY", canvasRelativeY);

    let pointForHz = real_samplerate / real_spectrumSamples;
    let hz = canvasRelativeX * pointForHz + real_frequency;
    hz = Math.round(hz);
    console.log("selected freq", hz);

    document.getElementById("tuner_frequency").value = '' + hz;

    spectrumFilterStart = elementRelativeX;
    spectrumFilterEnd = null;

    drawFilter();
}

function spectrumMoveListener(e) {
    if (!spectrumMouseDown) {
        return;
    }

    console.log("spectrumMoveListener");

    const elementRelativeX = e.offsetX;
    const elementRelativeY = e.offsetY;
    const canvasRelativeX = elementRelativeX * spectrumCanvas.width / spectrumCanvas.clientWidth;
    const canvasRelativeY = elementRelativeY * spectrumCanvas.height / spectrumCanvas.clientHeight;

    let pointForHz = real_samplerate / real_spectrumSamples;
    let hz = canvasRelativeX * pointForHz + real_frequency;
    hz = Math.round(hz);
    console.log("released freq", hz);

    document.getElementById("tuner_width").value = '' + hz;

    spectrumFilterEnd = elementRelativeX;

    drawFilter();
}

function spectrumUpListener(e) {
    spectrumMouseDown = false;
    console.log("spectrumUpListener");

    const elementRelativeX = e.offsetX;
    const elementRelativeY = e.offsetY;
    const canvasRelativeX = elementRelativeX * spectrumCanvas.width / spectrumCanvas.clientWidth;
    const canvasRelativeY = elementRelativeY * spectrumCanvas.height / spectrumCanvas.clientHeight;

    let pointForHz = real_samplerate / real_spectrumSamples;
    let hz = canvasRelativeX * pointForHz + real_frequency;
    hz = Math.round(hz);
    console.log("released freq", hz);
    
    document.getElementById("tuner_width").value = '' + hz;

    spectrumFilterEnd = elementRelativeX;

    drawFilter();
}

//// Waterfall listeners

let waterfallMouseDown = false;

function waterfallDownListener(e) {
    waterfallMouseDown = true;

    const elementRelativeX = e.offsetX;
    const elementRelativeY = e.offsetY;
    const canvasRelativeX = elementRelativeX * waterfallCanvas.width / waterfallCanvas.clientWidth;
    const canvasRelativeY = elementRelativeY * waterfallCanvas.height / waterfallCanvas.clientHeight;

    let pointForHz = real_samplerate / real_spectrumSamples;
    let hz = canvasRelativeX * pointForHz + real_frequency;
    hz = Math.round(hz);
    console.log("selected freq", hz);

    document.getElementById("tuner_frequency").value = '' + hz;

    spectrumFilterStart = canvasRelativeX;
    spectrumFilterEnd = null;

    drawFilter();
}

function waterfallMoveListener(e) {
    if (!waterfallMouseDown) {
        return;
    }       

    console.log("waterfallMoveListener");

    const elementRelativeX = e.offsetX;
    const elementRelativeY = e.offsetY;
    const canvasRelativeX = elementRelativeX * waterfallCanvas.width / waterfallCanvas.clientWidth;
    const canvasRelativeY = elementRelativeY * waterfallCanvas.height / waterfallCanvas.clientHeight;

    let pointForHz = real_samplerate / real_spectrumSamples;
    let hz = canvasRelativeX * pointForHz + real_frequency;
    hz = Math.round(hz);
    console.log("selected freq", hz);
    
    document.getElementById("tuner_width").value = '' + hz;

    spectrumFilterEnd = canvasRelativeX;

    drawFilter();
}

function waterfallUpListener(e) {
    waterfallMouseDown = false;
    console.log("waterfallUpListener");

    const elementRelativeX = e.offsetX;
    const elementRelativeY = e.offsetY;
    const canvasRelativeX = elementRelativeX * waterfallCanvas.width / waterfallCanvas.clientWidth;
    const canvasRelativeY = elementRelativeY * waterfallCanvas.height / waterfallCanvas.clientHeight;

    let pointForHz = real_samplerate / real_spectrumSamples;
    let hz = canvasRelativeX * pointForHz + real_frequency;
    hz = Math.round(hz);
    console.log("selected freq", hz);
    
    document.getElementById("tuner_width").value = '' + hz;

    spectrumFilterEnd = canvasRelativeX;

    drawFilter();
}

////////

function initialize() {
    const dpr = window.devicePixelRatio;

    // Spectrum
    spectrumCanvas = document.getElementById('spectrumCanvas');
    spectrumCtx = spectrumCanvas.getContext("2d");

    let spectrum = document.getElementById('spectrum');
    spectrum.addEventListener('mousedown', spectrumDownListener)
    spectrum.addEventListener('mousemove', spectrumMoveListener)
    spectrum.addEventListener('mouseup', spectrumUpListener)

    // Spectrum grid
    spectrumGridCanvas = document.getElementById('spectrumGridCanvas');
    spectrumGridCtx = spectrumGridCanvas.getContext("2d");

    // Spectrum filter
    spectrumFilterCanvas = document.getElementById('spectrumFilterCanvas');
    spectrumFilterCtx = spectrumFilterCanvas.getContext("2d");

    // Band
    bandCanvas = document.getElementById('bandCanvas');
    bandCtx = bandCanvas.getContext("2d");

    // Waterfall
    waterfallCanvas = document.getElementById('waterfallCanvas');
    waterfallCtx = waterfallCanvas.getContext("2d");

    waterfallCanvas.addEventListener('mousedown', waterfallDownListener)
    waterfallCanvas.addEventListener('mousemove', waterfallMoveListener)
    waterfallCanvas.addEventListener('mouseup', waterfallUpListener)

    // Spectrum Chart Values
    spectrumChartValuesCanvas = document.getElementById('spectrumChartValues');
    spectrumChartValuesCtx = spectrumChartValuesCanvas.getContext("2d");

    // Spectrum Time Values
    spectrumTimeValuesCanvas = document.getElementById('spectrumTimeValues');
    spectrumTimeValuesCtx = spectrumTimeValuesCanvas.getContext("2d");

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
    const bw = spectrumCanvas.clientWidth;
    const bh = spectrumCanvas.clientHeight;

    // Setup canvas
    spectrumCanvas.width = bw;
    spectrumCanvas.height = bh;

    // Clear canvas
    spectrumCtx.clearRect(0, 0, bw, bh);

    if (waterfallData.length <= 0) {
        return;
    }

    const dataCount = waterfallData[0].length;

    var maxValue = Math.max.apply(Math, waterfallData[0]);
    var minValue = Math.min.apply(Math, waterfallData[0]);
    //console.log("maxValue", maxValue, "minValue", minValue);

    spectrumCtx.beginPath();
    spectrumCtx.strokeStyle = "#ff000";
    spectrumCtx.lineWidth = 1;

    let increment = bw / dataCount;

    let spectrumCtxPosHorizontal = 0
    for (var idx = 0; idx < dataCount; idx++) {
        const value = waterfallData[0][idx];
        const yPos = -value;
        if (idx == 0) {
            spectrumCtx.moveTo(0, yPos);
        }
        spectrumCtx.lineTo(spectrumCtxPosHorizontal, yPos);
        spectrumCtxPosHorizontal += increment;
    }

    spectrumCtx.stroke();
    spectrumCtx.closePath();
}

function drawGrid() {
    const bw = spectrumGridCanvas.clientWidth;
    const bh = spectrumGridCanvas.clientHeight;

    // Setup canvas
    spectrumGridCanvas.width = bw;
    spectrumGridCanvas.height = bh;

    // Clear canvas
    spectrumGridCtx.clearRect(0, 0, bw, bh);

    let ttt = real_bandwidth / real_spectrumSamples;

    spectrumGridCtx.strokeStyle = "black";
    spectrumGridCtx.lineWidth = 1;

    // Vertical lines
    const verticalDiv = 6;
    for (var div = 0; div <= verticalDiv; div++) {
        let point = bw * (div/verticalDiv);
        if (point == 0) {
            point = 1;
        }
        spectrumGridCtx.moveTo(point, 1);
        spectrumGridCtx.lineTo(point, bh);
    }

    // Horizontal lines
    const horizontalDiv = 10;
    for (var div = 0; div <= horizontalDiv; div++) {
        let point = bh * (div/horizontalDiv);
        if (point == 0) {
            point = 1;
        }
        spectrumGridCtx.moveTo(0, point);
        spectrumGridCtx.lineTo(bw, point);
    }

    spectrumGridCtx.stroke();
}

function drawFilter() {
    const bw = spectrumFilterCanvas.clientWidth;
    const bh = spectrumFilterCanvas.clientHeight;

    // Setup canvas
    spectrumFilterCanvas.width = bw;
    spectrumFilterCanvas.height = bh;

    // Clear canvas
    spectrumFilterCtx.clearRect(0, 0, bw, bh);

    if (spectrumFilterStart == null) {
        return;
    }

    // Begining of filter
    spectrumFilterCtx.beginPath();
    spectrumFilterCtx.moveTo(spectrumFilterStart, 0);
    spectrumFilterCtx.lineTo(spectrumFilterStart, bh);

    spectrumFilterCtx.strokeStyle = "black";
    spectrumFilterCtx.lineWidth = 0.5;
    spectrumFilterCtx.stroke();

    if (spectrumFilterEnd == null) {
        return;
    }

    if (spectrumFilterStart == spectrumFilterEnd) {
        return;
    }

    // End of filter
    spectrumFilterCtx.beginPath();
    spectrumFilterCtx.moveTo(spectrumFilterEnd, 0);
    spectrumFilterCtx.lineTo(spectrumFilterEnd, bh);

    spectrumFilterCtx.strokeStyle = "black";
    spectrumFilterCtx.lineWidth = 0.5;
    spectrumFilterCtx.stroke();

    // Filter area
    spectrumFilterCtx.fillStyle = "rgba(200,0,0,0.3)";
    spectrumFilterCtx.fillRect(spectrumFilterStart, 0, spectrumFilterEnd - spectrumFilterStart, bh);
}

function drawFrequencyBands() {
    const bw = bandCanvas.clientWidth;
    const bh = bandCanvas.clientHeight;

    // Setup canvas
    bandCanvas.width = bw;
    bandCanvas.height = bh;

    // Clear canvas
    bandCtx.clearRect(0, 0, bw, bh);

    bandCtx.font = "16px Georgia";
    bandCtx.fillStyle = "black";

    const textVerticalPosition = bh / 2;

    let freq = ((real_frequency) / 1000000).toFixed(2);
    let freqtext = freq + " MHz";
    let w = bandCtx.measureText(freqtext).width;
    bandCtx.fillText(freqtext, 0, textVerticalPosition);

    freq = ((real_frequency + real_bandwidth / 6 * 1) / 1000000).toFixed(2);
    freqtext = freq + " MHz";
    w = bandCtx.measureText(freqtext).width;
    bandCtx.fillText(freqtext, (bw / 6 * 1) - (w / 2), textVerticalPosition);

    freq = ((real_frequency + real_bandwidth / 6 * 2) / 1000000).toFixed(2);
    freqtext = freq + " MHz";
    w = bandCtx.measureText(freqtext).width;
    bandCtx.fillText(freqtext, (bw / 6 * 2) - (w / 2), textVerticalPosition);

    freq = ((real_frequency + real_bandwidth / 2) / 1000000).toFixed(2);
    freqtext = freq + " MHz";
    w = bandCtx.measureText(freqtext).width;
    bandCtx.fillText(freqtext, (bw / 2) - (w / 2), textVerticalPosition);

    freq = ((real_frequency + real_bandwidth / 6 * 4) / 1000000).toFixed(2);
    freqtext = freq + " MHz";
    w = bandCtx.measureText(freqtext).width;
    bandCtx.fillText(freqtext, (bw / 6 * 4) - (w / 2), textVerticalPosition);

    freq = ((real_frequency + real_bandwidth / 6 * 5) / 1000000).toFixed(2);
    freqtext = freq + " MHz";
    w = bandCtx.measureText(freqtext).width;
    bandCtx.fillText(freqtext, (bw / 6 * 5) - (w / 2), textVerticalPosition);

    freq = ((real_frequency + real_bandwidth) / 1000000).toFixed(2);
    freqtext = freq + " MHz";
    w = bandCtx.measureText(freqtext).width;
    bandCtx.fillText(freqtext, bw - w, textVerticalPosition);
}

function colorFromAmplitude(ampl) {
    let tmpVal = (ampl + 127);
    let color = colormap[tmpVal]

    return color;
}

function drawWaterfall() {
    if (waterfallData.length <= 0) {
        return;
    }

    const imageWidth = waterfallData[0].length;
    const imageHeight = waterfallData.length;

    waterfallCanvas.width = imageWidth;
    waterfallCanvas.height = waterfallMaxData;

    //console.log("waterfallWidth", imageWidth, "waterfallHeight", imageHeight);

    let waterfallImage = new ImageData(imageWidth, imageHeight);

    // Draw waterfall
    let idx = 0;
    for (var y = 0; y < waterfallData.length; y++) {
        for (var x = 0; x < waterfallData[y].length; x++) {
            var ampl = waterfallData[y][x];

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

function drawSpectrumChartValuesCanvas() {
    const bw = spectrumChartValuesCanvas.clientWidth;
    const bh = spectrumChartValuesCanvas.clientHeight;

    // Setup canvas
    spectrumChartValuesCanvas.width = bw;
    spectrumChartValuesCanvas.height = bh;

    // Clear canvas
    spectrumChartValuesCtx.clearRect(0, 0, bw, bh);

    // Draw title


    spectrumChartValuesCtx.fillStyle = 'black';

    spectrumChartValuesCtx.save();
    spectrumChartValuesCtx.translate(0, 100);
    spectrumChartValuesCtx.rotate(-Math.PI/2);
    spectrumChartValuesCtx.textAlign = "center";
    spectrumChartValuesCtx.fillText("Power (dBm)", 0, 20);
    spectrumChartValuesCtx.restore();

    //

    spectrumChartValuesCtx.fillText("123", 0, 50);
    spectrumChartValuesCtx.fillText("123", 0, 100);
    spectrumChartValuesCtx.fillText("123", 0, 150);
    spectrumChartValuesCtx.fillText("123", 0, 200);
    spectrumChartValuesCtx.fillText("123", 0, 250);
}

function drawSpectrumTimeValuesCanvas() {
    const bw = spectrumTimeValuesCanvas.clientWidth;
    const bh = spectrumTimeValuesCanvas.clientHeight;

    // Setup canvas
    spectrumTimeValuesCanvas.width = bw;
    spectrumTimeValuesCanvas.height = bh;

    // Clear canvas
    spectrumTimeValuesCtx.clearRect(0, 0, bw, bh);


    
    spectrumTimeValuesCtx.fillStyle = 'black';

    spectrumTimeValuesCtx.save();
    spectrumTimeValuesCtx.translate(0, 100);
    spectrumTimeValuesCtx.rotate(-Math.PI/2);
    spectrumTimeValuesCtx.textAlign = "center";
    spectrumTimeValuesCtx.fillText("Time", 0, 20);
    spectrumTimeValuesCtx.restore();
}

async function resizeImageData(imageData, width, height) {
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
    let frequency = document.getElementById("frequency").value;
    socket_lm.send("freq " + frequency);
}

function samplerate_change() {
    let samplerate = document.getElementById("samplerate").value;
    socket_lm.send("samplerate " + samplerate);
}

function rfbw_change() {
    let bandwidth = document.getElementById("bandwidth").value;
    socket_lm.send("rfbw " + bandwidth);
}

function rfgain_mode_change() {
    let gain_mode = document.getElementById("gain_mode").value;
    socket_lm.send("gain_mode " + gain_mode);
}

function rfgain_change() {
    let rfgain = document.getElementById("rfgain").value;
    socket_lm.send("rfgain " + rfgain);
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
        const sineWaveArray = new Int8Array(real_spectrumSamples);
        for (i = 0; i < sineWaveArray.length; i++) {
            let t = (Math.sin(i * Math.PI * 2 / hz) - 1.1) * 50;  // 0...-100
            sineWaveArray[i] = t;
        }
        waterfallData.unshift(sineWaveArray);
    }

    initialize();
    drawGrid();
    drawSpectrum();
    drawWaterfall();
}