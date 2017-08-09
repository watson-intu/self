myApp.controller("ConversationCtrl",["$rootScope", "$scope", "$timeout", function($rootScope, $scope, $timeout) {
    $scope.textObjs = [];
    $scope.sayObjs = [];
    $scope.convObjs = [];
    $scope.scrollHeight = 0;
    Blackboard.getInstance().subscribeToType("Text", ThingEventType.ADDED, "", onText);
    Blackboard.getInstance().subscribeToType("IIntent", ThingEventType.ADDED, "", onIntent);

    function onIntent(payload) {
	$scope.updateChatLog(payload);
        $scope.$apply();
    }

    function onText(payload) {
	$scope.updateChatLog(payload);
        $scope.$apply();
    }

    function onSay(payload) {
	$scope.updateChatLog(payload);
        $scope.$apply();
    }

    $scope.updateChatLog = (payload) => {
        $scope.convObjs.push(payload);
        $('.conversation-panel').animate({
          scrollTop: $('.conversation-panel')[0].scrollHeight}, 1000);
    };

    $scope.sendText = function() {
        var textObj = document.getElementById('usermsg');
        if(textObj.value != '') {
            keyboard.sendData(textObj.value);
            textObj.value = '';
        }        
    };

    
}]);
