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

var SpeechAgent = function(agentName, agentId) {
	this.agentName = agentName;
	this.agentId = agentId;
}

/**
*  This is an example of how to implement your own Agent.
*  This agent is interested when the embodiment is speaking
*/
SpeechAgent.prototype = {
	constructor: SpeechAgent,

	/**
	*  This function gets called when a Say object is placed on the blackboard
	*/
	onText : function(payload) {
		var text = payload["m_Text"];
		var formattedText = text.replace(/ *\[[^\]]*]/, '');
		var data = {'output': {'text': formattedText}};
		Api.setWatsonPayload(data);
	},

	/**
	*  This function gets called when the agent starts up
	*/
	onStart : function() {
		console.log("SpeechAgent OnStart Called!");
		Blackboard.getInstance().subscribeToType("Say", ThingEventType.ADDED, "", this.onText);
//		topicClient.subscribe("conversation", this.onText);
		return true;
	},

	/**
	*  This function gets called when the agent stops
	*/
	onStop : function() {
		console.log("SpeechAgent OnStop Called!");
		return true;
	}
}