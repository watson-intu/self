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

var ExampleClassifier = function(classifierName, classifierId) {
	this.classifierName = classifierName;
	this.classifierId = classifierId;
}

/**
*  This is an example of how to implement your own Classifier.
*  A classifier should refine what is placed on the blackboard
*/
ExampleClassifier.prototype = {
	constructor: ExampleClassifier,

	/**
	*  Logic on what it should do when an IThing of interest is placed on the blackboard
	*/
	onText : function(payload) {
		var text = payload["m_Text"];
		console.log("Received Text: " + text);
	},

	/**
	*  Logic on when a classifier starts up to declare what IThing it wants to subscribe to
	*/
	onStart : function() {
		console.log("ExampleClassifier OnStart Called!");
		Blackboard.getInstance().subscribeToType("Text", ThingEventType.ADDED, "", this.onText);
		return true;
	},

	/**
	*  Logic on when a classifier stops to declare what IThing it wants to unsubscribe to
	*/
	onStop : function() {
		console.log("ExampleClassifier OnStop Called!");
		Blackboard.getInstance().unsubscribeToType("Text")
		return true;
	}
}