/**
 * @file acquia_lift.variations.models.js
 * 
 * Backbone models for variation application.
 */
(function($, Drupal, Dialog, Backbone, _) {

  Drupal.acquiaLiftVariations.models = Drupal.acquiaLiftVariations.models || {};

  /**
   * Base model for a variation that can be shown or edited.
   */
  Drupal.acquiaLiftVariations.models.VariationModel: Backbone.Model.extend({
    // Each type of variation overrides this function to return its content.
    getContent: function () {
      return '';
    }
  });

  $.extend(Drupal.acquiaLiftVariations.models, {
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
    VariationTypeModel: Backbone.Model.extend({
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
    }),

    /**
     * Model for a variation that can be shown or edited.
     */
    VariationModel: Backbone.Model.extend({
      // Each type of variation overrides this function to return its content.
      getContent: function () {
        return '';
      }
    })
  };

  /**
   * The model for a variation within a personalize elements option set.
   */
  Drupal.acquiaLiftVariations.models.ElementVariationModel = Drupal.acquiaLiftVariations.models.VariationModel.extend({
    defaults: {
      osid: null,
      optionId: null
    },

    getContent: function () {
      if (Drupal.settings.personalize.option_sets.hasOwnProperty(osid)) {
        var options = Drupal.settings.personalize.option_sets[osid].options;
        _.each(options, function(option) {
          if (option['option_id'] == this.get('optionId')) {
            return option['personalize_elements_content'];
          }
        });
      }
      return '';
    }
  });

  /**
   * The model for a variation within a page variation.
   */
  Drupal.acquiaLiftVariations.models.PageVariationModel = Drupal.acquiaLiftVariations.models.VariationModel.extend({
    defaults: {
      agentName: null,
      variationIndex: -1,
      selector: null
    },

    getContent: function () {
      var variationIndex = this.get('variationIndex'),
        agentName = this.get('agentName'),
        selector = this.get('selector');
      if (!agentName || !selector) {
        return '';
      }
      // Find the right option set for this agent and selector.
      _.each(Drupal.settings.personalize.option_sets, function(option_set) {
        if (option_set.agent === agentName && option_set.selector === selector) {
          if (option_set.options.hasOwnProperty(variationIndex)) {
            return option_set.options[variationIndex].personalize_elements_content;
          }
        }
      });
      return '';
    }
  });

}(Drupal.jQuery, Drupal, Drupal.visitorActions.ui.dialog, Backbone, _));
