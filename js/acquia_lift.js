var _tcaq = _tcaq || [];
var _tcwq = _tcwq || [];

(function ($, Drupal, drupalSettings) {

  "use strict";

  Drupal.acquia_lift = {};

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
  drupalSettings.acquia_lift.trackingId = generateTrackingId();

  Drupal.behaviors.acquia_lift = {
    attach: function () {
      Drupal.acquia_lift.initialize();
    }
  };

  /**
   * Initialize.
   */
  Drupal.acquia_lift.initialize = function () {
    var credential = drupalSettings.acquia_lift.credential;
    _tcaq.push(['setAccount', credential.account_name, credential.customer_site]);

    (function() {
      function async_load() {
        var s = document.createElement('script');
        s.type = 'text/javascript';
        s.async = true;
        s.src = ('https:' == document.location.protocol ? 'https' : 'http') + '://' + credential.js_path;
        var x = document.getElementsByTagName('script')[0];
        x.parentNode.insertBefore(s, x);
      }
      if (window.attachEvent) {
        window.attachEvent('onload', async_load);
      }
      else {
        window.addEventListener('load', async_load, false);
      }
    })();
  };

})(jQuery, Drupal, drupalSettings);
