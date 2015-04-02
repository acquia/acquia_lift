/**
 * @file acquia_lift.variations.models.js
 * 
 * Backbone models for variation application.
 */
(function($, Drupal, Dialog, Backbone, _) {

  Drupal.acquiaLiftVariations.models = Drupal.acquiaLiftVariations.models || {};

  $.extend(Drupal.acquiaLiftVariations.models, {
  /**
     * Backbone model for the variations process.
     */
    AppModel: Backbone.Model.extend({
      defaults: {
        // If this app is being loaded, it is because it is being launched into
        // an edit mode.
        editMode: true,
        // The current ElementVariationModel that's being edited.
        variation: null
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
          variation: null
        }
      )
    }),

    /**
     * The model for a variation within a personalize elements option set.
     */
    ElementVariationModel: Backbone.Model.extend({
      defaults: {
        osid: null,
        optionId: null,
        option: null
      },

      initialize: function () {
        var osid = this.get('osid'),
          optionId = this.get('optionId'),
          that = this;
        if (Drupal.settings.personalize.option_sets.hasOwnProperty(osid)) {
          var options = Drupal.settings.personalize.option_sets[osid].options;
          _.each(options, function (option) {
            if (option['option_id'] === optionId) {
              that.set('option', option);
            }
          });
        }
      },

      getVariationNumber: function () {
        return this.get('optionId');
      },

      getVariationLabel: function () {
        var option = this.get('option');
        return option ? option.option_label : Drupal.t('Variation');
      },

      getContent: function () {
        var option = this.get('option');
        return option ? option.personalize_elements_content : '';
      }
    })
  });

}(Drupal.jQuery, Drupal, Drupal.visitorActions.ui.dialog, Backbone, _));
