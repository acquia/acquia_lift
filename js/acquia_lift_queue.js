(function ($) {
  Drupal.behaviors.acquia_lift_queue = {
    attach: function(context, settings) {
      var queue_url;
      if (Drupal.settings.acquia_lift && Drupal.settings.acquia_lift.sync_queue) {
        queue_url = Drupal.settings.basePath + 'acquia_lift/queue';
        $.ajax({
          url: queue_url,
          type: "POST"
        });
      }
    }
  };
}(jQuery));