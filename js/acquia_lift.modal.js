/**
 * Functionality related specifically to the modal campaign management
 * procedures.
 */
(function ($, Drupal) {
  Drupal.behaviors.acquiaLiftCampaignTypeModal = {
    attach: function (context, settings) {
      // Make the whole campaign type div clickable.
      $('div.ctools-modal-content .modal-content .acquia-lift-campaign-type', context).once(function() {
        $(this).on('click', function(e) {
          var $link = $(this).find('a.acquia-lift-campaign-select');
          // If it's a modal process, then we only need to trigger the handlers.
          // If it's a regular link, then we also need to set the new location.
          if ($link.hasClass('ctools-use-modal')) {
            $link.trigger('click');
          } else {
            window.location = $link.attr('href');
          }
        })
      });
    }
  }
}(jQuery, Drupal));
