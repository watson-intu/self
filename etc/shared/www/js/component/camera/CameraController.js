myApp.controller("CameraCtrl", ["$rootScope","$scope",function($rootScope, $scope) {
    console.log("In camera controller");

    $rootScope.$watchCollection("cameraSensors",function(newV, oldV) {
        if(newV != oldV && newV != null) {
            $scope.cameraParams = newV;
        }
    });

    $scope.updateType = function(type) {
        $rootScope.fullType = type;
    };
}]);