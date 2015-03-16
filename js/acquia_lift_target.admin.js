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
    }
  }


  Drupal.theme.acquiaLiftTargetingDroppable = function () {
    var html = '';
    html += '<div class="acquia-lift-targeting-droppable">';
    html += 'Drop variations here';
    html += '</div>';
    return html;
  }

})(Drupal.jQuery, Drupal);
