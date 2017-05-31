(function() {
'use strict';

  myApp.directive('cameraView', audioView);

  function audioView() {
    return {
      restrict: 'E',
      templateUrl: 'js/component/camera/camera.html',
      scope: {
        cameraParams: '='
      },
      controller: 'CameraCtrl',
      link: function(scope, element, attrs) {
        scope.cameraName = "Camera";
        console.log("I'm in camera directive");
        console.log(scope.cameraParams, "The params");
        scope.$watch('cameraParams', function(newVal) {
                scope.cameraParams = newVal;
        }, true);

        scope.update = function() {
          scope.cameraName = scope.item.m_SensorName;
        };
      }
    };
  }


})();
