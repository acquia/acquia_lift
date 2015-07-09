(function ($) {

  /**
   * Add the 'chosen' behavior to multiselect elements.
   */
  Drupal.behaviors.acquia_lift_admin = {
    attach: function (context, settings) {
      $('.acquia-lift-chosen-select', context).each(function() {
        var chosenWidth = '940px';
        if ($(this).hasClass('acquia-lift-chosen-select-half')) {
          chosenWidth = '470px';
        } else if ($(this).hasClass('acquia-lift-chosen-select-third')) {
          chosenWidth = '470px';
        }
        var options = {
          width: chosenWidth
        };
        $(this).chosen(options);
      });

      // Listener for context selection.
      $contextSelect = $('.acquia-lift-report-section .acquia-lift-report-context-select', context);
      if ($contextSelect.length > 0) {
        $contextSelect.once().chosen().change(function(e) {
          var selected = $(this).val(),
            num = selected ? selected.length : 0,
            selectors = [],
            $report = $(this).parents('.acquia-lift-report-section').find('table tbody');

          // If nothing is selected, then show all.
          if (num == 0) {
            $report.find('tr').show();
            return;
          }
          for (var i=0; i<num; i++) {
            selectors.push('tr[data-acquia-lift-feature="' + selected[i] + '"]');
          }
          var selector = selectors.join(', ');
          $report.find('tr').not(selector).hide();
          $report.find(selector).show();
        });
      }
    }
  };

  /**
   * Campaign edit form behaviors.
   */
  Drupal.behaviors.acquiaLiftCampaignEdit = {
    attach: function (context, settings) {
      // Add a handler to the "Reset data" button to warn the user before resetting the data.
      $('.personalize-wizard-process-bar-actions .acquia-lift-reset').once('acquia-lift-reset').click(function(e) {
        if (confirm(Drupal.t('This action will delete all existing data for this personalization and cannot be undone. Are you sure you want to continue?'))) {
          return true;
        } else {
          e.stopImmediatePropagation();
          return false;
        }
      });
    }
  };

  /**
   * Adjusts the display of high-low components.
   */
  Drupal.behaviors.acquia_lift_high_low = {
    attach: function(context, settings) {
      // Determine the min and max bounds for the component.
      var maxBound = 0;
      var minBound = NaN; // Don't set this to 0 or it will always be the min.
      $('.acquia-lift-hilo-bounds').each(function() {
        var lo = parseFloat($(this).attr('data-acquia-lift-low'));
        if (isNaN(minBound)) {
          minBound = lo;
        } else {
          minBound = Math.min(minBound, lo);
        }
        maxBound = Math.max(maxBound, parseFloat($(this).attr('data-acquia-lift-high')));
      });

      // Now adjust the bounds display for each high-low component.
      $('.acquia-lift-hilo-estimate', context).each(function() {
        var $bounds = $('.acquia-lift-hilo-bounds', this);
        var high = parseFloat($bounds.attr('data-acquia-lift-high'));
        var low = parseFloat($bounds.attr('data-acquia-lift-low'));
        var estimate = parseFloat($('.acquia-lift-badge', this).text());
        if (isNaN(high) || isNaN(low) || isNaN(estimate) || (high === 0 && low === 0)) {
          $bounds.css('height', 0);
          return;
        }
        var scale = $(this).width() / (maxBound - minBound);
        var scaleLow = (low - minBound) * scale;
        var scaleHigh = (high - minBound) * scale;
        var width = Math.max((scaleHigh - scaleLow), 20);
        $bounds.css('width', width + 'px');
        $bounds.css('left', scaleLow + 'px');
      });
    }
  };

  /**
   * Remove any duplicated message display areas.
   */
  Drupal.behaviors.acquiaLiftDSM = {
    attach: function(context, settings) {
      $newMessages = $('div.messages', context);
      if ($newMessages.length === 0) {
        return;
      }
      $priorMessages = $('div.messages').not($newMessages).hide();
    }
  };

  /**
   * Campaign list display.
   */
  Drupal.behaviors.acquiaLiftCampaignList = {
    attach: function (context, settings) {
      $('#personalize-personalizations-list').once('acquia-lift-campaign-list', function () {
        $('.acquia-lift-personalize-list-campaign .acquia-lift-campaign-title', this).on('click', function () {
          var $audienceRows = $(this).parent().nextUntil('.acquia-lift-personalize-list-campaign');
          // Cannot perform slide jquery animations on table rows so we have
          // to wrap the cell contents in something that we can animate.
          if ($audienceRows.hasClass('element-hidden')) {
            $audienceRows.find('td')
              .wrapInner('<div style="display: block; display: none;" />')
              .parent()
              .removeClass('element-hidden')
              .find('td > div')
              .slideDown(400, function() {
                $(this).parent().parent().removeClass('element-hidden');
                $(this).contents().unwrap();
                console.log('complete');
              });
            $(this).addClass('acquia-lift-is-open');
          } else {
            $audienceRows.find('td')
              .wrapInner('<div style="display: block;">')
              .parent()
              .find('td > div')
              .slideUp(400, function () {
                $(this).parent().parent().addClass('element-hidden');
                $(this).contents().unwrap();
              });
            $(this).removeClass('acquia-lift-is-open');
          }
        });
      });
    }
  }

})(Drupal.jQuery);
