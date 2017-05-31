/**
* Copyright 2017 IBM Corp. All Rights Reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
*/

function MicrophoneSensor() {
	console.log("microphone sensor instantiated!");

}

var micPaused = true;

var resampler = function(sampleRate, audioBuffer, micSensor, callbackProcessAudio) {

  console.log('length: ' + audioBuffer.length + ' ' + sampleRate);
  var channels = 1;
  var targetSampleRate = 16000;
  var numSamplesTarget = audioBuffer.length * targetSampleRate / sampleRate;

  var offlineContext = new OfflineAudioContext(channels, numSamplesTarget, targetSampleRate);
  var bufferSource = offlineContext.createBufferSource();
  bufferSource.buffer = audioBuffer;

  // callback that is called when the resampling finishes
  offlineContext.oncomplete = function(event) {
    var samplesTarget = event.renderedBuffer.getChannelData(0);
    console.log('Done resampling: ' + samplesTarget.length + ' samples produced');

  // convert from [-1,1] range of floating point numbers to [-32767,32767] range of integers
  var index = 0;
  var volume = 0x7FFF;
    var pcmEncodedBuffer = new ArrayBuffer(samplesTarget.length*2);    // short integer to byte
    var dataView = new DataView(pcmEncodedBuffer);
    for (var i = 0; i < samplesTarget.length; i++) {
      dataView.setInt16(index, samplesTarget[i]*volume, true);
      index += 2;
    }

    // l16 is the MIME type for 16-bit PCM
    callbackProcessAudio(micSensor,dataView);
  };

  bufferSource.connect(offlineContext.destination);
  bufferSource.start(0);
  offlineContext.startRendering();
};

function onAudio(sensor, data) {
    sensorManager.sendData(sensor, data);
};

/**
*  This is an example of how to implement your own Microphone sensor.
*  A sensor should interface hardware and produce IData objects
*/
MicrophoneSensor.prototype = {
	sensorId: GUID(),
	sensorName: "Microphone",
	dataType: "AudioData",
	binaryType: "audio/L16;rate=16000;channels=1",
    
	/**
	*  Function that will get called to start the sensor
	*/
	onStart: function() {
		console.log("Microphone Sensor has started!");
		console.log(this, "Microphone object");
		var micSensor = this;
        
        var audio = this.onAudio;
        
		var streamAudio = function(stream) {
	    	var context = new AudioContext();
	    	var input = context.createMediaStreamSource(stream)
	    	var processor = context.createScriptProcessor(4096,1,1);
            input.connect(processor);
	  //  	var analyser = context.createAnalyser();

	    	processor.connect(context.destination);

	    	processor.onaudioprocess = function(e){
	      		if(!micPaused) {
                    resampler(this.context.sampleRate, e.inputBuffer, micSensor, onAudio)
	      		}
	    	};
  		};

  		navigator.mediaDevices.getUserMedia({ audio: true, video: false })
      		.then(streamAudio);
		return true;
	},

	/**
	*  Function that will be called to stop the sensor
	*/
	onStop: function() {
		return true;
	},

	/**
	*  Pause the sensor
	*/
	onPause: function() {
		console.log("Pausing microphone!");
		micPaused = true;
		return true;
	},

	/*
	*  Resume the sensor
	*/
	onResume: function() {
		console.log("Resuming microphone");
		micPaused = false;
		return true;
	},
}