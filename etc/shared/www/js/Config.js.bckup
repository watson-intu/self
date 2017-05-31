var Subscriber = function(callback, thing_event, path) {
	this.callback = callback;
	this.thing_event = thing_event;
	this.path = path;
}

function ConfigInstance() {
}

ConfigInstance.prototype = {
	constructor: ConfigInstance,
   start: function(callback) {
	   topicClient.subscribe("config", callback);
   },

	shutdown : function() {
			topicClient.unsubscribe("agent-society");
	},

	onConfig : function(payload) {
		return payload;
	},

	listLibs: function(thing, path) {
		var msg = {
			"event" : "list_libs",
		};
		topicClient.publish("config", msg, false);
	},

	listClasses: function(baseClass, path) {
		var msg = {
			"event" : "list_classes",
			"base_class" : baseClass
		};
		topicClient.publish(path + "config", msg, false);
	},

	disableLib: function(lib, path) {
		var msg = {
			"event" : "disable_lib",
			"lib" : lib
		};
		topicClient.publish(path + "config", msg, false);
	},

	enableLib: function(lib, path) {
		var msg = {
			"event" : "enable_lib",
			"lib" : lib
		};
		topicClient.publish(path + "config", msg, false);
	},

	addCred: function(cred, path) {
		var msg = {
			"event" : "add_cred",
			"config" : cred
		};
		topicClient.publish(path + "config", msg, false);
	},

	removeCred: function(serviceId, path) {
		var msg = {
			"event" : "enable_lib",
			"serviceId" : serviceId
		};
		topicClient.publish(path + "config", msg, false);
	},

	addObject: function(obj, path) {
		var msg = {
			"event" : "add_object",
			"object" : obj
		};
		topicClient.publish(path + "config", msg, false);
	},

	removeObject: function(objGuid, path) {
		var msg = {
			"event" : "remove_object",
			"object_guid" : objGuid
		};
		topicClient.publish(path + "config", msg, false);
	},

	logLevel: function(logReactor, logLevel, path) {
		var msg = {
			"event" : "log_level",
			"log_reactor" : logReactor,
			"log_level": logLevel
		};
		topicClient.publish(path + "config", msg, false);
	},

	getConfig: function() {
		var msg = {
			"event" : "get_config",
		};
		topicClient.publish("config", msg, false);
	}	

}


var Config = (function () {
	var instance;

	function createInstance() {
		var object = new ConfigInstance();
		console.log("Instantiating Config!");
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
