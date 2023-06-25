var pos = 0;
var started = false;
var down = 0;
var no_last = 1;
var last_x = 0, last_y = 0;
var ctx;
var socket_lm;
var color = "#000000";
var y_offset = 256;
var spectrogram_offset = 315;
var frequency = 100000;
var spectrumgain = 0;
var real_spectrumgain = 0;
var real_frequency = 100000;
var bandwidth = 2048;
var real_bandwidth = 2048;
var conn_counter = 0;
var conn_inc = -1;
var spectrogram = new Array();
for (var idx = 0; idx < 200; idx++) {
    spectrogram[idx] = new Array();
    for (var idx2; idx2 < 1024; idx2++) {
        spectrogram[idx][idx2] = 0;
    }
}
var spectrogram_idx = -1;
var spectrogram_image;
var sound_on = false;
var samplesProcessorNode;
var audioContext;

function initialize() {
    socket_lm = new WebSocket(get_appropriate_ws_url() + "/websocket", "rtl-ws-protocol");
    console.log("WebSocket instantiated");
    try {
        socket_lm.binaryType = "arraybuffer";

        socket_lm.onopen = function () {
            document.getElementById("ws_status").src = "connect2.png";
        }

        socket_lm.onmessage = function got_packet(msg) {
            // binary frame
            const view = new DataView(msg.data);
            var firstChar = String.fromCharCode(view.getUint8(0));

            if (firstChar == 'S') {
                // Spectrum

                var bytearray = new Uint8Array(msg.data, 1);

                ctx.clearRect(0, 0, 1030, 315);
                spectrogram_idx++;
                if (spectrogram_idx >= spectrogram.length) {
                    spectrogram_idx = 0;
                }
                ctx.beginPath();
                for (var idx = 0; idx < bytearray.length; idx++) {
                    if (idx == 0) {
                        ctx.moveTo(0, -value * 4 + y_offset);
                        ctx.strokeStyle = "black";
                    }
                    var value = bytearray[idx];
                    spectrogram[spectrogram_idx][idx - 1] = value;
                    ctx.lineTo(idx, -value * 4 + y_offset);
                }
                ctx.stroke();
                ctx.closePath();

                update_spectrogram();
            } else if (firstChar == 'A') {
                // Audio
                var audioArray = new Float32Array(msg.data, 4);
                samplesProcessorNode.port.postMessage(audioArray);
            }

            return;


            var data_type = "";
            var bytearray = new Uint8Array(msg.data);
            var header = "";
            var readChar = '0';
            var header_len = 0;
            while (readChar != 'd') {
                readChar = String.fromCharCode(bytearray[header_len++]);
                header += readChar;
            }

            j = header.split(';');
            f = 0;
            var redraw_hz_axis = false;
            while (f < j.length) {
                if (j[f].charAt(0) != 'F') {
                    i = j[f].split(' ');
                    if (i[0] == 'd') {
                        if (data_type == "s") {

                        } else if (data_type == "a") {

                        }
                    } else if (i[0] == 'b') {
                        var bw_element = document.getElementById("bandwidth");
                        if ((bw_element.value * 1000) != i[1]) {
                            bw_element.style.color = "lightgray";
                        } else {
                            bw_element.style.color = "black";
                            real_bandwidth = parseInt(bandwidth);
                            redraw_hz_axis = true;
                        }
                    } else if (i[0] == 'f') {
                        var freq_element = document.getElementById("frequency");
                        if ((freq_element.value * 1000) != i[1]) {
                            freq_element.style.color = "lightgray";
                        } else {
                            freq_element.style.color = "black";
                            real_frequency = parseInt(frequency);
                            redraw_hz_axis = true;
                        }
                    } else if (i[0] == 's') {
                        var spectrumgain_element = document.getElementById("spectrumgain");
                        if (spectrumgain_element.value != i[1]) {
                            spectrumgain_element.style.color = "lightgray";
                        } else {
                            spectrumgain_element.style.color = "black";
                            real_spectrumgain = parseInt(spectrumgain);
                        }
                    } else if (i[0] == 't') {
                        data_type = i[1];
                    }
                }
                f++;
            }

            if (redraw_hz_axis) {
                ctx.font = "10px Georgia";
                ctx.fillStyle = "black";
                var freqtext = (real_frequency - real_bandwidth / 2) / 1000 + " MHz";
                var w = ctx.measureText(freqtext).width;
                ctx.fillText(freqtext, 0, 300);
                freqtext = (real_frequency) / 1000 + " MHz";
                w = ctx.measureText(freqtext).width;
                ctx.fillText(freqtext, 512 - w / 2, 300);
                freqtext = (real_frequency + real_bandwidth / 2) / 1000 + " MHz";
                w = ctx.measureText(freqtext).width;
                ctx.fillText(freqtext, 1024 - w, 300);
            }
        }

        socket_lm.onclose = function () {
            document.getElementById("ws_status").src = "connect2_close.png";
        }

        socket_lm.onerror = function () {
            document.getElementById("ws_status").src = "connect2_close.png";
        }
    } catch (exception) {
        alert('<p>Error' + exception);
    }

    var canvas = document.createElement('canvas');
    canvas.height = 500;
    canvas.width = 1030;
    ctx = canvas.getContext("2d");
    spectrogram_image = ctx.createImageData(1024, spectrogram.length);

    document.getElementById('spectrum').appendChild(canvas);

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

function update_spectrogram() {
    if (started) {
        for (var y = 0; y < spectrogram.length; y++) {
            var s_y = spectrogram_idx + y;
            s_y = ((s_y >= spectrogram.length) ? (s_y - spectrogram.length) : s_y);
            for (var x = 0; x < 1024; x++) {
                var idx = (((spectrogram.length - 1) - y) * 1024 * 4) + x * 4;
                var ampl = 0;
                try {
                    ampl = spectrogram[s_y][x];
                } catch (exception) {
                    ampl = 0;
                }
                ampl *= 8;
                ampl += 12;
                spectrogram_image.data[idx + 0] = 255 - ampl;
                spectrogram_image.data[idx + 1] = 255 - ampl;
                spectrogram_image.data[idx + 2] = 255 - ampl;
                spectrogram_image.data[idx + 3] = 255;
            }
        }
        ctx.putImageData(spectrogram_image, 0, spectrogram_offset);
    }
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
    var value = document.getElementById("start_or_stop").value;
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
    audioContext = new AudioContext({sampleRate: 48000});
    await audioContext.audioWorklet.addModule("./samples-processor.js");
    
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