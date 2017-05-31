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

var agentMap = new Map;
var agentOverrideMap = new Map;

function AgentSocietyInstance() {
	console.log("AgentSociety has been instantiated!!");
}

/**
*  This manager initializes all agents for the local environment. 
*/
AgentSocietyInstance.prototype = {
	constructor: AgentSocietyInstance,

	isRegistered : function(agent) {
		for(var i = 0; i++ < agentMap.size; agentMap.next()) {
			var a = agentMap.value();
			if(agent.agentId == a.agentId) {
				return true;
			}
		}

		return false;
	},

	/**
	*  Add an agent to Self
	*/
	addAgent : function(agent, override) {
		if(agentMap.get(agent.agentId) == undefined) {
			agentMap.put(agent.agentId, agent);
			agentOverrideMap.put(agent.agentId, override);
			var msg = {
				"event" : "add_agent_proxy",
				"agentId" : agent.agentId,
				"name" : agent.agentName,
				"override" : override
			};
			topicClient.publish("agent-society", msg, false);
			console.log("Adding Agent to Agent Society: " + agent.agentId);
		}
	},

	/**
	*  Removes an agent from Self
	*/
	removeAgent : function(agent) {
		if(agentMap.get(agent.agentId) != undefined) {
			agentMap.remove(agent.agentId);
			var msg = {
				"event" : "remove_agent_proxy",
				"agentId" : agent.agentId
			};
			topicClient.publish("agent-society", msg, false);
		}
	},

	/**
	*  Event message from Self to an agent of interest
	*/
	onEvent : function(msg) {
		var payload = JSON.stringify(msg);
		var data = JSON.parse(msg["data"]);
		if(agentMap.get(data["agentId"]) != undefined) {
			if(data["event"] == "start_agent") {
				agentMap.get(data["agentId"]).onStart();
			}
			else if(data["event"] == "stop_agent") {
				agentMap.get(data["agentId"]).onStop();
			}
			else {
				console.log("Could not interpet event for AgentSociety: " + data["event"]);
			}
		}
		else {
			console.log("Agent Id not found! " + data["agentId"]);
		}
	},

	/**
	*  Unsubscribes from Self
	*/
	shutdown : function() {
		topicClient.unsubscribe("agent-society");
	},

	/**
	*  When a disconnect event occurs
	*/
	onDisconnect : function() {
		for(var i = 0; i++ < agentMap.size; agentMap.next()) {
			var agent = agentMap.value();
			agent.onStop();
		}
	},

	/**
	*  Reconnect logic
	*/
	onReconnect : function() {
		for(var i = 0; i++ < agentMap.size; agentMap.next()) {
			var agent = agent.value();
			var override = agentOverrideMap.get(agent.agentId);
			var msg = {
				"event" : "add_agent_proxy",
				"agentId" : agent.agentId,
				"name" : agent.agentName,
				"override" : override
			};
			topicClient.publish("agent-society", msg, false);
		}
	},

	/**
	*  Start the subscription to Self
	*/
	start: function() {
		topicClient.subscribe("agent-society", this.onEvent);
	}
}

var AgentSociety = (function () {
	var instance;

	function createInstance() {
		var object = new AgentSocietyInstance();
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