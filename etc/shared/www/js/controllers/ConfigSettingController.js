myApp.controller('ConfigSettingCtrl', [ '$rootScope', '$scope', '$location', function($rootScope, $scope, $location) {
	$scope.sensors;
	$scope.displayDetails = true;
	$scope.selectedIndex;
	$scope.configPreJSON = {
		"Type_" : ""
	};

	// $rootScope.config.getConfig();

	$scope.doneLoading = false;

	$scope.toggleDetails = function(index) {
		$scope.selectedIndex = $scope.selectedIndex != index ? index : -1;
	};

	switch($location.path()) {
		case '/agents':
		  $scope.configType = "Agents";
		  break;
		case '/classifiers':
		  $scope.configType = "Classifiers";
		  break;
		case '/extractors':
		  $scope.configType = "Extractors";
		  break;
		case '/services':
		  $scope.configType = "Services";
		  break;
		case '/sensors':
		  $scope.configType = "Sensors";
		  break;
		default:
		  $scope.configType = "Agents";
	}

	$rootScope.$watchCollection('configData' , function(newValue,oldValue){
		
		console.log(newValue, "The new config");
		console.log(oldValue, "The old config");

		$scope.doneLoading = false;
		// if(newValue !== oldValue) {
			$scope.configObjs = newValue;
			switch($scope.configType) {
				case 'Agents':
					$scope.configObjs = newValue.m_Agents;
					$rootScope.config.listClasses("IAgent", "");
					break;
				case 'Classifiers':
					$scope.configObjs = newValue.m_Classifiers;
					$rootScope.config.listClasses("IClassifier", "");
					break;
				case 'Extractors':
					$scope.configObjs = newValue.m_Extractors;
					$rootScope.config.listClasses("IExtractor", "");
					break;
				case 'Sensors':
					$scope.configObjs = newValue.m_Sensors;
					$rootScope.config.listClasses("ISensor", "");
					break;
				case 'Services':
					$scope.configObjs = newValue.m_Services;
					$rootScope.config.listClasses("IService", "");
					break;
				default:
					console.log("Wrong");
			}
		// }
	});

	$rootScope.$watch('classes', function(newValue, oldValue) {
		if(newValue && (newValue != oldValue)) {
			$scope.configPossibles = newValue;
			//$scope.configObjs = getActiveConfig( newValue, $scope.configObjs );
			$scope.doneLoading = true;
		}
		// $scope.$apply();
	});

	$scope.updateConfigObj = function(configObj){
		$rootScope.config.addObject(configObj, "");
		$("section.with-details").slideUp();
	};

	$scope.removeConfigObj = function(configObj){
		$rootScope.config.removeObject(configObj.GUID_, "");
	};

	$scope.addNewConfig = function() {
		$scope.configPreJSON = {};
		// $scope.configPreJSON.Type_ = $scope.configObjSelect.Type_;
		$scope.configPreJSON.Type_ = $scope.configObjSelect;
		$scope.configPreJSON.m_bEnabled = false;
		$(".add-obj-btn").fadeIn();
		$(".added-row").fadeOut();
		// $scope.configPreJSON = JSON.stringify($scope.configPreJSON, null, 4);
	}

	$scope.addConfigObj = function(newConfigObj) {
		// newJsonConfigObj = JSON.parse(newConfigObj);
		$rootScope.config.addObject(newConfigObj, "");
	};

	$scope.toggleConfigObj = function( configObj ) {
		if(!configObj.m_bEnabled)
		  configObj.m_bEnabled = true;
		else
			configObj.m_bEnabled = false;

		$rootScope.config.addObject(configObj, "");
	};

	function getActiveConfig(availConfig, fullConfig) {
		
			return fullConfig;
	}

	//show/hide config details
	$( document ).ready(function() {
		
		$( document ).on( 'click', '.plus-toggle', function(e) {
			
			var section = $(this).parent().parent().parent().find("section");

			if(section.hasClass("open")) {
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

		$(".add-obj-btn").click(function() {
			$(this).fadeOut();
			$(".added-row").fadeIn();
		});

	});

}]);