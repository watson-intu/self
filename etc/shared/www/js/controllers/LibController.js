myApp.controller('LibCtrl', [ '$rootScope', '$scope',function($rootScope, $scope) {
	
	$scope.configType = "Plugins"
	$scope.configObjs = [];
	
	$rootScope.config.listLibs();

	$rootScope.$watch('libs' , function(newValue, oldValue) {
		$scope.configObjs = newValue;
	});

	$scope.toggleLib = function(configObj) {
		if(configObj.disabled)
		  $rootScope.config.enableLib(configObj.name, "");
		else
		  $rootScope.config.disableLib(configObj.name, "");
		
	};

}]);