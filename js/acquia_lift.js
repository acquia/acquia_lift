var _tcaq = _tcaq || [];
var _tcwq = _tcwq || [];

(function ($, Drupal, settings) {

  "use strict";

  function generateTrackingId(){
    var d = new Date().getTime();
    var uuid = 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replace(/[xy]/g,
      function(c) {
        var r = (d + Math.random()*16)%16 | 0;
        d = Math.floor(d/16);
        return (c=='x' ? r : (r&0x7|0x8)).toString(16);
      });
    return uuid;
  }
  settings.pageContext.trackingId = generateTrackingId();

  Drupal.behaviors.acquia_lift = {
    attach: function () {
      Drupal.acquia_lift.initialize();
    }
  };

  Drupal.acquia_lift = {};

  /**
   * Initialize.
   */
  Drupal.acquia_lift.initialize = function () {
    // Set account.
    _tcaq.push(['setAccount', settings.credential.account_name, settings.credential.customer_site]);

    // Capture view.
    _tcaq.push([ 'captureView', 'Content View', settings.pageContext]);

    // Capture identity.
    if(settings.hasOwnProperty('identity')) {
      _tcaq.push(['captureIdentity', settings.identity.identity, settings.identity.identityType]);
    }

    // Load and run capture scripts.
    (function() {
      function async_load() {
        var script = document.createElement('script'),
          firstScript = document.getElementsByTagName('script')[0];
        script.type = 'text/javascript';
        script.async = true;
        script.src = ('https:' == document.location.protocol ? 'https://' : 'http://') + settings.credential.js_path;
        firstScript.parentNode.insertBefore(script, firstScript);
      }
      if (window.attachEvent) {
        window.attachEvent('onload', async_load);
      }
      else {
        window.addEventListener('load', async_load, false);
      }
    })();
  };

})(jQuery, Drupal, drupalSettings.acquia_lift);
