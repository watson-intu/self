(function() {
'use strict';

  myApp.directive('conversationView', audioView);

  function audioView() {
    return {
      restrict: 'E',
      templateUrl: 'js/component/conversation/conversation.html',
      controller: 'ConversationCtrl',
      scope: {},
      link: function(scope, element, attrs) {
      }
    };
  }


})();
