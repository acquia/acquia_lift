(function ($) {

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
    var $messageBox = $('#acquia-lift-message-box');
    if ($messageBox.length == 0) {
      $messageBox = $('<div id="acquia-lift-message-box"><div class="close">X</div><p class="message"></p></div>');
      $('.region-page-top').append($messageBox);
      $messageBox.find('.close').on('click', function closeMessageBox() {
        $messageBox.animate({ top:"+=15px", opacity:0 }, "slow", function() {
          $(this).addClass('element-hidden')
        });
      });
    }
    else {
      $messageBox.removeClass('element-hidden');
    }

  }

  /**
   * Shows the requested message within a message box.
   *
   * @param message
   */
  function showMessageBox(message) {
    var $messageBox = createMessageBox();
    $messageBox.find('.message').html(message);
  }

  /**
   * A Drupal AJAX command to display a message box.
   */
  Drupal.ajax.prototype.commands.acquia_lift_message_box = function (ajax, response, status) {
    Drupal.ajax.prototype.commands.acquia_lift_message_box = null;
    showMessageBox(response.data);
  };
})(jQuery);
