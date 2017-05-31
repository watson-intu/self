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

function KeyboardSensor() {
	console.log("keyboard sensor instantiated!");
}

/**
*  This is an example of how to implement your own Sensor.
*  A sensor should interface hardware and produce IData objects
*/
KeyboardSensor.prototype = {
	sensorId: "asdf",
	sensorName: "keyboard",
	dataType: "TextData",
	binaryType: "KeyboardData",

	/**
	*  Function that will get called to start the sensor
	*/
	onStart: function() {
		console.log("Keyboard Sensor has started!");
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
		return true;
	},

	/**
	*  Resume the sensor
	*/
	onResume: function() {
		return true;
	},

	/**
	*  Send data feature extractors
	*/
	sendData: function(value) {
		topicClient.publish("conversation", value, false);
	}
}