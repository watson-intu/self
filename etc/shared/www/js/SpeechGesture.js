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

var SpeechGesture = function(gestureId, instanceId) {
	this.gestureId = gestureId;
	this.instanceId = instanceId;
}

/**
*  This is an example of how to implement your own gesture.
*  A gesture is something that produces some type of output
*/
SpeechGesture.prototype = {
	constructor: SpeechGesture,

	/**
	*  This function gets called when the gesture has been registered to Self
	*/
	onStart : function() {
		console.log("SpeechGesture OnStart Called!");
		return true;
	},

	/**
	*  This function gets called when the gesture is unregistered to Self
	*/
	onStop : function() {
		console.log("SpeechGesture OnStop Called!");
		return true;
	},

	/**
	* This function gets called when Self indicates it needs to execute
	*/
	execute : function(params) {
		var text = params["text"];
		var language = params["language"];
		var gender = params["gender"];

//		addChatText("Watson", text);
		console.log("SpeechGesture: " + text);
		var data = {'output': {'text': text}};
		Api.setWatsonPayload(data);
		gestureManager.onGestureDone(this.gestureId, this.instanceId);
		return true;
	},

	/**
	*  Aborts current execution of gesture
	*/
	abort : function() {
		return true;
	}
}