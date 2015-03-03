/**
 * @file
 * acquia_lift.inputs.revealing.js: Provides JavaScript for updated input types.
 */

(function ($, Drupal) {
  "use strict";

  /**
   * Attach tooltips to elements with help data.
   */
  Drupal.behaviors.acquiaLiftInputsRevealing = {
    attach: function (context, settings) {
      $('.acquia-lift-revealing-input > input', context).once('acquia-lift-revealing-input').each(function () {
        // Add a link to edit the value.
        var $input = $(this);
        var $revealing = $input.parent();
        $input.wrap('<div class="acquia-lift-reveal clearfix">');
        $input.after('<a class="acquia-lift-revealer">' + Drupal.t('Edit') + '</a>');
        $input.after('<span class="acquia-lift-reveal-value">' + $input.val() + '</span>');

        function revealEdit() {
          $('.acquia-lift-reveal-value', $revealing).hide();
          $('.acquia-lift-revealer', $revealing).hide();
          $revealing.removeClass('acquia-lift-is-hidden');
          $input.show();
          $input.focus();
        }

        function hideEdit() {
          $('.acquia-lift-reveal-value', $revealing).text($input.val());
          $('.acquia-lift-reveal-value', $revealing).show();
          $('.acquia-lift-revealer', $revealing).show();
          $input.hide();
          $revealing.addClass('acquia-lift-is-hidden');
        }

        // Set the initial state.
        hideEdit();

        // Make revealer link toggle editing.
        $('.acquia-lift-revealer', $revealing).bind('click', revealEdit);
        $('.acquia-lift-reveal-value', $revealing).bind('click', revealEdit);

        // Switch back to hidden state when the input loses focus.
        $input.bind('blur', hideEdit);
      });
    }
  }
}(Drupal.jQuery, Drupal));
