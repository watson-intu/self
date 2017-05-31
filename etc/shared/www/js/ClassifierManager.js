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

function ClassifierManagerInstance() {
	console.log("ClassifierManager has been instantiated!!");
}

/**
*  This manager initializes all classifiers for the local environment. 
*/
ClassifierManagerInstance.prototype = {
	constructor: ClassifierManagerInstance,

	isRegistered : function(classifier) {
		for(var i = 0; i++ < classifierMap.size; classifierMap.next()) {
			var a = classifierMap.value();
			if(classifier.classifierId == a.classifierId) {
				return true;
			}
		}

		return false;
	},

	/**
	*  Add a classifier to Self
	*/
	addClassifier : function(classifier, override) {
		if(classifierMap.get(classifier.classifierId) == undefined) {
			classifierMap.put(classifier.classifierId, classifier);
			classifierOverrideMap.put(classifier.classifierId, override);
			var msg = {
				"event" : "add_classifier_proxy",
				"classifierId" : classifier.classifierId,
				"name" : classifier.classifierName,
				"override" : override
			};
			topicClient.publish("classifier-manager", msg, false);
			console.log("Adding Classifier to Classifier Manager: " + classifier.classifierId);
		}
	},

	/**
	*  Remove a classifier to Self
	*/
	removeClassifier : function(classifier) {
		if(classifierMap.get(classifier.classifierId) != undefined) {
			classifierMap.remove(classifier.classifierId);
			var msg = {
				"event" : "remove_classifier_proxy",
				"classifierId" : classifier.classifierId
			};
			topicClient.publish("classifier-manager", msg, false);
		}
	},

	/**
	*  Event message from Self to classifier of interest
	*/
	onEvent : function(msg) {
		var payload = JSON.stringify(msg);
		var data = JSON.parse(msg["data"]);
		if(classifierMap.get(data["classifierId"]) != undefined) {
			if(data["event"] == "start_classifier") {
				classifierMap.get(data["classifierId"]).onStart();
			}
			else if(data["event"] == "stop_classifier") {
				classifierMap.get(data["classifierId"]).onStop();
			}
			else {
				console.log("Could not interpet event for ClassifierManager: " + data["event"]);
			}
		}
		else {
			console.log("Classifier Id not found! " + data["classifierId"]);
		}
	},

	/**
	*  Unsubscribes from remote Self
	*/
	shutdown : function() {
		topicClient.unsubscribe("classifier-manager");
	},

	/**
	*  Notified when disconnect occurs
	*/
	onDisconnect : function() {
		for(var i = 0; i++ < classifierMap.size; classifierMap.next()) {
			var classifier = classifierMap.value();
			classifier.onStop();
		}
	},

	/**
	*  Reconnect logic
	*/
	onReconnect : function() {
		for(var i = 0; i++ < classifierMap.size; classifierMap.next()) {
			var classifier = classifier.value();
			var override = classifierOverrideMap.get(classifier.classifierId);
			var msg = {
				"event" : "add_classifier_proxy",
				"classifierId" : classifier.classifierId,
				"name" : classifier.classifierName,
				"override" : override
			};
			topicClient.publish("classifier-manager", msg, false);
		}
	},

	/**
	*  Start subscription to Self
	*/
	start: function() {
		topicClient.subscribe("classifier-manager", this.onEvent);
	}
}

var ClassifierManager = (function () {
	var instance;

	function createInstance() {
		var object = new ClassifierManagerInstance();
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