myApp.controller('ClassifierCtrl', [ '$rootScope', '$scope',function($rootScope, $scope) {
	$scope.agents;
	$rootScope.$watch('configData.m_Classifiers' , function(newValue,oldValue){
		$scope.classifiers = newValue;
	});

	$scope.updateAgent= function(classifierObj){
		$rootScope.config.addObject(classifierObj, "");
	};
	
}]);