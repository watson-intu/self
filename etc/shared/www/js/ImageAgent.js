var ExampleAgent = function(agentName, agentId) {
	this.agentName = agentName;
	this.agentId = agentId;
}

ExampleAgent.prototype = {
	constructor: ExampleAgent,

	onImage : function(payload) {
		//console.log(payload, " Got The image");
		document.getElementById("intu-camera").src = 'data:image/jpeg;base64,' + payload.thing.m_Content;
		/*
		var text = payload["thing"]["m_Text"];
		if(text[0] == '"' && text[text.length - 1] == '"') {
			text = text.substring(1, text.length-1);
		}
		addChatText("John", text);
		*/

	},

	onStart : function() {
		console.log("ImageAgent OnStart Called!");
		Blackboard.getInstance().subscribeToType("Image", ThingEventType.ADDED, "", this.onImage);
		return true;
	},

	onStop : function() {
		console.log("ExampleAgent OnStop Called!");
		return true;
	}
}
