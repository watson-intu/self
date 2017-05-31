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

var ExampleAgent = function(agentName, agentId) {
	this.agentName = agentName;
	this.agentId = agentId;
}

/**
*  This is an example of how to implement your own Agent.
*  An agent should act on what is on the blackboard
*/
ExampleAgent.prototype = {
	constructor: ExampleAgent,

	/**
	*  Logic on what it should do when an IThing of interest is placed on the blackboard
	*/
	onText : function(payload) {
		var text = payload["m_Text"];
		if(text[0] == '"' && text[text.length - 1] == '"') {
			text = text.substring(1, text.length-1);
		}
		var data = {'input': {'text': text}};
		Api.setUserPayload(data);
//		addChatText("John", text);

	},
    
    onIntent : function(payload) {
        var data = JSON.stringify(payload);
        console.log(data);
        var thing = new Thing();
        thing.deserialize(payload);
        console.log(thing.guid);
        Blackboard.getInstance().getParent(thing, "");
    },

	/**
	*  Logic on when an agent starts up to declare what IThing it wants to subscribe to
	*/
	onStart : function() {
		console.log("ExampleAgent OnStart Called!");
		Blackboard.getInstance().subscribeToType("Text", ThingEventType.ADDED, "", this.onText);
        Blackboard.getInstance().subscribeToType("IIntent", ThingEventType.ADDED, "", this.onIntent);
		return true;
	},

	/**
	*  Logic on when an agent stops to declare what IThing it wants to unsubscribe to
	*/
	onStop : function() {
		console.log("ExampleAgent OnStop Called!");
		return true;
	}
}