(function ($, Drupal) {

  /**
   * JavaScript regarding the message box that is used for administrative
   * messages.
   */

  /**
   * Creates a message box if one is not present.
   *
   * @return
   *   The jQuery message box.
   */
  function createMessageBox() {
    var $messageBox = getMessageBox();
    if ($messageBox.length == 0) {
      $messageBox = $('<div id="acquia-lift-message-box"><div class="close">' + Drupal.t('Close') + '</div><p class="message"></p></div>');
      $('.region-page-top').append($messageBox);
      $messageBox.find('.close').on('click', closeMessageBox);
      // Don't close the message box if you click on it (other than close).
      $messageBox.on('click', function(e) {
        e.stopPropagation();
      });
    }
    return $messageBox;
  }

  /**
   * Close the message box.
   *
   * @param e
   *   (optional) The event that triggered the close.
   */
  function closeMessageBox(e) {
    $messageBox = getMessageBox();
    $messageBox.animate({ height:0, opacity:0 }, "slow", function() {
      $(this).addClass('element-hidden');
      // Take off the height/opacity styles - only used for animation.
      $(this).removeAttr('style');
    });
    if (e && e.stopPropagation) {
      e.stopPropagation();
    }
  }

  /**
   * Helper function to retrieve the message box from the DOM if it exists.
   *
   * @returns jQuery match for message box selector
   */
  function getMessageBox() {
    return $('#acquia-lift-message-box');
  }

  /**
   * Shows the requested message within a message box.
   *
   * @param message
   *   The message to show.
   * @param seconds
   *   The number of seconds to show for.  If value = 0, then the message
   *   is shown until the user clicks to close it.
   */
  function showMessageBox(message, seconds) {
    var $messageBox = createMessageBox();
    $messageBox.find('.message').html(message);
    // Measure the final height while the box is still hidden.
    var fullHeight = $messageBox.height();
    // Reset the properties to animate so that it starts hidden.
    $messageBox.css('height', '0px');
    $messageBox.css('opacity', '0');
    $messageBox.removeClass('element-hidden');
    // Animate the box height and opacity to draw attention.
    $messageBox.animate({height: fullHeight + 'px', opacity: 1}, 'slow');

    // Close the message box by clicking anywhere on the page.
    $(document).one('click', function(e) {
      closeMessageBox(e);
    });

    if (seconds > 0) {
      setTimeout(closeMessageBox, seconds*1000);
    }
  }

  /**
   * A Drupal AJAX command to display a message box.
   */
  Drupal.ajax.prototype.commands.acquia_lift_message_box = function (ajax, response, status) {
    var seconds = response.data.seconds || 0;
    var message = response.data.message || '';
    if (message.length >0) {
      showMessageBox(response.data.message, seconds);
    }
  };
})(jQuery, Drupal);
