const audioBufferLength = 30;

class SamplesProcessor extends AudioWorkletProcessor {
  constructor(...args) {
    super(...args);

    this.buffers = [];
    this.lastVolumeTime = 0;
    this.maxSample = 0;
    this.numSamples = 0;

    this.port.onmessage = (event) => {
      // console.log("SamplesProcessor event " + event);

      this.buffers.push(event.data);
      // if the buffer is too big, delete it
      if (this.buffers.length > audioBufferLength) {
        console.log("SamplesProcessor buffers splice", this.buffers.length - audioBufferLength);
        this.buffers.splice(0, this.buffers.length - audioBufferLength);
      }
      console.log("SamplesProcessor buffers length", this.buffers.length);
    };
  }

  process(inputs, outputs, parameters) {
    const output = outputs[0];
    /* for (const frames of output) {
      frames.fill(0);
    } */
    // console.log('outputs', outputs.length);
    let bufferIndex, frameIndex;
    for (const frames of output) {
      bufferIndex = 0;
      frameIndex = 0;

      if (bufferIndex < this.buffers.length) {
        for (let i = 0; i < frames.length; i++) {
          const buffer = this.buffers[bufferIndex];
          if (frameIndex < buffer.length) {
            // console.log('set frame', frames, buffer);
            const v = buffer[frameIndex++];
            frames[i] = v;
            this.maxSample = Math.max(Math.abs(v), this.maxSample);
            this.numSamples++;
          } else {
            bufferIndex++;
            frameIndex = 0;
            if (bufferIndex < this.buffers.length) {
              i--;
              continue;
            } else {
              break;
            }
          }
        }
      }
    }
    if (bufferIndex > 0) {
      // console.log('finished buffer', bufferIndex);
      this.buffers.splice(0, bufferIndex);
    }
    if (frameIndex > 0) {
      this.buffers[0] = this.buffers[0].slice(frameIndex);
      if (this.buffers[0].length === 0) {
        this.buffers.shift();
      }
    }

    return true;
  }
}

registerProcessor("samples-processor", SamplesProcessor);
