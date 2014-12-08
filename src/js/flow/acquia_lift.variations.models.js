/**
 * @file acquia_lift.variations.models.js
 * 
 * Backbone models for variation application.
 */
(function($, Drupal, Dialog, Backbone, _) {

  Drupal.acquiaLiftVariations.models = Drupal.acquiaLiftVariations.models || {
    /**
     * Backbone model for the page variation process.
     */
    AppModel: Backbone.Model.extend({
      defaults: {
        // If this app is being loaded, it is because it is being launched into
        // an edit mode.
        editMode: true,
        variationIndex: -1
      },

      /**
       * {@inheritdoc}
       */
      destroy: function (options) {
        this.trigger('destroy', this, this.collection, options);
      }
    }),

    /**
     * Backbone model representing a single page element variation type
     * that can be presented within a contextual menu.
     */
    ElementVariationModel: Backbone.Model.extend({
      defaults: {
        limitByChildrenType: ''
      }
    }),


    /**
     * Backbone model for a variation type form.
     */
    VariationTypeFormModel: Dialog.models.DialogModel.extend({
      defaults: _.extend({}, Dialog.models.DialogModel.prototype.defaults,
        {
          // A type of variation, e.g. 'editHTML', 'prependHTML'
          type: null,
          // The label for the variation type.
          typeLabel: null,
          selector: null,
          variationIndex: -1
        }
      )
    })
  };

}(Drupal.jQuery, Drupal, Drupal.visitorActions.ui.dialog, Backbone, _));
