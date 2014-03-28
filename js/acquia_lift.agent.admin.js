(function ($) {
  /**
   * Allows one control to adjust the percentage while another
   * displays the amount left.
   */
  Drupal.behaviors.acquiaLiftAgentPercentageControls = {
    attach: function(context, settings) {
      $('div.acquia-lift-percentage-control', context).once(function() {
        var $rest = $('.acquia-lift-percentage-rest-display', this);
        var $input = $('input.acquia-lift-percentage-control', this);

        var updateRest = function() {
          var newPercent = $input.val();
          var newRest = 100 - newPercent;
          $rest.text(newRest + '%');
        }

        // Limit input to numbers and decimal sign.
        $('input.acquia-lift-percentage-control',this).keyup(function(e) {
          var code = e.keyCode || e.which;
          // Don't apply behavior to tab, enter, or arrow keys.
          if (code == 9 || code == 13 || (code >= 37 && code <= 40)) { return; }

          this.value = this.value.replace(/[^0-9\.]/g, '');
        });

        // Update the "rest" percentage box when percentage changes.
        $('input.acquia-lift-percentage-control', this).change(updateRest);

        // Set initial "rest" percentage value.
        updateRest();
      })
    }
  };

  /**
   * Adds ability to collapse description for form elements.
   */
  Drupal.behaviors.acquiaLiftShowHide = {
    attach: function(context, settings) {
      var showText = Drupal.t('Info');
      var hideText = Drupal.t('Hide info');
      $('.acquia-lift-collapsible', context).once(function() {
        var $container = $(this);
        $container.prepend('<a class="acquia-lift-toggle-text" href="#"></a>');

        $('.acquia-lift-toggle-text', this).click(function(e) {
          if ($container.hasClass('collapsed')) {
            // Open and show additional details.
            $('.description', $container).slideDown();
            $container.removeClass('collapsed');
            $(this).text(Drupal.t('Hide info'));
          } else {
            $container.addClass('collapsed');
            $('.description', $container).slideUp();
            $(this).text(Drupal.t('Info'));
          }
          return false;
        });

        // Hide descriptions if collapsed by default.
        if ($container.hasClass('collapsed')) {
          $('.description', $container).hide();
          $('.acquia-lift-toggle-text', this).text(showText);
        } else {
          $('.acquia-lift-toggle-text', this).text(hideText);
        }
      });
    }
  }
})(jQuery);
