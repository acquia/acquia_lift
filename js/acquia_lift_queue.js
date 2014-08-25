(function ($) {

  /**
   * Actually trigger the Acquia Lift queue processing and let
   * Drupal handle any command results.
   */
  var processQueue = function() {
    var queue_url = Drupal.settings.basePath + 'acquia_lift/queue';
    $.ajax({
      url: queue_url,
      type: "POST",
      success: function (response, status, jqXHR) {
        var processed = false;
        // Process any Drupal commands returned.
        for (var i in response) {
          if (response.hasOwnProperty(i) && response[i]['command'] && Drupal.ajax.prototype.commands[response[i]['command']]) {
            Drupal.ajax.prototype.commands[response[i]['command']](Drupal.ajax.prototype, response[i], status);
            processed = true;
          }
        }
        if (processed) {
          Drupal.attachBehaviors();
        }
      }
    });
  }

  /**
   * Run the queue processing if the settings indicate to do so.
   */
  Drupal.behaviors.acquia_lift_queue = {
    attach: function(context, settings) {
      if (Drupal.settings.acquia_lift && Drupal.settings.acquia_lift.sync_queue) {
        processQueue();
      }
    }
  };

  /**
   * A Drupal AJAX command to trigger the queue processing.
   */
  Drupal.ajax.prototype.commands.acquia_lift_process_queue = function (ajax, response, status) {
    processQueue();
  };
}(jQuery));
