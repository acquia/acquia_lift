/**
 * The basic backbone application collections for the unified navigation bar
 * tray.
 */
(function (Drupal, $, _, Backbone) {

  /**
   * A base collection class for common functionality.
   */
  var CollectionBase = Backbone.Collection.extend({
    initialize: function() {
      // Enable models to send a remove event when they are removed from
      // a collection via the reset() function.
      this.on('reset', function (col, opts) {
        _.each(opts.previousModels, function(model) {
          model.trigger('remove');
        });
      });
    }
  });

  /**
   * Provides campaign model management.
   */
  Drupal.acquiaLiftUI.MenuCampaignCollection = CollectionBase.extend({
    model: Drupal.acquiaLiftUI.MenuCampaignModel,
    initialize: function () {
      this.on('change:isActive', this.changeVisibility, this);
      this.parent('inherit', this.options);
    },
    changeVisibility: function (changedModel, value, options) {
      // When a campaign is deactivated, we don't need to enforce anything.
      if (changedModel.get('isActive') === false) {
        return;
      }
      // This campaign was activated; deactivate all other campaigns.
      this.each(function (model) {
        if (model.get('isActive') === true && model !== changedModel) {
          model.set('isActive', false);
        }
      });
    }
  });

  /**
   * A collection of option sets within a model.
   */
  Drupal.acquiaLiftUI.MenuOptionSetCollection = CollectionBase.extend({
    model: Drupal.acquiaLiftUI.MenuOptionSetModel,

    /**
     * {@inheritDoc}
     */
    initialize: function() {
      // Allow certain model changes to trigger a general change event for
      // the entire collection.
      this.variations = null;
      this.on('change:options', this.triggerChange, this);
      this.on('reset', this.triggerChange, this);
      this.parent('inherit', this.options);
    },

    /**
     * Causes the cached variation list to be reset.
     */
    triggerChange: function() {
      this.trigger('change:variations');
    }
  });

  /**
   *  A collection of options.
   */
  Drupal.acquiaLiftUI.MenuOptionCollection = CollectionBase.extend({
    model: Drupal.acquiaLiftUI.MenuOptionModel,

    initialize: function(options) {
      this.parent('inherit', this.options);
    },

    /**
     * Remember the original index for an option within the option set.
     */
    add: function(models, options) {
      if (_.isArray(models)) {
        _.each(models, function (model, index) {
          model.original_index = index;
        });
      }
      Backbone.Collection.prototype.add.call(this, models, options);
    }
  });

  /**
   * A collection of goals (used for a campaign).
   */
  Drupal.acquiaLiftUI.MenuGoalCollection = Backbone.Collection.extend({
    model: Drupal.acquiaLiftUI.MenuGoalModel
  });

}(Drupal, Drupal.jQuery, _, Backbone));
