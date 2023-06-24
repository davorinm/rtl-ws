rtl-ws
======

Software radio based on RTL2832U-dongle.
* C server with
 * FFT calculation
 * FM demodulator
* HTML5 UI
 * Spectrum
 * Spectrogram
 * Audio output using WebAudio
* Data is transferred with WebSockets
* Server is runnable in Raspberry Pi (Rasbpian, Model B+)

Check Wiki for details.

sudo apt-get install libpthread-stubs0-dev
sudo apt-get install libwebsockets-dev
sudo apt-get install librtlsdr-dev
sudo apt-get install libfftw3-dev


Audio

https://github.com/defold/defold/blob/2b011e74971b3f42f45c8fe9bc8079ada9803acf/engine/sound/src/js/library_sound.js#L98
https://stackoverflow.com/questions/72830662/get-play-position-of-audio-buffer-in-web-audio-api
https://stackoverflow.com/questions/75184977/play-decoded-audio-buffer-with-audioworklet
https://stackoverflow.com/questions/62271985/send-audiobuffer-to-speaker
https://stackoverflow.com/questions/74803284/how-to-access-input-output-of-worklet-node-from-main-file
https://stackoverflow.com/questions/75184977/play-decoded-audio-buffer-with-audioworklet
https://developer.mozilla.org/en-US/docs/Web/API/AudioWorkletNode
https://github.com/mdn/webaudio-examples/blob/main/audioworklet/index.html
