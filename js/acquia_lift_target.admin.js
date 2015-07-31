/**
 * @file acquia_lift_target.admin.js
 *
 * Functionality for the administration of Acquia Lift Target campaigns.
 */

(function ($, Drupal) {

  /**
   * Enable full section to be clickable when using radio lists as button lists
   */
  Drupal.behaviors.acquiaLiftRadioButtonList = {
    attach: function(context, settings) {
      $('.acquia-lift-radio-list-item', context).once().on('click', function(e) {
        var $radio = $('input[type="radio"]', this);
        $radio.attr('checked', true);
        $radio.trigger('change');
      });
    }
  };

  /**
   * Section help
   */
  Drupal.behaviors.acquiaLiftCampaignHelp = {
    attach: function(context, settings) {
      $('#acquia-lift-campaign-wizard-section-help', context).once(function() {
        function toggleHelp(e) {
          if ($('#acquia-lift-campaign-wizard-section-help', context).hasClass('is-expanded')) {
            // Collapse help.
            var $container = $('#acquia-lift-campaign-wizard-section-help');
            $container.removeClass('is-expanded');
            $container.find('.acquia-lift-section-help-trigger').attr('title', Drupal.t('Show help'));
            $('#acquia-lift-campaign-wizard-section-help .acquia-lift-section-help-expanded').slideUp(400, function() {
              $('#acquia-lift-campaign-wizard-section-help').addClass('is-collapsed');
            });
          } else {
            // Expand help.
            var $container = $('#acquia-lift-campaign-wizard-section-help');
            $container.addClass('is-expanded');
            $container.find('.acquia-lift-section-help-trigger').attr('title', Drupal.t('Hide help'));
            $('#acquia-lift-campaign-wizard-section-help .acquia-lift-section-help-expanded').slideDown(400, function() {
              $('#acquia-lift-campaign-wizard-section-help').removeClass('is-collapsed');
            });
          }
          return false;
        }
        $('.acquia-lift-section-help-trigger', this).on('click', toggleHelp);
        $('.acquia-lift-section-help-collapse', this).on('mousedown', toggleHelp);
      });
    }
  };

  /**
   * Targeting behaviors
   */
  Drupal.behaviors.acquiaLiftTargetAdmin = {
    attach: function (context, settings) {

      // Fix the variations list to a position on the page.
      $('#acquia-lift-targeting-variations').once(function() {
        var $sidebar = $(this);
        var $window = $(window);
        var offset = $sidebar.offset();
        var $body = $('body');
        var paddingTop = parseInt($sidebar.css('margin-top')) + parseInt($sidebar.css('padding-top'));
        var scrollPaddingTop = paddingTop + parseInt($body.css('margin-top')) + parseInt($body.css('padding-top'));
        var $columns = $('.personalize-wizard-column');

        $window.scroll(function() {
          // Don't move if the columns have stacked vertically in cases of
          // limited content area width.
          if ($columns.first().offset().left == $columns.last().offset().left) {
            return;
          }
          if ($window.scrollTop() > offset.top) {
            $sidebar.stop().animate({
              marginTop: $window.scrollTop() - offset.top + scrollPaddingTop
            }, 'fast');
          } else {
            $sidebar.stop().animate({
              marginTop: paddingTop
            }, 'fast');
          }
        });
      });

      // Get a mapping of audiences that allow the selection of a winner
      // and the combination of options that are assigned upon load.
      var winnerSelections = {};
      $('#acquia-lift-targeting-audiences .acquia-lift-complete-audience').once(function() {
        var $card = $(this).closest('.el-card');
        var audience_id = $(this).attr('data-acquia-lift-audience-id');
        var assignments = $('.acquia-lift-targeting-assignment-order', $card).val();
        winnerSelections[audience_id] = assignments.split(',').sort();
      });

      // Attach the drag 'n' drop behavior to audiences in order to determine
      // audience weight.
      $('#acquia-lift-targeting-audiences .el-card.is-sortable').once(function () {
        $(this).parent().addClass('acquia-lift-sortable');
        // Hide the weight form item as it will be dynamically determined.
        $('.acquia-lift-sortable-weight', this).parent().hide();
      });
      // Make the audiences sortable
      $('#acquia-lift-targeting-audiences').sortable({
        items: ".acquia-lift-sortable",
        placeholder: 'acquia-lift-sortable-placeholder',
        forcePlaceholderSize: true,
        opacity: 0.7,
        tolerance: 'pointer',
        handle: '.el-card__dragger',
        appendTo: document.body,
        helper: 'clone',
        update: function (event, ui) {
          // Update the weight values for each sortable element after
          // the positions have been updated.
          var weight = 10;
          $('#acquia-lift-targeting-audiences .el-card.is-sortable').each(function() {
            $('.acquia-lift-sortable-weight', this).val(weight);
            weight += 10;
          });
        }
      });

      // Allow removal of an option from the draggable list.
      function handleDraggableItemRemove(event) {
        var $li = $(this).closest('li');
        var $ul = $li.parent();
        var optionId = $li.data('acquia-lift-option-id');
        var $select = $(this).closest('.form-item').children('select.acquia-lift-targeting-assignment');
        var selectedItems = $select.val();
        var $orderInput = $(this).parents('.el-card__content').find('input.acquia-lift-targeting-assignment-order');
        // Remove it from the selected items.
        selectedItems.splice(selectedItems.indexOf(optionId), 1);
        $select.val(selectedItems);
        $orderInput.val(selectedItems.join(','));
        // Remove the list item.
        $li.remove();
        checkTargetingPlaceholder($ul);
        indicateControlVariation($ul);
        indicateTest($ul);
        showAdvancedTestSettings();
        indicatePickWinner($ul);
      }

      /**
       * Helper function to determine if a dragged item is already in
       * a particular list.
       *
       * @param dragItem
       *   The ui item that is being dragged.
       * @param list
       *   The list that it is being dragged to.
       * @returns {boolean}
       *   True if already in list, false otherwise.
       */
      function isDragItemInList(dragItem, list) {
        var dropId = dragItem.data('acquia-lift-option-id');
        var selected = $(list).parent().children('select.acquia-lift-targeting-assignment').val() || [];
        return selected.indexOf(dropId) >= 0;
      }

      /**
       * Adjusts the location of the targeting placeholder.
       */
      function checkTargetingPlaceholder($ul) {
        // If all of the items have been removed then add the placeholder.
        if ($ul.find('li.acquia-lift-targeting-empty').length == 0) {
          $ul.append(Drupal.theme('acquiaLiftTargetingEmptyAudience'));
        } else {
          // Move it to the end of the elements.
          $('li.acquia-lift-targeting-empty', $ul).appendTo($ul);
        }
        var numChildren = $ul.find('li.acquia-lift-targeting-draggable').length;
        $ul.parents('.el-card__content').find('.acquia-lift-test-message').toggle(numChildren > 1);
      }

      /**
       * Relabel the list to indicate the control.
       */
      function indicateControlVariation($ul) {
        if (!$ul.hasClass('acquia-lift-draggable-connect')) {
          return;
        }
        var $variationItems = $ul.find('li.acquia-lift-targeting-draggable');
        var numberVariations = $variationItems.length;
        for (var i = 0; i < numberVariations; i++) {
          var $current = $variationItems.eq(i);
          // remove any current indicator.
          $('.acquia-lift-control-indicator', $current).remove();
          if (i == 0 && numberVariations > 1) {
            $current.append(Drupal.theme('acquiaLiftControlIndicator', 'acquia-lift-control-indicator'));
          }
        }
      }

      /**
       * Indicate if the audience contains a test.
       */
      function indicateTest($ul) {
        var $variationItems = $ul.find('li.acquia-lift-targeting-draggable');
        var numberVariations = $variationItems.length;
        $ul.closest('.el-card').find('.el-card__flag').toggleClass('is-hidden', numberVariations <= 1);
      }

      /**
       * Show/hide test settings based on whether there is currently
       * a test configured.
       */
      function showAdvancedTestSettings() {
        var showSettings = false;
        $('#acquia-lift-targeting-audiences .acquia-lift-targeting-assignment').each(function() {
          var selected = $(this).val();
          if (selected && selected.length > 1) {
            showSettings = true;
          }
        });
        $('.acquia-lift-test-options').toggleClass('element-hidden', !showSettings);
      }


      /**
       * Determine if the audience should show it's button to
       * complete the test and pick a winner.
       */
      function indicatePickWinner($ul) {
        var $card = $ul.closest('.el-card');
        var $button = $('.acquia-lift-complete-audience', $card);
        if ($button.length == 0) {
          // This audience never had the option to pick a winner.
          return;
        }
        var currentSelections = $('.acquia-lift-targeting-assignment-order', $card).val();
        var sorted = currentSelections.split(',').sort();
        var audience_id = $button.attr('data-acquia-lift-audience-id');
        var showWinnerSelection = false;
        var implemented = winnerSelections.hasOwnProperty(audience_id) ? winnerSelections[audience_id] : [];
        if (implemented.length == sorted.length) {
          showWinnerSelection = true;
          for (var i=0; i < implemented.length; i++) {
            if (implemented[i] !== sorted[i]) {
              showWinnerSelection = false;
              break;
            }
          }
        }
        // Can't just use element-hidden because some themes don't set the
        // buttons properly.
        $button.toggle(showWinnerSelection);
      }

      // Add drag and drop behavior to assign variations to audiences.
      $('.acquia-lift-targeting-assignment').once(function() {
        var cardDisabled = $(this).closest('.el-card').attr('data-card-enabled') === 'false';
        var $wrapperDiv = $(this).parent();
        var selectId = $(this).attr('id');
        var selectedOptions = [];
        var assignmentOrder;
        var allowMove = $(this).data('acquia-lift-targeting-allow-move');
        var allowRemove = !cardDisabled && $(this).data('acquia-lift-targeting-allow-remove');
        var variationsListHtml;
        var variationsListClasses = ['acquia-lift-draggable-variations'];
        if (allowMove) {
          variationsListClasses.push('acquia-lift-draggable-connect');
        }
        variationsListHtml = '<ul class="' + variationsListClasses.join(' ') + '">';

        // Hide the label for the select element.
        $('label[for="' + selectId + '"]').hide();
        // Convert each selected option in an audience assignment select
        // into a draggable container.
        if (allowMove) {
          assignmentOrder = $wrapperDiv.parents('.el-card__content').find('input.acquia-lift-targeting-assignment-order').val();
          if (assignmentOrder.length > 0) {
            selectedOptions = assignmentOrder.split(',');
          }
          for (var i = 0; i < selectedOptions.length; i++) {
            var optionId = selectedOptions[i];
            var optionText = $('option[value="' + optionId + '"]', this).text();
            variationsListHtml += Drupal.theme('acquiaLiftTargetingDraggableItem', optionId, optionText, allowRemove);
          }
        } else {
          // Just get all the select options in the variations list to show.
          $('option:selected', this).each(function() {
            variationsListHtml += Drupal.theme('acquiaLiftTargetingDraggableItem', $(this).val(), $(this).text(), allowRemove);
          });
        }
        variationsListHtml += '</ul>';
        $wrapperDiv.append(variationsListHtml);

        $('.acquia-lift-draggable-variations', $wrapperDiv).sortable({
          items: '> .acquia-lift-targeting-draggable',
          placeholder: 'element-hidden',
          opacity: 0.7,
          tolerance: 'pointer',
          connectWith: '.acquia-lift-draggable-variations.acquia-lift-draggable-connect',
          cancel: '.acquia-lift-targeting-droppable',
          start: function() {
            $('.acquia-lift-targeting-empty').addClass('is-active');
          },
          beforeStop: function() {
            $('.acquia-lift-targeting-empty').removeClass('is-active');
          },
          receive: function (event, ui) {
            // Validate that the item can be dropped on this list.
            if (isDragItemInList(ui.item, this)) {
              ui.sender.sortable('cancel');
              indicateControlVariation(ui.sender);
              indicateTest(ui.sender);
              showAdvancedTestSettings();
              indicatePickWinner(ui.sender);
              return;
            }
            if (ui.sender.data().hasOwnProperty('acquialiftcopied')) {
              // Make the new item removable.
              ui.item.prepend(Drupal.theme('acquiaLiftTargetingItemRemove'));
              $('.acquia-lift-targeting-remove', ui.item).on('click', handleDraggableItemRemove);

              // This is a copy from the variation options list.
              ui.sender.data('acquialiftcopied', true);
            }
          },
          update: function (event, ui) {
            // This will fire on the origin list too (the variations options)
            // but there is no need to do anything with them.
            if (!$(this).hasClass('acquia-lift-draggable-connect')) {
              return;
            }
            // Update the selected variations in the underlying select to
            // match the list contents.
            var $this = $(this);
            var $select = $this.parent().children('select.acquia-lift-targeting-assignment');
            var selectedOptions = [];
            var $orderInput = $this.parents('.el-card__content').find('input.acquia-lift-targeting-assignment-order');
            $(this).children('li.acquia-lift-targeting-draggable').each(function () {
              selectedOptions.push($(this).data('acquia-lift-option-id'));
            });
            $select.val(selectedOptions);
            $orderInput.val(selectedOptions.join(','));
            checkTargetingPlaceholder($(this));
            indicateControlVariation($(this));
            indicateTest($(this));
            checkTargetingPlaceholder($this);
            indicateControlVariation($this);
            indicateTest($this);
            showAdvancedTestSettings();
            indicatePickWinner($this);

            // Make sure the droppable area is last in the list.
            $this.append($('.acquia-lift-targeting-droppable', this));
          }
        });
        if (allowMove) {
          // Indicate the control option for any initial tests.
          indicateControlVariation($('ul.acquia-lift-draggable-variations', $wrapperDiv));
          // Show/hide test indicator.
          indicateTest($('ul.acquia-lift-draggable-variations', $wrapperDiv));
          // Add a placeholder list item.
          checkTargetingPlaceholder($('.acquia-lift-draggable-variations', $wrapperDiv));
        } else {
          // Special handling for the variations options list to enable a copy
          // to be added.
          $('.acquia-lift-draggable-variations', $wrapperDiv).sortable('option', 'helper', function(e, li) {
            this.copyHelper = li.clone().insertAfter(li);
            $(this).data('acquialiftcopied', false);
            return li.clone();
          });
          $('.acquia-lift-draggable-variations', $wrapperDiv).sortable('option', 'stop', function () {
            var copied = $(this).data('acquialiftcopied');
            if (!copied) {
              this.copyHelper.remove();
            }
            this.copyHelper = null;
          });
        }

        // Make options removable if indicated.
        $('.acquia-lift-targeting-remove', $wrapperDiv).on('click', handleDraggableItemRemove);

        // If the card is not editable, then all of this should be disabled.
        if (cardDisabled) {
          $('.acquia-lift-draggable-variations', $wrapperDiv).sortable('disable');
        }

        // Hide the actual select input.
        $(this).hide();
      });

      // Hide the assignment order fields.
      $('.acquia-lift-targeting-assignment-order').closest('.form-item').addClass('element-hidden');
      // Check for whether advanced test settings should be shown by default.
      showAdvancedTestSettings();
    }
  }

  /**
   * Theme function to generate a draggable list item for a variation (display
   * option).
   *
   * @param id
   *   The variation id
   * @param label
   *   The variation label to display.
   * @param allowRemove
   *   True if the draggable item can be removed from its current container.
   *   False if it cannot.
   */
  Drupal.theme.prototype.acquiaLiftTargetingDraggableItem = function(id, label, allowRemove) {
    var html = '';
    html += '<li data-acquia-lift-option-id="' + id + '" data-acquia-lift-option-label="' + encodeURI(label) + '" class="acquia-lift-targeting-draggable">';
    if (allowRemove) {
      html += Drupal.theme('acquiaLiftTargetingItemRemove');
    }
    html += label;
    html += '</li>';
    return html;
  };

  Drupal.theme.prototype.acquiaLiftTargetingItemRemove = function() {
    return '<span class="acquia-lift-targeting-remove">' + Drupal.t('Remove') + '</span>';
  };

  Drupal.theme.prototype.acquiaLiftTargetingEmptyAudience = function() {
    var html = '';
    html += '<li class="acquia-lift-targeting-empty acquia-lift-connected-sortable-placeholder">';
    html += Drupal.t('Drag a variation here to show to this audience.');
    html += '</li>';
    return html;
  };

  Drupal.theme.prototype.acquiaLiftControlIndicator = function(controlClass) {
    var html = '';
    html = '<span class="' + controlClass + '">';
    html += '(' + Drupal.t('Control') + ')';
    html += '</span>';
    return html;
  }

})(Drupal.jQuery, Drupal);
