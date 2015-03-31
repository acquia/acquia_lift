/**
 * @file acquia_lift_target.admin.js
 *
 * Functionality for the administration of Acquia Lift Target campaigns.
 */

(function ($, Drupal) {

  Drupal.behaviors.acquiaLiftTargetAdmin = {
    attach: function (context, settings) {

      // Attach the drag 'n' drop behavior to audiences in order to determine
      // audience weight.
      $('#acquia-lift-targeting-audiences .el-card.is-sortable').once(function () {
        $(this).parent().addClass('acquia-lift-sortable');
        // Hide the weight form item as it will be dynamically determined.
        $('.acquia-lift-sortable-weight', this).parent().hide();
      });
      // Make the audiences sortable
      $('#acquia-lift-targeting-audiences').sortable({
        items: "> .acquia-lift-sortable",
        placeholder: 'acquia-lift-sortable-placeholder',
        forcePlaceholderSize: true,
        opacity: 0.7,
        cursor: 'move',
        tolerance: 'pointer',
        handle: '.el-card__dragger',
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

      // Define the draggable configuration so that draggable items can be
      // initialized from multiple places.
      var draggableConfiguration = {
        containment: '#acquia-lift-targeting-audiences',
        scope: 'acquia-lift-targeting-variations',
        helper: 'clone',
        appendTo: 'body',
        cursor: 'move',
        cursorAt: {
          top: 0,
          left: 0
        },
        start: function (event, ui) {
          if ($(event.originalEvent.target).hasClass('acquia-lift-targeting-duplicate')) {
            // Remove the duplicate indicator.
            ui.helper.find('.acquia-lift-targeting-duplicate-icon').remove();
            // Remove the removal indicator.
            ui.helper.find('.acquia-lift-targeting-remove').remove();
            // Give feedback that this is a copy.
            ui.helper.text(ui.helper.data('acquia-lift-option-label') + ' - ' + Drupal.t('Copy'));
            // Add a class to be able to tell upon drop.
            ui.helper.addClass('acquia-lift-targeting-duplicate');
          } else {
            ui.helper.text(ui.helper.data('acquia-lift-option-label'));
          }
        }
      };

      // Allow removal of an option from the draggable list.
      function handleDraggableItemRemove(event) {
        var $li = $(this).closest('li');
        var optionId = $li.data('acquia-lift-option-id');
        var $select = $(this).closest('.form-item').children('select.acquia-lift-targeting-assignment');
        var selectedItems = $select.val();
        // Remove it from the selected items.
        selectedItems.splice(selectedItems.indexOf(optionId), 1);
        $select.val(selectedItems);
        // Remove the list item.
        $li.remove();
      }

      // Add drag and drop behavior to the variations for audience assignment.
      $('.acquia-lift-targeting-assignment').once(function() {
        var $wrapperDiv = $(this).parent();
        var selectId = $(this).attr('id');
        var allowDuplication = $(this).data('acquia-lift-targeting-allow-copy');
        var allowMove = $(this).data('acquia-lift-targeting-allow-move');
        var allowRemove = $(this).data('acquia-lift-targeting-allow-remove');
        var variationsListHtml = '<ul class="acquia-lift-draggable-variations">';
        // Hide the label for the select element.
        $('label[for="' + selectId + '"]').hide();
        // Convert each selected option in an audience assignment select
        // into a draggable container.
        $('option:selected', this).each(function() {
          variationsListHtml += Drupal.theme('acquiaLiftTargetingDraggableItem', $(this).val(), $(this).text(), allowDuplication, allowMove, allowRemove);
        });
        variationsListHtml += '</ul>';
        $wrapperDiv.append(variationsListHtml);

        // Make the options draggable.
        $('.acquia-lift-draggable-variations li', $wrapperDiv).draggable(draggableConfiguration);
        // Make options removable if indicated.
        $('.acquia-lift-targeting-remove', $wrapperDiv).on('click', handleDraggableItemRemove);

        // Convert each assignment select area into a droppable target.
        if ($(this).data('acquia-lift-targeting-droppable')) {
          $wrapperDiv.append(Drupal.theme('acquiaLiftTargetingDroppable'));
          $('.acquia-lift-targeting-droppable', $wrapperDiv).droppable({
            scope: 'acquia-lift-targeting-variations',
            activeClass: 'is-droppable',
            hoverClass: 'is-hovered',
            drop: function (event, ui) {
              var $dropSelect = $(this).parent().children('select.acquia-lift-targeting-assignment');
              var dropOptionId = ui.draggable.data('acquia-lift-option-id');
              var dropSelectedOptions = $dropSelect.val() || [];
              var $dragSelect = ui.draggable.closest('.form-item').children('select.acquia-lift-targeting-assignment');
              var dragSelectedOptions = $dragSelect.val() || [];
              var dropIsEveryoneElse = $dropSelect.hasClass('acquia-lift-targeting-everyone-else');
              var $dropList = $(this).parent().children('.acquia-lift-draggable-variations');
              var isCopy = ui.helper.hasClass('acquia-lift-targeting-duplicate');

              ui.helper.remove();

              if (dropSelectedOptions.indexOf(dropOptionId) >= 0) {
                // Option is already selected.
                if (dropIsEveryoneElse) {
                  // Allow the user to drag it here anyway to remove it from the
                  // variation.
                  ui.draggable.remove();
                }
                return;
              }
              // Select the new option in the underlying form.
              dropSelectedOptions.push(dropOptionId);
              $dropSelect.val(dropSelectedOptions);

              if (isCopy) {
                // Making a copy so this should remain selected and in the list.
                // In the new list it needs to inherit the actions based on that
                // audience container.
                var allowDuplication = $dropSelect.data('acquia-lift-targeting-allow-copy');
                var allowMove = $dropSelect.data('acquia-lift-targeting-allow-move');
                var allowRemove = $dropSelect.data('acquia-lift-targeting-allow-remove');
                var copyLi = Drupal.theme('acquiaLiftTargetingDraggableItem', dropOptionId, ui.draggable.data('acquia-lift-option-label'), allowDuplication, allowMove, allowRemove);
                $dropList.append(copyLi);
                // Make the new item draggable and removable.
                $('[data-acquia-lift-option-id="' + dropOptionId + '"]', $dropList).draggable(draggableConfiguration);
                $('[data-acquia-lift-option-id="' + dropOptionId + '"] .acquia-lift-targeting-remove', $dropList).on('click', handleDraggableItemRemove);
              } else {
                // Unselect this option in the draggable select input.
                dragSelectedOptions.splice(dragSelectedOptions.indexOf(dropOptionId), 1);
                $dragSelect.val(dragSelectedOptions);

                // Move the list item from the dragged list to the dropped list.
                $dropList.append(ui.draggable);
              }
            }
          });
        }

        // Hide the actual select input.
        $(this).hide();
      });
    }
  }


  /**
   * Theme function to generate a droppable region.
   */
  Drupal.theme.prototype.acquiaLiftTargetingDroppable = function () {
    var html = '';
    html += '<div class="acquia-lift-targeting-droppable">';
    html += 'Drop here to target this audience with this display option';
    html += '</div>';
    return html;
  }

  /**
   * Theme function to generate a draggable list item for a variation (display
   * option).
   *
   * @param id
   *   The variation id
   * @param label
   *   The variation label to display.
   * @param allowDuplication
   *   True if the draggable item can be copied as well as moved, false if the
   *   item can only be moved.
   * @param allowMove
   *   True if the draggable item can be moved from the container.  False if the
   *   draggable item is only available for copy.
   * @param allowRemove
   *   True if the draggable item can be removed from its current container.
   *   False if it cannot.
   */
  Drupal.theme.prototype.acquiaLiftTargetingDraggableItem = function(id, label, allowDuplication, allowMove, allowRemove) {
    var html = '';
    var classes = ['acquia-lift-targeting-draggable'];
    // If the item can be copied but not moved, then dragging anywhere on the
    // item should indicate a duplication action.
    if (allowDuplication && !allowMove) {
      classes.push('acquia-lift-targeting-duplicate');
    }
    html += '<li data-acquia-lift-option-id="' + id + '" data-acquia-lift-option-label="' + label + '" class="' + classes.join(' ') + '">';
    html += label;
    if (allowRemove) {
      html += '<span class="acquia-lift-targeting-remove">Remove</span>';
    }
    // Show an icon to duplicate this item in addition to the move label so that
    // the user can indicate one action or the other based on where they click.
    if (allowDuplication && allowMove) {
      html += '<span class="acquia-lift-targeting-duplicate-icon acquia-lift-targeting-duplicate">Duplicate</span>';
    }
    html += '</li>';
    return html;
  }

})(Drupal.jQuery, Drupal);
