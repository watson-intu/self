myApp.controller("ConversationCtrl",["$rootScope", "$scope", function($rootScope, $scope) {
    $scope.textObjs = [];
    $scope.sayObjs = [];
    $scope.convObjs = [];
    Blackboard.getInstance().subscribeToType("Text", ThingEventType.ADDED, "", onText);
    Blackboard.getInstance().subscribeToType("IIntent", ThingEventType.ADDED, "", onIntent);

    function onIntent(payload) {
        $scope.convObjs.push(payload);
        $scope.$apply();
    }

    function onText(payload) {
        $scope.convObjs.push(payload);
        $scope.$apply();
    }

    function onSay(payload) {
        $scope.convObjs.push(payload);
        $scope.$apply();
    }

    $scope.sendText = function() {
        var textObj = document.getElementById('usermsg');
        if(textObj.value != '') {
            keyboard.sendData(textObj.value);
            textObj.value = '';
        }        
    };

    
}]);
