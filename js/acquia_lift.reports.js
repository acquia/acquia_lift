/**
 * Functionality related specifically to the modal campaign management
 * procedures.
 */
(function ($, Drupal) {
  "use strict";

  Drupal.behaviors.acquiaLiftReports = {
    attach: function (context, settings) {
      // Generate a lift graph for the A/B statistics table.
      $('table[data-lift-statistics]').not('acquia-lift-processed').each(function() {
        $(this).liftGraph();
        $(this).addClass('acquia-lift-processed');
      })
    }
  }
}(jQuery, Drupal));
