myApp.controller('ServiceCtrl', [ '$rootScope', '$scope',function($rootScope, $scope) {
	$scope.services;
	$rootScope.$watch('configData.m_Services' , function(newValue,oldValue){
		$scope.services = newValue;
	});

	$scope.updateAgent= function(serviceObj){
		$rootScope.config.addObject(serviceObj, "");
	};
	
}]);