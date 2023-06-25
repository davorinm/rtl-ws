const audioBufferLength = 30;

class SamplesProcessor extends AudioWorkletProcessor {
  constructor(...args) {
    super(...args);

    this.audioBuffers = [];

    this.processedAudioBuffer = null;
    this.processedAudioBufferIndex = 0;

    this.port.onmessage = (event) => {
      // console.log("SamplesProcessor event " + event);

      let length = this.audioBuffers.push(event.data);
      console.log("SamplesProcessor added buffer, count", length);

      // if the buffer is too big, delete it
      if (this.audioBuffers.length > audioBufferLength) {
        this.audioBuffers.splice(0, this.audioBuffers.length - audioBufferLength);
        console.log("SamplesProcessor buffers splice", this.audioBuffers.length - audioBufferLength, "new count", this.audioBuffers.length);
      }
    };
  }

  process(inputs, outputs, parameters) {
    const samples = outputs[0][0];
    let bufferIndex = 0;
    let frameIndex = 0;

    // Fill samples
    for (let i = 0; i < samples.length; i++) {

      // Check if processing buffer exists
      if (this.processedAudioBuffer == null) {
        // Bufferred length
        if (this.audioBuffers.length < 5) {
          return true
        }

        // Get buffer
        this.processedAudioBuffer = this.audioBuffers.shift();
        console.log("SamplesProcessor processing new buffer", this.audioBuffers.length);

        // Check again
        if (this.processedAudioBuffer == null) {
          return true
        }

        // New buffer, new index
        this.processedAudioBufferIndex = 0;
      }

      samples[i] = this.processedAudioBuffer[this.processedAudioBufferIndex++];

      if (this.processedAudioBufferIndex >= this.processedAudioBuffer.length) {
        this.processedAudioBuffer = null;
      }
    }

    return true;
  }
}

registerProcessor("samples-processor", SamplesProcessor);
