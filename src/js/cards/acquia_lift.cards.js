/**
 * @file acquia_lift.cards.js  Initialization and Acquia Lift specific handling
 * of cards.
 */

(function ($, Drupal) {
  'use strict';

  Drupal.behaviors.acquiaLiftCards = {
    attach: function (context, settings) {
      $('.el-card').once('acquia-lift-card', function() {
        $(this).card();
      });
    }
  }

})(jQuery, Drupal);
