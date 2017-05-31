myApp.controller('ServicesCredsCtrl', [ '$rootScope', '$scope', '$location', function($rootScope, $scope, $location) {
	$scope.sensors;
	$scope.displayDetails = true;
	$scope.selectedIndex;
	$scope.configPreJSON = {
		"Type_" : ""
	};
	$scope.serviceCred = {};
	$scope.configType = "Service Config"

	$scope.toggleDetails = function(index) {
		$scope.selectedIndex = $scope.selectedIndex != index ? index : -1;
	};

	$rootScope.$watch('configData' , function(newValue,oldValue){
		$scope.configObjs = newValue.m_ServiceConfigs;
	});

	$scope.addServiceCred = function(serviceCred) {
		$rootScope.config.addCred(serviceCred, "");
		$scope.serviceCred = {};
	};

	/*$rootScope.$watch('classes', function(newValue, oldValue) {
		if(newValue)
		  $scope.configObjs = getActiveConfig( newValue, $scope.configObjs );
	});*/

	$scope.updateConfigObj = function(configObj){
		$rootScope.config.addCred(configObj, "");
	};

	$scope.removeConfigObj = function(configObj){
		$rootScope.config.removeCred(configObj.m_ServiceId, "");
	};

	$scope.addNewConfig = function() {
		$scope.configPreJSON = {};
		$scope.configPreJSON.Type_ = $scope.configObjSelect.Type_;
		$scope.configPreJSON = JSON.stringify($scope.configPreJSON, null, 4);
	}

	$scope.addConfigObj = function(newConfigObj) {
		newJsonConfigObj = JSON.parse(newConfigObj);
		$rootScope.config.addObject(newJsonConfigObj, "");
	};

	function getActiveConfig(availConfig, fullConfig) {
		if(fullConfig && availConfig) {
			for(var i = 0; i < fullConfig.length; i++) {
				var currentConfig = fullConfig[i].Type_;
				if(availConfig.indexOf(currentConfig) == -1)
					fullConfig.splice(i, 1);
			}
		}
		return fullConfig;
	}

	//show/hide config details
	$( document ).ready(function() {
		
		$( document ).on( 'click', '.plus-toggle', function() {

			var section = $(this).parent().parent().parent().find("section");

			if(section.hasClass("open")) {
				section.removeClass("open");
				section.slideUp();
			}else{
				section.addClass("open");
				section.slideDown();
			}
			
		});

		$("select.add-obj").change(function() {
			$(this).siblings("section").slideDown();
		});

		$(".cancel-btn").click(function() {
			$(".new-config-info").slideUp();
		});

	});

}]);