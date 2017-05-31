(function() {
'use strict';

  myApp.directive('menuView', audioView);

  function audioView() {
    return {
      restrict: 'E',
      templateUrl: 'js/component/menu/menu.html',
      scope: {},
      link: function(scope, element, attrs) {
        scope.menuToggle = false;
        scope.toggleConfig = function(){
          scope.configToggle = !scope.configToggle;
        };
        scope.closeMenu = function() {
          scope.configToggle = false;
          scope.menuToggle = false;
        }
        scope.toggleMenu = function() {
          scope.menuToggle = !scope.menuToggle;
          if(!scope.menuToggle)
          { 
            scope.configToggle = false;
          }
        };

      }

    };
  }


})();
