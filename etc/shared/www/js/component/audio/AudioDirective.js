(function() {
'use strict';

  myApp.directive('audioView', audioView);

  function audioView() {
    return {
      restrict: 'E',
      templateUrl: 'js/component/audio/audio.html',
      controller: 'AudioCtrl',
      scope: {},
      link: function(scope, element, attrs) {
        
      }
    };
  }


})();
