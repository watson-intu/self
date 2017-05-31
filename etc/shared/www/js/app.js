var myApp = angular.module('dashboard', ['ngRoute']);

myApp.config(['$locationProvider','$routeProvider', function ($locationProvider, $routeProvider) {
    $locationProvider.hashPrefix('');
    $routeProvider
        .when('/', {
            templateUrl: 'js/partials/main.html'
        })
        .when("/agents", {
            templateUrl: 'js/partials/intu-config-settings.html',
            controller: 'ConfigSettingCtrl'
        })
        .when("/classifiers", {
            templateUrl: 'js/partials/intu-config-settings.html',
            controller: 'ConfigSettingCtrl'
        })
        .when("/extractors", {
            templateUrl: 'js/partials/intu-config-settings.html',
            controller: 'ConfigSettingCtrl'
        })        
        .when("/services", {
            templateUrl: 'js/partials/intu-config-settings.html',
            controller: 'ConfigSettingCtrl'
        })
        .when("/sensors", {
            templateUrl: 'js/partials/intu-config-settings.html',
            controller: 'ConfigSettingCtrl'
        })
        .when("/Auth", {
            templateUrl: 'js/partials/intu-authorization.html',
            controller: 'AuthCtrl'
        })
        .when("/service-creds", {
            templateUrl: 'js/partials/intu-services-settings.html',
            controller: 'ServicesCredsCtrl'
        })
        .when("/libs", {
            templateUrl: 'js/partials/intu-lib-settings.html',
            controller: 'LibCtrl'
        })
        
        .otherwise({ redirectTo: '/' });
}]);