/**
 * @file acquia_lift.variations.models.js
 * 
 * Backbone models for variation application.
 */
(function($, Drupal, Dialog, Backbone, _) {

  Drupal.acquiaLiftVariations.models = Drupal.acquiaLiftVariations.models || {
    /**
     * Backbone model for the variations process.
     */
    AppModel: Backbone.Model.extend({
      MODEL_MODE_PAGE: 'page',
      MODEL_MODE_ELEMENT: 'element',

      defaults: {
        // If this app is being loaded, it is because it is being launched into
        // an edit mode.
        editMode: true,
        modelMode: this.MODEL_MODE_PAGE,
        // The current variation being edited.
        // This will be an integer for a page variation or an option id
        // for an element variation.
        variationIndex: -1
      },

      /**
       * Set the model mode.
       *
       * The mode can be either page-level variations (used for simple a/b
       * tests) or individual variations (used for all other campaigns).
       */
      setModelMode: function (pageLevel) {
        this.set('modelMode', pageLevel ? this.MODEL_MODE_PAGE : this.MODEL_MODE_ELEMENT);
      },

      /**
       * Determine if the model is in page variation mode or element mode.
       *
       * @returns boolean
       * True if page variation mode, false otherwise.
       */
      isPageModelMode: function () {
        return this.get('modelMode') === this.MODEL_MODE_PAGE;
      },

      /**
       * {@inheritdoc}
       */
      destroy: function (options) {
        this.trigger('destroy', this, this.collection, options);
      }
    }),

    /**
     * Backbone model representing a single element variation type
     * that can be presented within a contextual menu.
     *
     * Examples:  edit HTML, edit text, add class, etc.
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
