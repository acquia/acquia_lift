(function ($) {
  /**
   * Allows one control to adjust the percentage while another
   * displays the amount left.
   */
  Drupal.behaviors.acquiaLiftAgentPercentageControls = {
    attach: function(context, settings) {
      $('.acquia-lift-agent-percentage-adjust', context).once(function() {
        var restId = $(this).attr('data-acquia-lift-percentage-rest');
        var $rest = $('#' + restId, context);

        // Limit input to numbers and decimal sign.
        $(this).keyup(function(e) {
          var code = e.keyCode || e.which;
          // Don't apply behavior to tab, enter, or arrow keys.
          if (code == 9 || code == 13 || (code >= 37 && code <= 40)) { return; }

          this.value = this.value.replace(/[^0-9\.]/g, '');
        });

        // Update the "rest" percentage box when percentage changes.
        $(this).change(function(e) {
          var newPercent = $(this).val();
          var newRest = 100 - $(this).val();
          $rest.val(newRest);
        });
      })
    }
  };

  Drupal.behaviors.acquiaLiftShowHide = {
    attach: function(context, settings) {
      $('.acquia-lift-more-information').once(function() {
        var $parent = $(this);
        if (this.tagName.toLowerCase() == 'fieldset') {
          $parent = $('.fieldset-wrapper', this);
        }
        $parent.prepend('<a class="acquia-lift-toggle-text" href="#">' + Drupal.t('Info') + '</span>');

        $('.acquia-lift-toggle-text', this).click(function(e) {
          if ($(this).hasClass('collapsed')) {
            // Open and show additional details.
            $('.description', $parent).slideDown();
            $(this).removeClass('collapsed');
            $(this).text(Drupal.t('Hide info'));
          } else {
            $(this).addClass('collapsed');
            $('.description', $parent).slideUp();
            $(this).text(Drupal.t('Info'));
          }
          return false;
        });

        // Hide all the descriptions by default
        $('.acquia-lift-toggle-text', this).addClass('collapsed');
        $('.description', $parent).hide();
      });
    }
  }
})(jQuery);
