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

var gestureMap = new Map;
var gestureOverrideMap = new Map;

function GestureManagerInstance() {
}

/**
*  This manager initializes all gestures for the local environment. 
*/
GestureManagerInstance.prototype = {
	constructor: GestureManagerInstance,

	/**
	*  add a new gesture to Self
	*/
	addGesture: function(gesture, override) {
		var g = gestureMap.get(gesture.gestureId);
		if(g == undefined) {
			if(gesture.onStart()) {
				var msg = {
					"event" : "add_gesture_proxy",
					"gestureId" : gesture.gestureId,
					"instanceId" : gesture.instanceId,
					"override" : override
				};
				topicClient.publish("gesture-manager", msg, false);
				gestureMap.put(gesture.gestureId, gesture);
				gestureOverrideMap.put(gesture.gestureId, override);
			}

		}
	},

	/**
	*  remove a gesture to Self
	*/
	removeGesture: function(gesture) {
		if(gestureMap.get(gesture.gestureId) != undefined) {
			gestureMap.remove(gesture.gestureId);
			gestureOverrideMap.remove(gesture.gestureId);
			var msg = {
				"event" : "remove_gesture_proxy",
				"gestureId" : gesture.gestureId,
				"instanceId" : gesture.instanceId
			};
			topicClient.publish("gesture-manager", msg, false);
		}
	},

	/**
	*  Event message from self to gesture of interest
	*/
	onEvent: function(msg) {
		var payload = JSON.stringify(msg);
		var data = JSON.parse(msg["data"]);
		if(gestureMap.get(data["gestureId"]) != undefined) {
			if(data["event"] == "execute_gesture") {
				var params = data["params"];
				gestureMap.get(data["gestureId"]).execute(params);
			}
			else if(data["event"] == "abort_gesture") {
				gestureMap.get(data["gestureId"]).abort();
			}
		}
		else {
			console.log("Gesture Id not found! " + data["gestureId"]);
		}
	},

	/**
	*  Notification when a gesture has finished
	*/
	onGestureDone: function(gestureId, instanceId, error) {
		var msg = {
			"event" : "execute_done",
			"gestureId" : gestureId,
			"instanceId" : instanceId,
			"error" : error
		};

		topicClient.publish("gesture-manager", msg, false);
	},

	/**
	*  Unsubscribe to Self
	*/
	shutdown: function() {
		topicClient.unsubscribe("gesture-manager");
	},

	/**
	*  on reconnect logic 
	*/
	onReconnect: function() {
		for(var i = 0; i++ < gestureMap.size; gestureMap.next()) {
			var sensor = gestureMap.value();
			var override = gestureOverrideMap.get(gesture.gestureId);
			var msg = {
				"event" : "add_gesture_proxy",
				"gestureId" : gesture.gestureId,
				"instanceId" : gesture.instanceId,
				"override" : override
			};

			topicClient.publish("gesture-manager", msg, false);
		}
	},

	/**
	*  Subscribes to remote Self
	*/
	start: function() {
		topicClient.subscribe("gesture-manager", this.onEvent);
	}
}

var GestureManager = (function () {
	var instance;

	function createInstance() {
		var object = new GestureManagerInstance();
		return object;
	}

	return {
		getInstance: function() {
			if(!instance) {
				instance = createInstance();
			}

			return instance;
		}
	};
})();