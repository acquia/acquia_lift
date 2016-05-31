(function ($, Drupal, settings) {

  "use strict";

  var initialized = false;

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
    if (initialized) {
      return;
    }
    initialized = true;

    // Load and run capture scripts.
    if (window.attachEvent) {
      window.attachEvent('onload', Drupal.acquia_lift.asyncLoad);
      return;
    }

    window.addEventListener('load', Drupal.acquia_lift.asyncLoad, false);
  };

  /**
   * Generate tracking ID.
   */
  Drupal.acquia_lift.generateTrackingId = function () {
    var d = new Date().getTime();
    settings.pageContext.trackingId = 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replace(/[xy]/g, function(c) {
      var r = (d + Math.random()*16)%16 | 0;
      d = Math.floor(d/16);
      return (c=='x' ? r : (r&0x7|0x8)).toString(16);
    });
  };

  /**
   * Load and run capture scripts.
   */
  Drupal.acquia_lift.asyncLoad = function () {
    var script = document.createElement('script'),
      firstScript = document.getElementsByTagName('script')[0];
    script.type = 'text/javascript';
    script.async = true;
    script.src = settings.credential.js_path;
    firstScript.parentNode.insertBefore(script, firstScript);
  };

})(jQuery, Drupal, drupalSettings.acquia_lift);
