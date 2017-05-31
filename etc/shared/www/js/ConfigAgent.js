var ConfigAgent = function(agentName, agentId) {
	this.agentName = agentName;
	this.agentId = agentId;
}

ConfigAgent.prototype = {
	constructor: ConfigAgent,

	onConfig : function(payload) {
		console.log(payload, " Got the config");
		//document.getElementById("intu-camera").src = 'data:image/jpeg;base64,' + payload.m_Content;
	},

	onStart : function() {
		console.log("ConfigAgent OnStart Called!");
		Blackboard.getInstance().subscribeToType("Config", ThingEventType.ADDED, "", this.onConfig);
		return true;
	},

	onStop : function() {
		console.log("ConigAgent OnStop Called!");
		return true;
	},
	getConfig : function() {
		var msg = {
			"event" : "get_config",
		};
		topicClient.publish("config", msg, false);
		//Config.getInstance().getConfig("");
		return true;
	}
}
