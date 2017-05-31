myApp.controller('ConfigCtrl', ["$rootScope", '$scope', function($rootScope, $scope) {
    $rootScope.showEntities = false;
    $rootScope.config = Config.getInstance();
    $rootScope.config.start(onConfig);
    $rootScope.cameraSensors = null;
    $rootScope.config.getConfig();
    $rootScope.convObj = null;
    $scope.convObjStr = "";
    $rootScope.configData = {};
    $rootScope.classes = [];

    $rootScope.setConvObj = function(convObj) {
        $rootScope.convObj = convObj;
        $rootScope.showEntities = true;
        $scope.convObjStr = JSON.stringify(convObj, null, 2);
    };
    
    $rootScope.closedAdvanced = function () {
        $rootScope.showEntities = false;
    };

    function onConfig(payload) {
        var jsonConfig = JSON.parse(payload.data);

        //list the config
        if(jsonConfig.config && jsonConfig.event == "config_sent") {
            $rootScope.configData = jsonConfig.config;
            $rootScope.cameraSensors = getSensorCameraData($rootScope.configData.m_Sensors);
            $rootScope.$apply();
        }

        //list the classes
        if(jsonConfig.classes && jsonConfig.success) {
            $rootScope.classes = jsonConfig.classes;
            $rootScope.$apply();
        }

        //list the libs
        if(jsonConfig.libs && jsonConfig.success) {
            $rootScope.libs = jsonConfig.libs;
            $rootScope.$apply();
        }

        if(jsonConfig.object && jsonConfig.success) {
            $rootScope.config.getConfig();
        }

        if(jsonConfig.object_guid && jsonConfig.success) {
            $rootScope.config.getConfig();
        }

        if((jsonConfig.event == "lib_disabled" || jsonConfig.event == "lib_enabled") && jsonConfig.success) {
            $rootScope.config.listLibs();
        }

        if(jsonConfig.event == "cred_added" && jsonConfig.success) {
            $rootScope.config.getConfig();
        }

        if(jsonConfig.event == "cred_removed" && jsonConfig.success) {
            $rootScope.config.getConfig();
        }
    }

    function onConfigHandler(e) {
        var jsonConfig = JSON.parse(e.detail.data);
        if(jsonConfig.config) {
            $rootScope.configData = jsonConfig.config;
            $rootScope.cameraSensors = getSensorCameraData($rootScope.configData.m_Sensors);
            $rootScope.$apply();
        }
    }

    function getSensorCameraData(sensors) {
        var cameraArry = Array();
        for(var i = 0; i < sensors.length; i++) {
            if(sensors[i].Type_ == "Camera") {
                cameraArry.push(sensors[i]);
            }
        }
        
        return cameraArry;
    }

    $rootScope.updateType = function(type) {
        $rootScope.fullType = type;
        $rootScope.showFullScreen = true;
    };

}]);