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
