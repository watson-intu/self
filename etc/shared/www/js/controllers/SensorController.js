myApp.controller('SensorCtrl', [ '$rootScope', '$scope',function($rootScope, $scope) {
	$scope.sensors;
	$rootScope.$watch('configData.m_Sensors' , function(newValue,oldValue){
		$scope.sensors = newValue;
	});

	$scope.updateAgent= function(sensorObj){
		$rootScope.config.addObject(sensorObj, "");
	};
	
}]);