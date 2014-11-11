/**
 * Custom Drupal AJAX commands used for the unified navigation bar tray.
 */
(function(Drupal, $) {

  /**
   * Custom AJAX command to preview a specific page variation.
   *
   * The response should include a data object with the following keys:
   * - agentName: The name of the campaign for this page variation.
   * - variationIndex: The variation index to edit.  This can be an existing
   *   variation index to edit, or -1 to create a new variation.
   */
  Drupal.ajax.prototype.commands.acquia_lift_page_variation_preview = function (ajax, response, status) {
    var view = Drupal.acquiaLiftUI.views.pageVariations[response.data.agentName]
    view.selectVariation(response.data.variationIndex);
  }

  /**
   * Custom AJAX command to indicate a deleted page variation.
   * This is necessary because Drupal's settings merge utilizes jQuery.extend
   * which will only add to the original object.
   *
   * The response should include a data object with the following keys:
   * - option_sets:  An updated array of option sets.
   */
  Drupal.ajax.prototype.commands.acquia_lift_option_set_updates = function (ajax, response, status) {
    var osid, option_sets = response.data.option_sets;

    if (option_sets) {
      for (osid in option_sets) {
        if (option_sets.hasOwnProperty(osid)) {
          // A campaign's option sets are empty so remove from the settings.
          if (osid === 'empty') {
            var empty_agent = option_sets[osid];
            for (var option_set_id in Drupal.settings.personalize.option_sets) {
              if (Drupal.settings.personalize.option_sets[option_set_id]['agent'] == empty_agent) {
                delete Drupal.settings.personalize.option_sets[option_set_id];
              }
            }
            // Notify of the deleted option sets.
            $(document).trigger('acquiaLiftOptionSetsEmpty', [empty_agent]);
          } else {
            Drupal.settings.personalize.option_sets[osid] = option_sets[osid];
          }
        }
      }
    }
  }

  /**
   * Custom AJAX command to indicate a deleted goal.
   * This is necessary because Drupal's settings merge utilizes jQuery.extend
   * which will only add to the original object.
   *
   * The response should include a data object with the following keys:
   * - campaigns: object of affected campaigns keyed by machine name
   */
  Drupal.ajax.prototype.commands.acquia_lift_goal_updates = function (ajax, response, status) {
    var campaignId, goalId, campaigns = response.data.campaigns;

    for (campaignId in campaigns) {
      Drupal.settings.personalize.campaigns[campaignId] = campaigns[campaignId];
    }
  }

}(Drupal, Drupal.jQuery));
