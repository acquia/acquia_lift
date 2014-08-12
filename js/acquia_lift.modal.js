/**
 * Functionality related specifically to the modal campaign management
 * procedures.
 */
(function ($, Drupal) {
  "use strict";

  Drupal.behaviors.acquiaLiftTypeModal = {
    attach: function (context, settings) {
      // Make the whole campaign type div clickable.
      $('div.ctools-modal-content .modal-content .acquia-lift-type', context).once(function() {
        $(this).on('click', function(e) {
          var $link = $(this).find('a.acquia-lift-type-select');
          // If it's a modal process, then we only need to trigger the handlers.
          // If it's a regular link, then we also need to set the new location.
          if ($link.hasClass('ctools-use-modal')) {
            $link.trigger('click');
          } else {
            if ($link.attr('href') == '/visitor_actions_ui/edit-mode-toggle') {
              $.ajax({
                url: $link.attr('href')
              });
              Drupal.CTools.Modal.dismiss();
              e.preventDefault();
            } else {
              window.location = $link.attr('href');
            }
          }
        })
      });

      // Provide method to hide full selector in variation type details form
      // until the user selects to edit.
      // Note that the form is sent as the new context so we can't just check
      // within the context.
      var $variationTypeForm = $('#acquia-lift-page-variation-details').not('.acquia-lift-processed');
      if ($variationTypeForm.length > 0) {
        var editLink = '<a class="acquia-lift-selector-edit">' + Drupal.t('Edit selector') + '</a>';
        var $selector =  $variationTypeForm.find('input[name="selector"]').closest('div');
        $variationTypeForm.parent().find('h2').append(editLink);
        $variationTypeForm.parent().find('.acquia-lift-selector-edit').on('click', function(e) {
          $selector.slideDown(function() {
            $(this).find('input[name="selector"]').focus();
          });
          $(this).fadeOut();
        });
        $selector.hide();
        $variationTypeForm.addClass('acquia-lift-processed');
      }
    }
  }
}(jQuery, Drupal));
