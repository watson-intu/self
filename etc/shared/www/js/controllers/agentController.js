myApp.controller('AgentCtrl', [ '$rootScope', '$scope',function($rootScope, $scope) {
	$scope.agents;
	$rootScope.$watch('configData.m_Agents' , function(newValue,oldValue){
		$scope.agents = newValue;
	});

	$scope.updateAgent= function(agent){
		$rootScope.config.addObject(agent, "");
	}

	$scope.updateChange = function(data) {
	
	};
}]);