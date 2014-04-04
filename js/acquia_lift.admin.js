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


  /**
   * Campaign edit form behaviors.
   */
  Drupal.behaviors.acquiaLiftCampaignEdit = {
    attach: function (context, settings) {
      // Add a handler to the "Reset data" button to warn the user before resetting the data.
      $('#personalize-acquia-lift-reset input[type="submit"]').once('acquia-lift-reset').each(function() {
        // Overwrite beforeSubmit of the ajax event.
        Drupal.ajax[this.id].options.beforeSubmit = function(form_values, $element, options) {
          if (confirm(Drupal.t('This action will delete all existing data for this campaign and cannot be undone. Are you sure you want to continue?'))) {
            return true;
          } else {
            return false;
          }
        }
      });
    }
  }
})(jQuery);
