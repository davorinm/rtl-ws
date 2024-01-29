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

let spectrumRange = [5, -105];

let spectrumFilterCanvas;
let spectrumFilterCtx;
let spectrumFilterStartHz;
let spectrumFilterWidthHz;

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
    if (spectrumMouseDown) {
        return;
    }

    spectrumMouseDown = true;

    const elementRelativeX = e.offsetX;

    const selectedProportion = elementRelativeX / spectrumCanvas.clientWidth;
    const selected_freq = real_samplerate * selectedProportion;
    const filter_freq = selected_freq + real_frequency;

    spectrumFilterStartHz = filter_freq;
    spectrumFilterWidthHz = null;

    document.getElementById("tuner_frequency").value = '' + Math.round(spectrumFilterStartHz);
    document.getElementById("tuner_filter_width").value = '';

    drawFilter();
}

function spectrumMoveListener(e) {
    if (!spectrumMouseDown) {
        return;
    }

    const elementRelativeX = e.offsetX;

    const selectedProportion = elementRelativeX / spectrumCanvas.clientWidth;
    const selected_freq = real_samplerate * selectedProportion;
    const filter_freq = selected_freq + real_frequency;

    spectrumFilterWidthHz = filter_freq - spectrumFilterStartHz;

    if (spectrumFilterWidthHz != 0) {
        document.getElementById("tuner_filter_width").value = '' + Math.round(spectrumFilterWidthHz);
    }

    drawFilter();
}

function spectrumUpListener(e) {
    spectrumMouseDown = false;

    const elementRelativeX = e.offsetX;

    const selectedProportion = elementRelativeX / spectrumCanvas.clientWidth;
    const selected_freq = real_samplerate * selectedProportion;
    const filter_freq = selected_freq + real_frequency;

    spectrumFilterWidthHz = filter_freq - spectrumFilterStartHz;

    if (spectrumFilterWidthHz != 0) {
        document.getElementById("tuner_filter_width").value = '' + Math.round(spectrumFilterWidthHz);
    }

    drawFilter();
}

//// Waterfall listeners

let waterfallMouseDown = false;

function waterfallDownListener(e) {
    if (waterfallMouseDown) {
        return;
    }

    waterfallMouseDown = true;

    const elementRelativeX = e.offsetX;

    const selectedProportion = elementRelativeX / spectrumCanvas.clientWidth;
    const selected_freq = real_samplerate * selectedProportion;
    const filter_freq = selected_freq + real_frequency;

    spectrumFilterStartHz = filter_freq;
    spectrumFilterWidthHz = null;

    document.getElementById("tuner_frequency").value = '' + Math.round(spectrumFilterStartHz);
    document.getElementById("tuner_filter_width").value = '';

    drawFilter();
}

function waterfallMoveListener(e) {
    if (!waterfallMouseDown) {
        return;
    }

    const elementRelativeX = e.offsetX;

    const selectedProportion = elementRelativeX / spectrumCanvas.clientWidth;
    const selected_freq = real_samplerate * selectedProportion;
    const filter_freq = selected_freq + real_frequency;

    spectrumFilterWidthHz = filter_freq - spectrumFilterStartHz;

    if (spectrumFilterWidthHz != 0) {
        document.getElementById("tuner_filter_width").value = '' + Math.round(spectrumFilterWidthHz);
    }

    drawFilter();
}

function waterfallUpListener(e) {
    waterfallMouseDown = false;
    
    const elementRelativeX = e.offsetX;

    const selectedProportion = elementRelativeX / spectrumCanvas.clientWidth;
    const selected_freq = real_samplerate * selectedProportion;
    const filter_freq = selected_freq + real_frequency;

    spectrumFilterWidthHz = filter_freq - spectrumFilterStartHz;

    if (spectrumFilterWidthHz != 0) {
        document.getElementById("tuner_filter_width").value = '' + Math.round(spectrumFilterWidthHz);
    }

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

    // Elements events
    let connection_button_element = document.getElementById('connection_button');
    connection_button_element.addEventListener("click", connect);

    let start_or_stop_element = document.getElementById('start_or_stop');
    start_or_stop_element.addEventListener("click", start_or_stop);
    
    let tuner_frequency_element = document.getElementById('tuner_frequency');
    tuner_frequency_element.addEventListener("input", tuner_frequency_change);

    let tuner_filter_width_element = document.getElementById('tuner_filter_width');
    tuner_filter_width_element.addEventListener("input", tuner_filter_width_change);

    let frequency_element = document.getElementById('frequency');
    frequency_element.addEventListener("input", frequency_change);

    let samplerate_element = document.getElementById('samplerate');
    samplerate_element.addEventListener("input", samplerate_change);

    let bandwidth_element = document.getElementById('bandwidth');
    bandwidth_element.addEventListener("input", rfbw_change);

    let gain_mode_element = document.getElementById('gain_mode');
    gain_mode_element.addEventListener("change", rfgain_mode_change);

    let rfgain_element = document.getElementById('rfgain');
    rfgain_element.addEventListener("change", rfgain_change);

    let toggle_sound_element = document.getElementById('toggle_sound');
    toggle_sound_element.addEventListener("click", toggle_sound);

    // Initial values
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
    const scale = window.devicePixelRatio;

    // Setup canvas
    spectrumCanvas.width = Math.floor(bw * scale);
    spectrumCanvas.height = Math.floor(bh * scale);
    spectrumCtx.scale(scale, scale);

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
        const yPos = mapToSpectrumRange(value, bh);
        if (idx == 0) {
            spectrumCtx.moveTo(0, yPos);
        }
        spectrumCtx.lineTo(spectrumCtxPosHorizontal, yPos);
        spectrumCtxPosHorizontal += increment;
    }

    spectrumCtx.stroke();
    spectrumCtx.closePath();
}

// 0 ... -100dBm to 0 ... 230px
// Map dBm to px
function mapToSpectrumRange(value, canvasHeight) {
    const minRange = spectrumRange[0];
    const maxRange = spectrumRange[1];

    if (minRange < maxRange) {
        return null;
    }

    let range = maxRange - minRange;

    let mul = canvasHeight / range;
    let result = (value - minRange) * mul;

    return result;
}

function drawGrid() {
    const bw = spectrumGridCanvas.clientWidth;
    const bh = spectrumGridCanvas.clientHeight;
    const scale = window.devicePixelRatio;

    // Setup canvas
    spectrumGridCanvas.width = Math.floor(bw * scale);
    spectrumGridCanvas.height = Math.floor(bh * scale);
    spectrumGridCtx.scale(scale, scale);

    // Clear canvas
    spectrumGridCtx.clearRect(0, 0, bw, bh);

    let ttt = real_bandwidth / real_spectrumSamples;

    spectrumGridCtx.strokeStyle = "black";
    spectrumGridCtx.lineWidth = 1;

    // Vertical lines
    const verticalDiv = 6;
    for (var div = 0; div <= verticalDiv; div++) {
        let point = bw * (div / verticalDiv);
        if (point == 0) {
            point = 1;
        }
        spectrumGridCtx.moveTo(point, 1);
        spectrumGridCtx.lineTo(point, bh);
    }

    // Horizontal lines
    for (var i = spectrumRange[0]; i >= spectrumRange[1]; i--) {
        if (i % 10 != 0) {
            continue
        }
        let y = mapToSpectrumRange(i, bh);
        spectrumGridCtx.moveTo(0, y);
        spectrumGridCtx.lineTo(bw, y);
    }

    spectrumGridCtx.stroke();
}

function drawFilter() {
    const bw = spectrumFilterCanvas.clientWidth;
    const bh = spectrumFilterCanvas.clientHeight;
    const scale = window.devicePixelRatio;

    // Setup canvas
    spectrumFilterCanvas.width = Math.floor(bw * scale);
    spectrumFilterCanvas.height = Math.floor(bh * scale);
    spectrumFilterCtx.scale(scale, scale);

    // Clear canvas
    spectrumFilterCtx.clearRect(0, 0, bw, bh);

    if (spectrumFilterStart == null) {
        return;
    }

    // Draw begining of filter
    spectrumFilterCtx.beginPath();
    spectrumFilterCtx.moveTo(spectrumFilterStartPx, 0);
    spectrumFilterCtx.lineTo(spectrumFilterStartPx, bh);

    spectrumFilterCtx.strokeStyle = "black";
    spectrumFilterCtx.lineWidth = 0.5;
    spectrumFilterCtx.stroke();

    if (spectrumFilterEnd == null) {
        return;
    }

    // If start and stop are same dismiss drawing
    if (spectrumFilterStartPx == spectrumFilterEndPx) {
        return;
    }

    // Draw end of filter
    spectrumFilterCtx.beginPath();
    spectrumFilterCtx.moveTo(spectrumFilterEndPx, 0);
    spectrumFilterCtx.lineTo(spectrumFilterEndPx, bh);

    spectrumFilterCtx.strokeStyle = "black";
    spectrumFilterCtx.lineWidth = 0.5;
    spectrumFilterCtx.stroke();

    // Filter area
    spectrumFilterCtx.fillStyle = "rgba(200,0,0,0.3)";
    spectrumFilterCtx.fillRect(spectrumFilterStartPx, 0, spectrumFilterEndPx - spectrumFilterStartPx, bh);
}

function drawFrequencyBands() {
    const bw = bandCanvas.clientWidth;
    const bh = bandCanvas.clientHeight;
    const scale = window.devicePixelRatio;

    // Setup canvas
    bandCanvas.width = Math.floor(bw * scale);
    bandCanvas.height = Math.floor(bh * scale);
    bandCtx.scale(scale, scale);

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
    const scale = window.devicePixelRatio;

    // Setup canvas
    spectrumChartValuesCanvas.width = Math.floor(bw * scale);
    spectrumChartValuesCanvas.height = Math.floor(bh * scale);
    spectrumChartValuesCtx.scale(scale, scale);

    // Clear canvas
    spectrumChartValuesCtx.clearRect(0, 0, bw, bh);

    // Draw title
    spectrumChartValuesCtx.save();
    spectrumChartValuesCtx.rotate(-Math.PI / 2);
    spectrumChartValuesCtx.textAlign = "center";
    spectrumChartValuesCtx.fillText("Power (dBm)", -(bh/2), 17);
    spectrumChartValuesCtx.restore();

    // Draw values
    spectrumChartValuesCtx.textAlign = "right";
    for (var i = spectrumRange[0]; i >= spectrumRange[1]; i--) {
        if (i % 10 != 0) {
            continue
        }

        let y = mapToSpectrumRange(i, bh);
        spectrumChartValuesCtx.fillText("" + i, bw - 5, y+2);
    }
}

function drawSpectrumTimeValuesCanvas() {
    const bw = spectrumTimeValuesCanvas.clientWidth;
    const bh = spectrumTimeValuesCanvas.clientHeight;
    const scale = window.devicePixelRatio;

    // Setup canvas
    spectrumTimeValuesCanvas.width = Math.floor(bw * scale);
    spectrumTimeValuesCanvas.height = Math.floor(bh * scale);
    spectrumTimeValuesCtx.scale(scale, scale);

    // Clear canvas
    spectrumTimeValuesCtx.clearRect(0, 0, bw, bh);

    // Draw title
    spectrumTimeValuesCtx.save();
    spectrumTimeValuesCtx.rotate(-Math.PI / 2);
    spectrumTimeValuesCtx.textAlign = "center";
    spectrumTimeValuesCtx.fillText("Time", -(bh/2), 17);
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

/// Actions

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
            let t = (Math.sin(i * Math.PI * 2 / hz) - 1.5) * 35;  // 0...-100
            sineWaveArray[i] = t;
        }
        waterfallData.unshift(sineWaveArray);
    }

    initialize();
    drawGrid();
    drawSpectrum();
    drawWaterfall();
}

function tuner_frequency_change() {
    let value = document.getElementById("tuner_frequency").value;

    spectrumFilterStartHz = parseInt(value);

    drawFilter();
}

function tuner_filter_width_change() {
    let value = document.getElementById("tuner_filter_width").value;

    spectrumFilterWidthHz = parseInt(value);

    drawFilter();
}