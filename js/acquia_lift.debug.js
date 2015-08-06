/**
 * Handles making personalization debug information available to the Acquia
 * Lift debugger.
 *
 * Responsible for listening to personalizeDebEvent events and logging
 * information into local storage in a standard event format readable
 * by the debugger application.  Should be a JSON encoded object with the
 * following keys:
 * - type: 'log'
 * - severity: 'error', 'warning', 'info'
 * - message: the actual message text to display
 * - page: a path to the page where the message occurred
 * - timestamp: the unix timestamp of the error
 * - data: (OPTIONAL) Any optional data that can be displayed along
 *   with the message for additional context.
 * - resolution: (OPTIONAL) Text to provide additional insight as to the
 *   error and how to resolve it.  Make include HTML.
 */

(function ($, Drupal, Storage) {
    function getSeverity(code) {
      if (code === 2000 || code === 2020){
        return '';
      }
      if (code < 3000) {
        return 'info';
      }
      if (code < 4000) {
        return 'warning';
      }
      return 'error';
    };

    function generateUUID(){
      var d = new Date().getTime();
      var uuid = 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replace(/[xy]/g, function(c) {
        var r = (d + Math.random()*16)%16 | 0;
        d = Math.floor(d/16);
        return (c=='x' ? r : (r&0x3|0x8)).toString(16);
      });
      return uuid;
    };

    function writeToStorage(data) {
      var key = 'acquiaLift::debug::' + generateUUID();
      Storage.write(key, data);
      return key;
    };

    function clearStorage(data){
      Storage.clearStorage(data);
    }

    function getType(options){
      if (options.type){
        return options.type;
      }else{
        if (options.code === 2020 || options.code === 2000){
          return 'Developer'
        }
        return 'Drupal'
      }
    }
  // Log any personalize debug events to sessionStorage for inclusion in the
  // Acquia Lift debugger panel.
  Drupal.behaviors.acquia_lift_debug = {
    attach: function (context, settings) {
      $('body').once('acquiaLiftDebug', function () {
        $(document).on('personalizeDebugEvent', function (e, options) {
          var data = {
            type: getType(options),
            timestamp: new Date().getTime(),
            page: Drupal.settings.basePath + Drupal.settings.pathPrefix + Drupal.settings.visitor_actions.currentPath,
            message: options.message,
            severity: getSeverity(options.code),
            resolution: ''
          }
          var key = writeToStorage(data);
          // Dispatch an event to alert the debugger that new stream data is
          // available.
          $(document).trigger('acquiaLiftDebugEvent', [key]);
        });
        $(document).on('personalizeDebugClear', function(data){
          clearStorage(data);
        })
      });
    }
  }

})(Drupal.jQuery, Drupal, Drupal.personalizeStorage);
