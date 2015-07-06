/**
 * The basic backbone application models for the unified navigation bar
 * tray.
 */
(function (Drupal, $, _, Backbone) {
  /**
   * The model for a campaign.
   */
  Drupal.acquiaLiftUI.MenuCampaignModel = Backbone.Model.extend({
    defaults: {
      label: '',
      links: {},
      name: '',
      isActive: false,
      type: '',
      status: '',
      editable: false
    },

    /**
     * {@inheritDoc}
     */
    initialize: function (options) {
      if (!(this.get('goals') instanceof Backbone.Collection)) {
        this.set('goals', new Drupal.acquiaLiftUI.MenuGoalCollection());
      }
      this.set('optionSets', new Drupal.acquiaLiftUI.MenuOptionSetCollection());
      this.listenTo(this.get('optionSets'), 'add', this.triggerOptionSetChange);
      this.listenTo(this.get('optionSets'), 'remove', this.triggerOptionSetChange);
      this.listenTo(this.get('optionSets'), 'change:variations', this.triggerOptionSetChange);
      this.listenTo(this.get('goals'), 'add', this.triggerGoalsChange);
      this.listenTo(this.get('goals'), 'remove', this.triggerGoalsChange);
      this.listenTo(this.get('goals'), 'reset', this.triggerGoalsChange);
    },

    /**
     * {@inheritDoc}
     */
    set: function (property, value) {
      if (property.hasOwnProperty('goals')) {
        this.setGoals(property.goals);
        delete property.goals;
      } else if (property === 'goals' && !(value instanceof Backbone.Collection)) {
        this.setGoals(value, true);
        return;
      }
      Backbone.Model.prototype.set.call(this, property, value);
    },

    /**
     * Determine if the campaign should be included in navigation.
     */
    includeInNavigation: function () {
      var types = this.get('optionSetTypes');
      // Include any campaigns that don't have variations yet.
      if (!types || !types.length || types.length == 0) {
        return true;
      }
      // If the campaign has only personalize fields option sets and they
      // aren't on this page, then hide it.
      var i, num = types.length;
      for (i = 0; i < num; i++) {
        if (types[i] !== 'fields') {
          return true;
        }
      }
      // If still here, then all option sets are personalize fields.
      return this.getNumberOfVariations() > 0;
    },

    /**
     * Updates the goals collection based on an array/object of goal data.
     *
     * @param goals
     *   An object of goal labels keyed by goal ids.
     * @param triggerChange
     *   Boolean to indicate if a change notification should be sent.
     */
    setGoals: function (goals, triggerChange) {
      var goalCollection = this.get('goals');
      triggerChange = typeof(triggerChange) == 'undefined' ? false : triggerChange;
      if (!goalCollection) {
        this.set('goals', new Drupal.acquiaLiftUI.MenuGoalCollection());
        goalCollection = this.get('goals');
      }

      var hasChanged = false,
        goalIds = [];
      if (goals !== null) {
        _.each(goals, function (goalLabel, goalId) {
          goalIds.push(goalId);
          var goalModel = goalCollection.findWhere({'id': goalId});
          if (goalModel) {
            if (goalLabel !== goalModel.get('name')) {
              goalModel.set('name', goalLabel);
              hasChanged = true;
            }
          } else {
            goalCollection.add(new Drupal.acquiaLiftUI.MenuGoalModel({
              id: goalId,
              name: goalLabel
            }));
            hasChanged = true;
          }
        });
        // Check to see if any goals have been removed.
        var num = goalCollection.length, i = num - 1;
        for (i; i >= 0; i--) {
          var goalModel = goalCollection.at(i);
          if (_.indexOf(goalIds, goalModel.get('id')) == -1) {
            // This is no longer in the goals for the campaign.
            goalCollection.remove(goalModel);
            hasChanged = true;
          }
        }
      } else {
        if (goalCollection.length > 0) {
          goalCollection.reset();
          hasChanged = true;
        }
      }
      if (triggerChange && hasChanged) {
        this.triggerGoalsChange();
      }
    },

    /**
     * Triggers a change notification for option sets.
     */
    triggerOptionSetChange: function (event) {
      this.refreshData();
      this.trigger('change:optionSets');
    },

    /**
     * Triggers a change notification for goals
     */
    triggerGoalsChange: function () {
      this.trigger('change:goals');
    },

    /**
     * Helper function to get the number of variations tos how based on the
     * type of model.
     */
    getNumberOfVariations: function () {
      var optionSets = this.get('optionSets');
      return optionSets.length;
    },

    /**
     * Refreshes the active option selected for a campaign's options ets.
     */
    refreshData: function () {
      var optionSets = this.get('optionSets');
      optionSets.each(function (model) {
        var activeOption = model.get('activeOption');
        // If there is currently an active option, make sure it is still in the
        // options for this option set.
        if (activeOption) {
          var found = model.get('options').findWhere({'option_id': activeOption});
          if (!found) {
            activeOption = null;
          }
        }
        // If the activeOption has not been set, set it to a default.
        if (!activeOption) {
          // Default the active option to the first/control option.
          var index = 0;
          if (Drupal.settings.personalize.preselected && Drupal.settings.personalize.preselected.hasOwnProperty(model.get('decision_name'))) {
            // If there is an option pre-selected, then it should be the default active option.
            var preselectedOptionName = Drupal.settings.personalize.preselected[model.get('decision_name')];
            if (preselectedOptionName) {
              index = model.get('option_names').indexOf(preselectedOptionName);
              if (index < 0) {
                index = 0;
              }
            }
          } else if (model.get('winner') !== null) {
            // Otherwise a winner should be the default if one has been defined.
            index = model.get('winner');
          }
          var options = model.get('options');
          var activeOption = options.findWhere({'original_index': index});
          if (!activeOption) {
            activeOption = options.at(0);
          }
          if (activeOption) {
            model.set('activeOption', activeOption.get('option_id'));
          }
        }
      });
    }
  });

  /**
   * The model for a menu of option set links.
   */
  Drupal.acquiaLiftUI.MenuOptionSetModel = Backbone.Model.extend({
    defaults: {
      name: '',
      agent: '',
      agent_info: {},
      decision_name: '',
      executor: 'show',
      label: '',
      mvt: '',
      option_names: [],
      activeOption: null,
      osid: '',
      stateful: 1,
      type: null,
      winner: null,
      plugin: null
    },

    /**
     * {@inheritDoc}
     */
    initialize: function (options) {
      this.parent('inherit', options);
      if (!this.get('options')) {
        this.set('options', new Drupal.acquiaLiftUI.MenuOptionCollection());
      }
      this.listenTo(this.get('options'), 'add', this.triggerChange);
      this.listenTo(this.get('options'), 'remove', this.triggerChange);
    },

    /**
     * {@inheritDoc}
     */
    set: function (property, value) {
      // Tricky - the initial creation from object model data passes all data
      // to this function first.
      if (typeof property == 'object') {
        if (property.hasOwnProperty('options')) {
          this.setOptions(property.options);
          // Remove this property so the rest can still be processed.
          delete property.options;
        }
      } else {
        if (property === 'options' && !(value instanceof Drupal.acquiaLiftUI.MenuOptionCollection)) {
          this.setOptions(value);
          return;
        }
      }
      this.parent('set', property, value);
    },

    setOptions: function (options) {
      var current,
        triggerChange = false,
        optionIds = [],
        optionsCollection = this.get('options');
      if (!optionsCollection) {
        this.set('options', new Drupal.acquiaLiftUI.MenuOptionCollection());
        optionsCollection = this.get('options');
      }

      _.each(options, function (option, option_index) {
        optionIds.push(option.option_id);
        // Update the model properties if the model is already in options.
        if (current = optionsCollection.findWhere({'option_id': option.option_id})) {
          _.each(option, function (optionValue, optionProp) {
            if (current.get(optionProp) !== optionValue) {
              current.set(optionProp, optionValue);
              triggerChange = true;
            }
          });
        } else {
          // Otherwise just add the new option.
          option.original_index = option_index;
          optionsCollection.add(option);
          triggerChange = true;
        }
      });
      // Check to see if any options have been removed.
      var num = optionsCollection.length, i = num - 1;
      for (i; i >= 0; i--) {
        var optionModel = optionsCollection.at(i);
        if (_.indexOf(optionIds, optionModel.get('option_id')) == -1) {
          // This is no longer in the options for the option set.
          optionsCollection.remove(optionModel);
          triggerChange = true;
        }
      }
      if (triggerChange) {
        this.triggerChange();
      }
      return optionsCollection;
    },

    /**
     * Force a change event whenever the options collection changes.
     */
    triggerChange: function (event) {
      this.trigger('change:options');
    }
  });

  /**
   * The model for a single option within an option set.
   */
  Drupal.acquiaLiftUI.MenuOptionModel = Backbone.Model.extend({
    defaults: {
      option_id: '',
      option_label: '',
      original_index: null,
      editable: false,
      deletable: false
    }
  });

  /**
   * The model for a single goal.
   */
  Drupal.acquiaLiftUI.MenuGoalModel = Backbone.Model.extend({});

  /**
   * The model for 'add variation' state for element variations.
   */
  Drupal.acquiaLiftUI.MenuElementVariationModeModel = Backbone.Model.extend({
    defaults: {
      isActive: false,
      isEditMode: false,
      variationIndex: -1
    },

    initialize: function () {
      var that = this;
      $(document).on('acquiaLiftMenuAction', function() {
        that.endEditMode();
      });
    },

    /**
     * Helper function to start adding a variation.
     *
     * This handles setting all the correct model parameters and sending an
     * event notification.
     */
    startAddMode: function () {
      // Don't restart if already in create mode.
      if (this.get('isActive') && !this.get('isEditMode')) {
        return;
      }
      this.set('variationIndex', -1);
      this.set('isEditMode', false);
      this.set('isActive', true);
      this.notifyTrigger();
    },

    /**
     * Helper function to start editing a variation.
     *
     * This handles setting all the correct model parameters and sending an
     * event notification.
     *
     * @param variationIndex
     *   Index of the variation to edit within the current campaign context.
     */
    startEditMode: function (variationIndex) {
      // Don't do anything if we are already editing the same variation.
      if (this.get('isActive') && this.get('variationIndex') == variationIndex) {
        return;
      }
      this.set('variationIndex', variationIndex);
      this.set('isEditMode', true);
      this.set('isActive', true);
      this.notifyTrigger();
    },

    /**
     * Helper function to end editing mode for a variation.
     *
     * This handles setting all the correct model parameters back and sending
     * an event notification.
     */
    endEditMode: function () {
      // If editing mode isn't already active, then just return.
      if (!this.get('isActive')) {
        return;
      }
      this.set('variationIndex', -1);
      this.set('isEditMode', false);
      this.set('isActive', false);
      this.notifyTrigger();
    },

    /**
     * Send a notification of the trigger in variation mode change.
     */
    notifyTrigger: function () {
      var data = {
        start: this.get('isActive'),
        variationIndex: this.get('variationIndex')
      };
      $(document).trigger('acquiaLiftElementVariationModeTrigger', [data]);
    }
  });

}(Drupal, Drupal.jQuery, _, Backbone));
