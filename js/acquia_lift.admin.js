(function ($) {

  /**
   * Add the 'chosen' behavior to multiselect elements.
   */
  Drupal.behaviors.acquia_lift_admin = {
    attach: function (context, settings) {
      $('.acquia-lift-chosen-select', context).each(function() {
        var options = {
          width: '940px'
        };
        $(this).chosen(options);
      });
    }
  };

})(jQuery);
