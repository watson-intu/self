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
var selfId='';
var token='';
var orgId='';
var messages=[];
var subscriptionMap = new Map;
var isConnected = false;

function TopicClientInstance(host, port) {
	// Private variables
	// TODO: The path to the WebSocket server could be an argument
	var socket = new WebSocket('ws://' + host + ':' + port + '/stream?selfId=' + selfId + '&orgId=' + orgId + '&token=' + token);


	socket.onopen = function(event) {
		console.log('Performing WebSocket handshake');
		var msg = {
			"targets": [""],
			"msg": "query",
			"request": "1",
			"origin": "/."
		};

		socket.send(JSON.stringify(msg));
		isConnected = true;
		console.log('Handshake complete, readyState = ' + socket.readyState);
		console.log('Handshake complete, protocol selected by the server = ' + socket.protocol);
		if(messages.length > 0) {
			for (var i = 0; i < messages.length; i++) {
				socket.send(JSON.stringify(messages[i]));
				
			}
		}
		// TODO: Write reconnection functionality
	}

	socket.onmessage = function(event) {
		var response = JSON.parse(event.data);
		if (!response.hasOwnProperty('topic')) {
			return;
		}
		if (subscriptionMap.get(response['topic']) != undefined) {
			subscriptionMap.get(response['topic'])(response);
		}
	}

	socket.onerror = function(event) {
		console.log('An error occurred: ' + event.data);
	}

	socket.onclose = function(event) {
		console.log('Closing the connection: ' + event.data);
		socket.close();
		isConnected = false;
	}

	// Public accessors

	this.getSocket = function() {
		return socket;
	}
}

/**
*  Topic Client - low level interface to communicate with Self
*/
TopicClientInstance.prototype = {
	constructor: TopicClientInstance,


	sendMessage: function(msg) {
		msg['origin'] = selfId + '/.';
		var socket = this.getSocket();
		if(isConnected) {
			socket.send(JSON.stringify(msg));
		}
		else {
			messages.push(msg);
		}
	},

	sendBinary: function(msg, data) {
		msg['data'] = data.byteLength;
		msg['origin'] = selfId + '/.';
		utf8 = JSON.stringify(msg) + '\0';
		var arr = new Uint8Array(utf8.length + data.byteLength);
		for (var i = 0; i < utf8.length; i++) {
		    arr[i] = utf8.charCodeAt(i);
		}
		var start = 0;
		for (var j = utf8.length + 1; j < arr.length; j++) {
            arr[j] = data.getUint8(start);
            start++;
		}
		var socket = this.getSocket();
		if(isConnected) {
			socket.send(arr)
		}
		else
			console.log("not connected!");
	},

	subscribe: function(path, callback) {
		if (subscriptionMap.get(path) == undefined) {
			subscriptionMap.put(path, callback);
		}

		data = {};
		targets = [path];
		data['targets'] = targets;
		data['msg'] = 'subscribe';
		this.sendMessage(data);
	},

	unsubscribe: function(path) {
		if (subscriptionMap.get(path) != undefined) {
			subscriptionMap.remove(path);
			data = {};
			targets = [path];
			data['targets'] = targets;
			data['msg'] = 'unsubscribe';
			this.sendMessage(data);
			console.log("Unsubscribing: " + path);
		}
	},

	publish: function(path, msg, persisted) {
		data = {};
		targets = [path];
		data['targets'] = targets;
		data['msg'] = 'publish_at';
		data['data'] = JSON.stringify(msg);
		data['binary'] = false;
		data['persisted'] = persisted;
		this.sendMessage(data);

	},

	publishBinary: function(path, msg, persisted) {
		data = {};
		targets = [path];
		data['targets'] = targets;
		data['msg'] = 'publish_at';
		data['binary'] = true;
		data['persisted'] = persisted;
		this.sendBinary(data, msg);
	},

	onPong: function(buffer) {
		console.log("TopicClient onPong called");
	},
}

var TopicClient = (function () {
	var instance;

	function createInstance(host, port) {
		var object = new TopicClientInstance(host, port);
		console.log("Intantiating TopicClient!!");
		return object;
	}

	return {
		getInstance: function() {
			if(!instance) {
				instance = createInstance('127.0.0.1', 9443);
			}

			return instance;
		}
	};
})();

