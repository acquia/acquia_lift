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
          if ($link.attr('href') == '/admin/structure/visitor_actions') {
            $('#acquiaLiftVisitorActionsConnector').find('a').trigger('click');
            Drupal.CTools.Modal.dismiss();
            e.preventDefault();
            e.stopImmediatePropagation();
          } else if ($link.hasClass('ctools-use-modal')) {
            // It needs to be the link that is triggered if we want CTools to
            // take over.
            if (!$(e.currentTarget).is('a')) {
              $link.trigger('click');
            }
            // Let the event bubble on to the next handler.
            return;
          } else {
            // If it's a regular link, then we also need to set the new location.
            window.location = $link.attr('href');
          }
        })
      });
      hidePageVisitorActionsButton();
      // When visitor actions is activated, remove the page actions button
      // because the user is selecting this through the modal process.
      $('body').once('acquiaLiftVisitorActionsHidePage', function() {
        $(document).bind('visitorActionsUIEditMode', function (event, isActive) {
          if (isActive) {
            hidePageVisitorActionsButton();
          }
        });
      });

      // The visitor actions ui application expects there to always be a
      // trigger link on the page, but with the modal process the trigger would
      // disappear when the modal closes.  We create a hidden trigger link
      // to handle the edit mode toggle.
      var $connector = $('#acquiaLiftVisitorActionsConnector');
      if ($connector.length == 0) {
        $('body').append('<div id="acquiaLiftVisitorActionsConnector"><a href="/admin/structure/visitor_actions/add" class="element-hidden">' + Drupal.t('Add goals') + '</a></div>');
        // Allow visitor actions UI to process the link.
        Drupal.attachBehaviors($('#acquiaLiftVisitorActionsConnector'));
        $(document).on('acquiaLiftVisitorActionsConnectorToggle', function(e) {
          $('#acquiaLiftVisitorActionsConnector').find('a').trigger('click');
        });
      }


      // Provide method to hide full selector in variation type details form
      // until the user selects to edit.
      // Note that the form is sent as the new context so we can't just check
      // within the context.
      var $variationTypeForm = $('#acquia-lift-page-variation-details-form').not('.acquia-lift-processed');
      if ($variationTypeForm.length > 0) {
        var editLink = '<a class="acquia-lift-selector-edit">' + Drupal.t('Edit selector') + '</a>';
        var $selectorInput = $variationTypeForm.find('input[name="selector"]');
        var $selector =  $selectorInput.closest('div');
        $variationTypeForm.parent().find('h2').append(editLink);
        $variationTypeForm.parent().find('.acquia-lift-selector-edit').on('click', function(e) {
          var newText = $(this).text() == Drupal.t('Edit selector') ? Drupal.t('Hide selector') : Drupal.t('Edit selector');
          $selector.slideToggle();
          $(this).text(newText);
        });
        $selector.hide();

        // Validate the jQuery selector if changed.
        $selectorInput.on('change', function(e) {
          try {
            var test = $selectorInput.val();
            var verify = $(test);
          } catch (error) {
            $selectorInput.addClass('error');
            if ($('.acquia-lift-js-message', $variationTypeForm).length == 0) {
              var errorMessage = '<div class="acquia-lift-js-message"><div class="messages error"><h2 class="element-invisible">' + Drupal.t('Error message') + '</h2>';
              errorMessage += Drupal.t('Selector field must contain a valid jQuery selector.');
              errorMessage += '</div></div>';
              $variationTypeForm.prepend(errorMessage);
            }
            $('#edit-variation-type-submit-form', $variationTypeForm).attr('disabled', 'disabled').addClass('form-button-disabled');
            return;
          }
          // If we are still here then validation passed and any previous
          // error messages should be removed.
          $selectorInput.removeClass('error');
          $('.acquia-lift-js-message', $variationTypeForm).remove();
          $('#edit-variation-type-submit-form', $variationTypeForm).removeAttr('disabled').removeClass('form-button-disabled');
        });

        $variationTypeForm.addClass('acquia-lift-processed');
      }

      // Populate the pages input with the current page.
      // The form is sent as the context so we can't check within it.
      var $pageGoalForm = $('#acquia-lift-create-goal-type-form').not('acquia-lift-processed');
      if ($pageGoalForm.length > 0) {
        $pageGoalForm.find('input[name="pages"]').val(Drupal.settings.visitor_actions.currentPath);
        $pageGoalForm.addClass('acquia-lift-processed');
      }
    }
  }

  function hidePageVisitorActionsButton() {
    $('#visitor-actions-ui-actionable-elements-without-identifiers').hide();
  }

}(jQuery, Drupal));
