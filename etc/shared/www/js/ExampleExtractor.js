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

var ExampleExtractor = function(extractorName, extractorId) {
	this.extractorName = extractorName;
	this.extractorId = extractorId;
}

/**
*  This is an example of how to implement your own Feature Extractor.
*  An extractor should subscribe to sensors and place objects on the blackboard
*/
ExampleExtractor.prototype = {
	constructor: ExampleExtractor,
	dataType: "AudioData",
	callback: undefined,

	/**
	*  Callback when AudioData is produced from a sensor
	*/
	onAudioData : function(payload) {
		console.log("Received Received Audio Data! ");
	},

	/**
	*  Logic on when an extractor starts up to declare what sensor it wants to subscribe to
	*/
	onStart : function() {
		console.log("ExampleExtractor OnStart Called!");
		SensorManager.getInstance().registerForSensor(this.extractorId, this.onAudioData);
		return true;
	},

	/**
	*  Logic on when an extractor stops and declare what sensor it wants to unsubscribe from
	*/
	onStop : function() {
		console.log("ExampleExtractor OnStop Called!");
		SensorManager.getInstance().unregisterForSensor("AudioData");
		return true;
	}
}