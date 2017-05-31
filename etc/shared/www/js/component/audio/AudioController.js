myApp.controller("AudioCtrl", ["$rootScope", "$scope", function($rootScope, $scope) {
  console.log("In the audio controller");
  $rootScope.textObjects = [];
  Blackboard.getInstance().subscribeToType("Text", ThingEventType.ADDED, "", onText);

  function onText(payload) {
    var textObj = {"transcription": payload.m_Text, "guid": payload.GUID_ };
    $rootScope.textObjects.push(textObj);
    $rootScope.$apply();
  }
}]);