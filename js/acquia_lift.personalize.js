/**
 * Theme functions for the unified navigation bar tray.
 */

(function(Drupal, $) {

  var navbarMenuClassName = Drupal.settings.acquia_lift.menuClass;

  /**
   * Replaces spaces and underscored with dashes in a string.
   *
   * @param string str
   *   The string to be cleaned.
   *
   * @return str
   *   The updated string.
   */
  function formatClass (str) {
    if (!str) return '';
    return str.trim().replace(/[\s\_]+/g, '-');
  }

  /**
   * Returns an href with the preview parameter for the option set option.
   *
   * The URL fragment is maintained.
   *
   * @param mixed options
   *   An array of options for preselection or a single options object.
   *   Each option object has the following keys:
   *   - osID: The ID of the option set.
   *   - os: The option set object.
   *
   * @return string
   */
  function generateHref (options) {
    // Removes leading '/', '?' and '#' characters from a string.
    var pathRegex = /^(?:[\/\?\#])*(.*)/;
    var base = Drupal.settings.basePath + Drupal.settings.pathPrefix;
    var path = location.pathname && pathRegex.exec(location.pathname)[1] || '';
    var param = Drupal.settings.personalize.optionPreselectParam;

    var href = base + path + '?' + param + '=';
    if (!(options instanceof Array)) {
      options = [options];
    }
    var params = [];
    var osids = [];
    _.each(options, function (element, index, list) {
      osids.push(element.osID);
      params.push(element.osID + '--' + element.id);
    });
    href += params.join();

    // Now we need to add on any other Option Sets for which a preview option
    // had been selected so that we can preview more than one at a time.
    var existingSelection = decodeURI(window.location.search.replace(new RegExp("^(?:.*[&\\?]" + encodeURI(param).replace(/[\.\+\*]/g, "\\$&") + "(?:\\=([^&]*))?)?.*$", "i"), "$1"));
    if (existingSelection.length > 0) {
      Drupal.acquiaLiftUI.utilities.looper(existingSelection.split(','), function (str, key) {
        // Exclude any preselection for the Option Set we're generating a link for.
        var existingOsid = str.split('--')[0];
        if (osids.indexOf(existingOsid) == -1) {
          href += ',' + str;
        }
      });
    }

    return href;
  }

  /**
   * Returns HTML for a count display element.
   */
  Drupal.theme.acquiaLiftCount = function (options) {
    return '<i class="acquia-lift-personalize-type-count acquia-lift-empty"><span>0</span>&nbsp;</i>';
  };

  /**
   * Returns the HTML for the selected context label.
   *
   * @param $options
   * - label: The label of the selected context
   * - append: Something to be appended to the label but not at the same
   *   level of visual importance.
   * - category: The type of context
   */
  Drupal.theme.acquiaLiftSelectedContext = function (options) {
    var label = options.category + ': ';
    label += '<span class="acquia-lift-active">' + options.label + '</span>';
    if (options.hasOwnProperty('append')) {
      label += '&nbsp;(' + options.append + ')';
    }
    return '<span class="acquia-lift-active-container">' + label + '</span>';
  }

  /**
   * Throbber.
   */
  Drupal.theme.acquiaLiftThrobber = function () {
    return '<div class="ajax-progress ajax-progress-throbber"><div class="throbber">&nbsp;</div></div>';
  };

  /**
   * Returns an empty campaign menu item.
   */
  Drupal.theme.acquiaLiftPersonalizeNoMenuItem = function (options) {
    var attrs = [
      'class="acquia-lift-menu-disabled"'
    ];

    var item = '\n<span ' + attrs.join(' ') + '>\n';
    item += Drupal.t('No @type', {'@type': options.type}) + '\n';
    item += '</span>\n';

    return item;
  }

  /*****************************************
   * C A M P A I G N S
   *****************************************/

  /**
   * Returns a menu item that contains a link to a campaign.
   *
   * @param object options
   *   Keys:
   *   - id: The id of the campaign.
   *   - label: The label of the campaign.
   *   - href: The edit link to the campaign.
   *
   * @return string
   */
  Drupal.theme.acquiaLiftPersonalizeCampaignMenuItem = function (options) {
    var editAttrs = [
      'class="acquia-lift-campaign-edit acquia-lift-menu-link"',
      'title="' + Drupal.t('Edit the @campaign personalization', {'@campaign': options.link.label}) + '"',
      'href="' + options.edit.href + '"'
    ];

    var linkAttrs = [
      'class="acquia-lift-campaign acquia-lift-campaign--' + formatClass(options.link.id) + ' visitor-actions-ui-ignore"',
      'href="' + options.link.href + '"',
      'title="' + Drupal.t('Preview the @campaign personalization', {'@campaign': options.link.label}) + '"',
      'data-acquia-lift-personalize-agent="' + options.link.id + '"',
      'aria-role="button"',
      'aria-pressed="false"'
    ];

    var item = '<div class="acquia-lift-menu-item clearfix"><a ' + linkAttrs.join(' ') + '>' + options.link.label + '</a>';
    item += '<div class="acquia-lift-menu-actions">';
    item += '<a ' + editAttrs.join(' ') + '>' + Drupal.t('Edit') + '</a>\n';
    item += '</div></div>\n';
    //item += '<a ' + linkAttrs.join(' ') + '>' + options.link.label + '</a>\n';

    return item;
  };

  /*****************************************
   * O P T I O N  S E T S
   *****************************************/

  /**
   * Returns a menu item that display information about an uneditable
   * campaign.
   */
  Drupal.theme.acquiaLiftPersonalizeNoEditItem = function (options) {
    var attrs = [
      'class="acquia-lift-menu-campaign-warning"'
    ];

    var item = '';
    item += '\n<span ' + attrs.join(' ') + '>\n';
    item += Drupal.t('Personalizations that are running cannot be edited.') + '\n';
    item += '</span>\n';

    return item;
  }

  /**
   * Returns a list item that contains links to preview option set options.
   *
   * @param object options
   *   Keys:
   *   - osID: The ID of the option set.
   *   - os: The option set object.
   *   - os.label: The label of the option set.
   *   - os.agent: The campaign/agent to which this option set belongs.
   *   - editable: Boolean indicating if the campaign is editable.
   *
   * @return string
   */
  Drupal.theme.acquiaLiftOptionSetItem = function (options) {
    var attrs = [
      'class="acquia-lift-preview-option-set acquia-lift-content-variation navbar-menu-item acquia-lift-preview-option-set-' + formatClass(options.osID)  + '"',
      'data-acquia-lift-personalize-id="' + options.osID + '"',
      'data-acquia-lift-personalize-agent="' + options.os.agent + '"'
    ];
    var item = '';
    item += '<span ' + attrs.join(' ') + '>' + Drupal.checkPlain(options.os.label) + '</span>';
    item += Drupal.theme('acquiaLiftOptionSetMenu', options);
    return item;
  };

  /**
   * Returns a menu item that contains links to preview option set options.
   *
   * @param object options
   *   Keys:
   *   - osID: The ID of the option set.
   *   - os: The option set object.
   *   - os.option_id: The ID of an option set option.
   *   - os.option_label: The label of an option set option.
   *   - os.deletable: Boolean indicating if the option is deletable from the
   *     menu.
   *   - os.editable: Boolean indicating if the option is editable from the menu.
   *   - editable: Boolean indicating if the entire campaign is editable.
   *
   * @return string
   */
  Drupal.theme.acquiaLiftOptionSetMenu = function (options) {
    var menu = '<ul class="' + navbarMenuClassName + '">' + "\n";
    var osID = options.osID;
    var os = options.os;
    var os_selector = os.selector;
    options.os.options.each(function(model) {
      menu += Drupal.theme('acquiaLiftPreviewOptionMenuItem', {
        id: model.get('option_id'),
        label: model.get('option_label'),
        osID: osID,
        osSelector: os_selector,
        showDelete: model.get('deletable'),
        showEdit: model.get('editable'),
        editable: options.editable
      });
    });
    if (options.editable && os.plugin === 'elements') {
      menu += '<li>';
      menu += '<a href="' + Drupal.settings.basePath + Drupal.settings.pathPrefix + 'admin/structure/personalize/variations/add/nojs"';
      menu += ' class="acquia-lift-variation-add acquia-lift-menu-link" title="' + Drupal.t('Add variation') + '" aria-role="button" aria-pressed="false">';
      menu += Drupal.t('Add variation');
      menu += '</a></li>';
    }
    menu += '</ul>\n';
    return menu;
  };

  /**
   * Returns a menu item that contains a link to an option set option.
   *
   * @param object options
   *   Keys:
   *   - id: The id of the option set option.
   *   - label: The label of the option set option.
   *   - osID: The ID of the option set.
   *   - osSelector: The selector representing the option set.
   *   - showDelete: Indicates if the delete option should be available for this
   *     particular item.
   *   - showEdit: Indicates if the edit option should be available for this
   *     item.
   *   - editable: Indicates if campaign is editable.
   *
   * @return string
   */
  Drupal.theme.acquiaLiftPreviewOptionMenuItem = function (options) {
    var item = '';
    var ariaAttrs = [
      'aria-role="button"',
      'aria-pressed="false"'
    ];
    // Prepare the selector string to be passed as a data attribute.
    var selector = options.osSelector.replace(/\"/g, '\'');
    var previewAttrs = [
      'class="acquia-lift-preview-option acquia-lift-preview-option--' + formatClass(options.id) + ' visitor-actions-ui-ignore"',
      'href="' + generateHref(options) + '"',
      'data-acquia-lift-personalize-option-set="' + options.osID + '"',
      'data-acquia-lift-personalize-option-set-selector="' + selector + '"',
      'data-acquia-lift-personalize-option-set-option="' + options.id + '"'
    ].concat(ariaAttrs);

    var deleteHref = Drupal.settings.basePath + Drupal.settings.pathPrefix + 'admin/structure/acquia_lift/variation/delete/' + options.osID + '/' + options.id + '/nojs';
    var deleteClasses = 'acquia-lift-variation-delete acquia-lift-menu-link';
    var deleteAttrs = [
      'data-acquia-lift-personalize-option-set-option="' + options.id + '"'
    ].concat(ariaAttrs);
    if (options.showDelete && options.editable) {
      deleteAttrs.push('class="' + deleteClasses + ' ctools-use-modal ctools-modal-acquia-lift-style"');
      deleteAttrs.push('title="' + Drupal.t('Delete variation') + '"');
      deleteAttrs.push('href="' + deleteHref + '"');
    } else {
      deleteAttrs.push('class="' + deleteClasses + ' acquia-lift-disabled"');
      deleteAttrs.push('title="' + Drupal.t('Variation cannot be deleted until the personalization is paused."'));
    }

    var editClasses = 'acquia-lift-variation-edit acquia-lift-menu-link';
    var editHref = '#';
    var editAttrs = [
      'data-acquia-lift-personalize-option-set-option="' + options.id + '"',
    ].concat(ariaAttrs);
    if (options.editable) {
      editAttrs.push('class="' + editClasses + '"');
      editAttrs.push('href="' + editHref + '"');
      editAttrs.push('title="' + Drupal.t('Edit variation') + '"');
    } else {
      editAttrs.push('class="' + editClasses + ' acquia-lift-disabled"');
      editAttrs.push('title="' + Drupal.t('Variations cannot be edited until the personalization is paused.') + '"');
    }

    item += '<li>\n<div class="acquia-lift-menu-item clearfix" data-acquia-lift-personalize-option-set="' + options.osID + '">';
    if (options.id !== Drupal.settings.personalize.controlOptionName) {
      item += '<div class="acquia-lift-menu-actions">';
      if (options.showEdit) {
        item += '<a ' + editAttrs.join(' ') + '>' + Drupal.t('Edit') + '</a>\n';
      }
      item += '<a ' + deleteAttrs.join(' ') + '>' + Drupal.t('Delete') + '</a>\n';
      item += '</div>';
    }
    item += '<a ' + previewAttrs.join(' ') + '>' + options.label + '</a> \n';
    item += '</div></li>';
    return item;
  };

  /*****************************************
   * G O A L S
   *****************************************/

  /**
   * Returns the HTML for the goals list for a campaign.
   *
   * @param MenuCampaignModel model
   *   The campaign model to create goals display for.
   * @param object actions
   *   An object of all actions keyed by the action machine name.
   * @param boolean editable
   *   Indicates if the goal should have edit options.
   */
  Drupal.theme.acquiaLiftCampaignGoals = function (model, actions, editable) {
    var goals = model.get('goals');
    var html = '<ul class="' + navbarMenuClassName + '">';

    if (goals.length == 0) {
      html += '<li>';
      html += Drupal.theme('acquiaLiftPersonalizeNoMenuItem', {
        type: 'goals'
      });
      html += '</li>';
      return html;
    }
    goals.each(function (goalModel) {
      var goalId = goalModel.get('id');
      var custom = actions.hasOwnProperty(goalId);
      html += '<li>';
      html += Drupal.theme('acquiaLiftPersonalizeGoal', {
        campaignID: model.get('name'),
        name: goalId,
        label: goalModel.get('name'),
        custom: custom,
        editable: editable
      });
      html += '</li>';
    });
    html += '</ul>';
    return html;
  }


  /**
   * Returns a campaign goal menu item.
   *
   * @param object options
   *   Keys:
   *   - campaignID: The ID of the campaign for these goals.
   *   - name: The goal ID.
   *   - label: The goal label.
   *   - custom: Boolean to indicate if a goal is custom or defined in code.
   *   - editable: Boolean to indicate if the goal is editable.
   *
   * @return string
   */
  Drupal.theme.acquiaLiftPersonalizeGoal = function (options) {
    var campaignID = options.campaignID;
    var item = '';

    var attrs = [
      'class="acquia-lift-goal acquia-lift-goal--' + formatClass(campaignID) + ' visitor-actions-ui-ignore"',
      'data-acquia-lift-personalize-agent="' + campaignID + '"',
      'data-acquia-lift-personalize-goal="' + options.name + '"'
    ];

    var renameHref = Drupal.settings.basePath + Drupal.settings.pathPrefix + 'admin/structure/acquia_lift/goal/rename/' + options.name + '/nojs';
    var renameClasses = 'acquia-lift-goal-rename acquia-lift-menu-link';
    var renameAttrs = [
      'aria-role="button"',
      'aria-pressed="false"',
    ];
    if (options.editable) {
      renameAttrs.push('title="' + Drupal.t('Rename goal') + '"');
      renameAttrs.push('href="' + renameHref + '"');
      renameAttrs.push('class="' + renameClasses + ' ctools-use-modal ctools-modal-acquia-lift-style"');
    } else {
      renameAttrs.push('title="' + Drupal.t('Goals cannot be edited until personalization is paused.') + '"');
      renameAttrs.push('class="' + renameClasses + ' acquia-lift-disabled"');
    }

    var deleteHref = Drupal.settings.basePath + Drupal.settings.pathPrefix + 'admin/structure/acquia_lift/goal/delete/' + options.campaignID + '/' + options.name + '/nojs';
    var deleteClasses = 'acquia-lift-goal-delete acquia-lift-menu-link';
    var deleteAttrs = [
      'aria-role="button"',
      'aria-pressed="false"',
    ];
    if (options.editable) {
      deleteAttrs.push('title="' + Drupal.t('Delete goal') + '"');
      deleteAttrs.push('href="' + deleteHref + '"');
      deleteAttrs.push('class="' + deleteClasses + ' ctools-use-modal ctools-modal-acquia-lift-style"');
    } else {
      deleteAttrs.push('title="' + Drupal.t('Goals cannot be deleted until personalization is paused.') + '"');
      deleteAttrs.push('class="' + deleteClasses + ' acquia-lift-disabled"');
    }

    item += '<div class="acquia-lift-menu-item clearfix">\n';
    item += '<div class="acquia-lift-menu-actions">';
    if (options.custom) {
      item += '<a ' + renameAttrs.join(' ') + '>' + Drupal.t('Rename') + '</a>\n';
    }
    item += '<a ' + deleteAttrs.join(' ') + '>' + Drupal.t('Delete') + '</a>\n';
    item += '</div>';
    item += '<span ' + attrs.join(' ') + '>' + Drupal.t('@text', {'@text': options.label}) + '</span>\n';
    item += '</div>'
    return item;
  };

}(Drupal, Drupal.jQuery));

/**
 * The basic backbone application shared utilities for the unified navigation
 * bar tray.
 */
(function (Drupal, $, _, Backbone) {

  Drupal.acquiaLiftUI = Drupal.acquiaLiftUI || {};
  Drupal.acquiaLiftUI.views = Drupal.acquiaLiftUI.views || [];
  Drupal.acquiaLiftUI.views.optionSets = Drupal.acquiaLiftUI.views.optionSets || {};
  Drupal.acquiaLiftUI.models = Drupal.acquiaLiftUI.models || {};
  Drupal.acquiaLiftUI.collections = Drupal.acquiaLiftUI.collections || {};
  Drupal.acquiaLiftUI.collections['option_sets'] = Drupal.acquiaLiftUI.collections['option_sets'] || {};

  var initialized = false;
  Drupal.acquiaLiftUI.utilities = Drupal.acquiaLiftUI.utilities || {};
  Drupal.acquiaLiftUI.utilities.updateNavbar = _.throttle(function() {
    if (initialized && Drupal.behaviors.acquiaLiftNavbarMenu) {
      Drupal.behaviors.acquiaLiftNavbarMenu.attach();
      Drupal.behaviors.ZZCToolsModal.attach($('.acquia-lift-controls'));
    }
  }, 500);

  Drupal.acquiaLiftUI.utilities.setInitialized = function(value) {
    initialized = value;
  };

  Drupal.acquiaLiftUI.utilities.shutDownGoalsUI = _.throttle(function () {
    $(document).trigger('visitorActionsUIShutdown');
  }, 500);

  /**
   * Apply a callback to values in an object.
   *
   * @param object obj
   *   The object to be looped over.
   * @param function cb
   *   The callback to be invoked on each object value.
   *
   * @return object
   *   The merged results of the callback.
   */
  Drupal.acquiaLiftUI.utilities.looper = function (obj, cb) {
    var composite = {};
    for (var key in obj) {
      if (obj.hasOwnProperty(key)) {
        var args = Array.prototype.slice.call(arguments, 2);
        // Pass the object item and its key as the first two arguments of the
        // callback, in addition to any other arguments passed to this function.
        args.unshift(obj[key], key);
        // Merge in the return objects from the callback.
        var ret = cb.apply(obj, args);
        if (ret && typeof ret === 'object') {
          $.extend(composite, ret);
        }
      }
    }
    return composite;
  }

  /**
   * Finds the model of the active campaign sets isActive to true.
   *
   * @param string activeCampaign
   *   The name of the active campaign.
   */
  Drupal.acquiaLiftUI.setActiveCampaign = function (activeCampaign) {
    // Refresh the model data for the campaigns.
    var newCampaign = Drupal.acquiaLiftUI.collections['campaigns'].findWhere({'name': activeCampaign});
    if (newCampaign) {
      newCampaign.set('isActive', true);
    } else {
      activeCampaign = '';
    }
    Drupal.settings.personalize.activeCampaign = activeCampaign;
  };

  /**
   * Updates the active campaign session variable on the server.
   *
   * @param string name
   *   The name of the selected campaign.
   */
  Drupal.acquiaLiftUI.setActiveCampaignAjax = function (name, view) {
    var url = Drupal.settings.personalize && Drupal.settings.personalize.links.campaigns.setActive;
    if (url) {
      // This could do with some error checking.
      $.ajax({
        url: url.replace('%personalize_agent', name),
        success: function (response) {
          if (response.personalize_campaign) {
            Drupal.acquiaLiftUI.setActiveCampaign(response.personalize_campaign);
          }
        },
        complete: function () {
          // @todo Finish styling a throbber for campaign links that fire an
          // ajax event.
          //view.$el.find('.acquia-lift-campaign').next('.ajax-progress').remove();
        }
      });
    }
  };

}(Drupal, Drupal.jQuery, _, Backbone));

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

/**
 * The basic backbone application components for the unified navigation bar
 * tray.
 */
(function (Drupal, $, _, Backbone) {

  // Menu classes added for new disabled create menu links.
  var liftAddMenuClasses = 'acquia-lift-menu-create acquia-lift-menu-link overlay-exclude navbar-menu-item visitor-actions-ui-ignore';

  /**
   * Returns the Backbone View of the Visitor Actions add action controller.
   *
   * @return Backbone.View
   */
  function getVisitorActionsAppModel () {
    return Drupal.visitorActions && Drupal.visitorActions.ui && Drupal.visitorActions.ui.models && Drupal.visitorActions.ui.models.appModel;
  }

  /**
   * Common view functionality to ensure clean removal of views.
   */
  var ViewBase = Backbone.View.extend({

    /**
     * Builds HTML and themes it.
     */
    build: function () {},
    /**
     * {@inheritdoc}
     */
    remove: function (releaseElement) {
      if (this.undelegateEvents) {
        this.undelegateEvents();
      }
      if (this.stopListening) {
        this.stopListening();
      }
      this.$el
        .removeData()
        .off()
        .empty()
        .removeClass('acquia-lift-processed');

      if (releaseElement) {
        this.setElement(null);
      }

      Backbone.View.prototype.remove.call(this);
    }
  });

  /***************************************************************
   *
   *            M A I N  M E N U
   *
   ***************************************************************/

  /**
   * View/controller for full menu list.
   */
  Drupal.acquiaLiftUI.MenuView = ViewBase.extend({

    /**
     * {@inheritdoc}
     */
    initialize: function (options) {
      // The campaign collection.
      this.collection = options.collection;
      this.listenTo(this.collection, 'change:isActive', this.render);
      this.render();
    },

    /**
     * {@inheritdoc}
     */
    render: function() {
      var hasCampaigns = this.collection.length > 0;
      var activeCampaign = this.collection.findWhere({'isActive': true});
      var links = activeCampaign ? activeCampaign.get('links') : {};
      var supportsGoals = activeCampaign && links.hasOwnProperty('goals') && links.goals.length > 0;
      var supportsTargeting = activeCampaign && links.hasOwnProperty('targeting') && links.targeting.length > 0;
      // Show or hide relevant menus.
      if (hasCampaigns && activeCampaign) {
        this.$el.find('[data-acquia-lift-personalize="goals"]').parents('li').toggle(supportsGoals);
        this.$el.find('[data-acquia-lift-personalize="targeting"]').parents('li').toggle(supportsTargeting);
        this.$el.find('[data-acquia-lift-personalize="option_sets"]').parents('li').show();
      } else {
        this.$el.find('[data-acquia-lift-personalize="goals"]').parents('li').hide();
        this.$el.find('[data-acquia-lift-personalize="option_sets"]').parents('li').hide();
      }
    },

    getMenuActive: function() {
      return this.$el.hasClass('navbar-active');
    },

    setMenuActive: function(isActive) {
      if (isActive) {
        this.$el.addClass('navbar-active');
      } else {
        this.$el.removeClass('navbar-active');
      }
    }
  });

  /***************************************************************
   *
   *            C A M P A I G N S
   *
   ***************************************************************/

  /**
   * View/controller for the campaign menu header.
   */
  Drupal.acquiaLiftUI.MenuCampaignsView = ViewBase.extend({
    events: {
      'click': 'onClick'
    },

    /**
     * {@inheritdoc}
     */
    initialize: function (options) {
      this.collection = options.collection;
      this.listenTo(this.collection, 'change:isActive', this.render);
    },

    /**
     * {@inheritdoc}
     *
     * @todo: Move count into this view instead of detaching and re-adding.
     */
    render: function () {
      var activeCampaign = this.collection.findWhere({'isActive': true});
      var $count = this.$el.find('i.acquia-lift-personalize-type-count').detach();
      if (!activeCampaign) {
        var label = Drupal.t('All personalizations');
        this.$el.attr('title', label);
      } else {
        var label = Drupal.theme.acquiaLiftSelectedContext({
          'label': activeCampaign.get('label'),
          'append': Drupal.settings.personalize.status[activeCampaign.get('status')],
          'category': Drupal.t('Personalization')
        });
        this.$el.attr('title', activeCampaign.get('label'));
      }
      this.$el.html(label);
      if ($count.length > 0) {
        this.$el.prepend($count);
      }
    },

    /**
     * Responds to clicks.
     *
     * @param jQuery.Event event
     */
    onClick: function (event) {
      event.preventDefault();
      event.stopPropagation();
      return false;
    }
  });

  /**
   * Backbone View/Controller for a single campaigns.
   */
  Drupal.acquiaLiftUI.MenuCampaignView = ViewBase.extend({

    events: {
      'click': 'onClick'
    },

    /**
     * {@inheritdoc}
     */
    initialize: function (options) {
      if (this.model) {
        this.model.on('change:isActive', this.render, this);
        this.model.on('destroy', this.remove, this);
        this.listenTo(this.model, 'change:optionSets', this.render);
        this.listenTo(this.model, 'change:variations', this.render);
      }

      this.build();
      this.render();
    },

    /**
     * {@inheritdoc}
     */
    render: function () {
      var isActive = this.model ? this.model.get('isActive') : false;
      this.$el.toggle(this.model && this.model.includeInNavigation());
      // The menu li element.
      this.$el.toggleClass('acquia-lift-active', isActive);
      // The link element.
      this.$el.find('.acquia-lift-campaign').attr('aria-pressed', isActive);
    },

    /**
     * {@inheritdoc}
     */
    build: function () {
      var html = '';
      if (this.model) {
        html += Drupal.theme('acquiaLiftPersonalizeCampaignMenuItem', {
          link: {
            'id': this.model.get('name'),
            'label': this.model.get('label'),
            'href': this.model.get('links').view
          },
          edit: {
            'href': this.model.get('links').edit
          }
        });
      } else {
        html += Drupal.theme('acquiaLiftPersonalizeNoMenuItem', {
          'type': 'campaigns'
        });
      }
      this.$el.html(html);
    },

    /**
     * {@inheritdoc}
     */
    remove: function () {
      ViewBase.prototype.remove.call(this, true);
    },

    /**
     * Responds to clicks.
     *
     * @param jQuery.Event event
     */
    onClick: function (event) {
      if ($(event.target).hasClass('acquia-lift-campaign')) {
        // @todo Finish styling a throbber for campaign links that fire an
        // ajax event.
        //$(Drupal.theme('acquiaLiftThrobber')).insertAfter(this.$el.find('.acquia-lift-campaign'));
        Drupal.acquiaLiftUI.setActiveCampaignAjax.call(null, this.model.get('name'), this);
        $(document).trigger('acquiaLiftMenuAction');
        event.preventDefault();
        event.stopPropagation();
      }
    }
  });

  /**
   * View to show campaign specific messages within option set or goal lists.
   *
   * The collection property passed in at creation is the collection of all
   * campaigns.
   */
  Drupal.acquiaLiftUI.MenuCampaignMessageView = ViewBase.extend({
    initialize: function (options) {
      this.collection = options.collection;
      this.model = this.collection.findWhere({'isActive': true});

      this.listenTo(this.collection, 'change:isActive', this.onActiveCampaignChange);

      this.build();

      // Set the initial campaign listeners if available.
      this.onActiveCampaignChange();
      this.render();
    },

    /**
     * Listen to changes in the option sets for the active campaign and
     * re-render.
     */
    onActiveCampaignChange: function () {
      this.render();
    },

    build: function () {
      var html = '';
      html += Drupal.theme('acquiaLiftPersonalizeNoEditItem');
      this.$el.html(html);
    },

    render: function () {
      var current = this.collection.findWhere({'isActive': true});
      if (!current) {
        this.$el.hide();
        return;
      }
      this.$el.toggle(!current.get('editable'));
    }
  });

  /***************************************************************
   *
   *            C O N T E N T  V A R I A T I O N S
   *
   ***************************************************************/


  /**
   * View for the top-level content variations menu.
   */
  Drupal.acquiaLiftUI.MenuContentVariationsMenuView = ViewBase.extend({
    events: {
      'click': 'onClick'
    },

    /**
     * {@inheritDoc}
     */
    initialize: function(options) {
      this.campaignCollection = options.campaignCollection;
      this.listenTo(this.campaignCollection, 'change:isActive', this.render);
      this.listenTo(this.campaignCollection, 'change:activeVariation', this.render);
      this.listenTo(this.campaignCollection, 'change:variations', this.render);
      this.$addLink = this.$el.closest('li').find('#acquia-lift-menu-option-set-add');
      this.$addLinkDisabled = '';
      this.build();
      this.render();
    },

    /**
     * {@inheritDoc}
     */
    render: function() {
      var currentCampaign = this.campaignCollection.findWhere({'isActive': true});
      var text = Drupal.t('What');
      var $count = this.$el.find('i.acquia-lift-personalize-type-count').detach();
      if (!currentCampaign) {
        return;
      }
      this.$el.html(text);
      if ($count) {
        this.$el.prepend($count);
      }
      // Enable/disable 'add variation set' option
      var editable = currentCampaign.get('editable');
      this.$addLink.toggle(editable);
      this.$addLinkDisabled.toggle(!editable);
    },

    /**
     * {@inheritDoc}
     */
    build: function() {
      // Add a decoy link for the disabled state of adding a new option set.
      // While ideally we'd just toggle a class, the integration with ctools
      // and drupal ajax links makes this very difficult and messy.
      this.$addLink.after('<a class="' + liftAddMenuClasses + ' acquia-lift-disabled" href="#">' + this.$addLink.text() + '</a>');
      this.$addLinkDisabled = this.$addLink.parent().find('.acquia-lift-disabled');
    },

    /**
     * Responds to clicks.
     *
     * @param jQuery.Event event
     */
    onClick: function (event) {
      event.preventDefault();
      event.stopPropagation();
      return false;
    }
  });

  /**
   * View for all content variation sets for a single campaigns.
   */
  Drupal.acquiaLiftUI.MenuContentVariationsView = ViewBase.extend({

    /**
     * {@inheritdoc}
     */
    initialize: function (options) {
      this.listenTo(this.model, 'change:isActive', this.render);
      this.listenTo(this.model, 'change:optionSets', this.render);
      this.listenTo(this.model, 'change:variations', this.render);
      this.render();
    },

    /**
     * {@inheritdoc}
     */
    render: function () {
      this.$el
        // Toggle visibility of the option set based on the active status of the
        // associated campaign.
        .toggle(this.model.get('isActive'));
    }
  });

  /**
   * Backbone View/Controller for an option set within a campaign.
   */
  Drupal.acquiaLiftUI.MenuOptionSetView = ViewBase.extend({

    events: {
      'click .acquia-lift-preview-option': 'onPreview',
      'click .acquia-lift-variation-add': 'onEdit',
      'click .acquia-lift-variation-edit': 'onEdit'
    },

    /**
     * {@inheritdoc}
     */
    initialize: function (options) {
      this.campaignModel = options.campaignModel;

      var that = this;

      if (this.model) {
        this.listenTo(this.model, 'change', this.render);
        this.listenTo(this.model, 'remove', this.onOptionSetRemoved);
        this.listenTo(this.model, 'change:options', this.rebuild);
        this.listenTo(this.model, 'change:label', this.rebuild);
      }
      this.listenTo(this.campaignModel, 'change:isActive', this.render);

      // Handle menu display changes when an element preview is complete.
      this.onOptionShowProxy = $.proxy(this.onOptionShow, this);
      $(document).on('personalizeOptionChange', function (event, $option_set, choice_name, osid) {
        that.onOptionShowProxy(event, $option_set, choice_name, osid);
      });

      this.rebuild();
    },

    /**
     * When the option set is removed, then update the model reference
     * and rebuild the view.
     */
    onOptionSetRemoved: function() {
      this.model = null;
      this.rebuild();
    },

    /**
     * Regenerates the list HTML and adds to the element.
     * This is necessary when the option set collection changes.
     *
     * @param model
     */
    rebuild: function () {
      this.build();
      this.render();
      // Re-run navbar handling to pick up new menu options.
      _.debounce(Drupal.acquiaLiftUI.utilities.updateNavBar, 300);
    },


    /**
     * {@inheritdoc}
     */
    render: function () {
      this.$el.toggle(this.campaignModel.get('isActive'));
      this.$el
        .find('[data-acquia-lift-personalize-option-set-option]')
        .removeClass('acquia-lift-active')
        .attr('aria-pressed', 'false');
      if (this.model) {
        this.$el
          .find('.acquia-lift-preview-option[data-acquia-lift-personalize-option-set-option="' + this.model.get('activeOption') + '"]')
          .addClass('acquia-lift-active')
          .attr('aria-pressed', 'true');
      }
    },

    /**
     * {@inheritdoc}
     */
    build: function () {
      var html = '';
      if (this.model) {
        html += Drupal.theme('acquiaLiftOptionSetItem', {
          osID: this.model.get('osid'),
          os: this.model.attributes,
          editable: this.campaignModel.get('editable')
        });
      }
      this.$el.html(html);
    },

    /**
     * {@inheritdoc}
     */
    remove: function () {
      $(document).off('personalizeOptionChange', this.onOptionShowProxy);
      ViewBase.prototype.remove.call(this);
    },

    /**
     * Responds to clicks on preview links.
     *
     * @param jQuery.Event event
     */
    onPreview: function (event) {
      if (!$(event.target).hasClass('acquia-lift-preview-option')) return;
      if (!this.model) return;

      var optionId = $(event.target).data('acquia-lift-personalize-option-set-option');
      this.model.set('activeOption', optionId);
      event.preventDefault();
      event.stopPropagation();
    },

    /**
     * Responds to clicks to add or edit an existing elements variation.
     */
    onEdit: function(event) {
      if ($(event.target).hasClass('acquia-lift-disabled')) {
        event.preventDefault();
        event.stopPropagation();
        return false;
      }
      var osData = this.model.get('data');
      var optionId = $(event.target).data('acquia-lift-personalize-option-set-option');
      var data = {
        variationType: osData.personalize_elements_type,
        selector: osData.personalize_elements_selector,
        osid: this.model.get('osid'),
        agentName: this.model.get('agent')
      }
      if (optionId) {
        data.variationIndex = optionId;
        // Set this as the active option for preview as well.
        this.model.set('activeOption', optionId);
      };
      $(document).trigger('acquiaLiftElementVariationEdit', data);
      event.preventDefault();
      event.stopPropagation();
      return false;
    },

    /**
     * Select a specific variation within an option set.
     *
     * @param string osid
     *   The id of the option set to which this choice belongs.
     * @param string choice_name
     *   The option id of the choice to show.
     */
    selectOption: function (osid, choice_name, force) {
      if (this.model && this.model.get('osid') === osid) {
        if (this.model.get('activeOption') === choice_name && force) {
          this.model.trigger('change:activeOption', this.model);
        } else {
          this.model.set('activeOption', choice_name);
        }
      }
    },

    /**
     * Responds to personalizeOptionChange change events.
     *
     * @param jQuery event
     * @param jQuery $option_set
     *   A reference to the jQuery-wrapped option set DOM element.
     * @param string choice_name
     *   The name of the selected choice.
     * @param string osid
     *   The id of the option set to which this choice belongs.
     */
    onOptionShow: function (event, $option_set, choice_name, osid) {
      this.selectOption(osid, choice_name);
    }
  });

  /**
   * View to show when there are no option sets for a campaign.
   *
   * The collection property passed in at creation is the collection of all
   * campaigns.
   */
  Drupal.acquiaLiftUI.MenuOptionSetEmptyView = ViewBase.extend({
    initialize: function (options) {
      this.collection = options.collection;
      this.model = this.collection.findWhere({'isActive': true});

      this.listenTo(this.collection, 'change:isActive', this.onActiveCampaignChange);

      this.build();

      // Set the initial campaign listeners if available.
      this.onActiveCampaignChange();
      this.render();
    },

    /**
     * Listen to changes in the option sets for the active campaign and
     * re-render.
     */
    onActiveCampaignChange: function () {
      if (this.model) {
        this.stopListening(this.model);
      }
      this.model = this.collection.findWhere({'isActive': true});
      if (this.model) {
        this.listenTo(this.model, 'change:optionSets', this.render);
        this.listenTo(this.model, 'change:variations', this.render);
      }
      this.render();
    },

    build: function () {
      var html = '';
      html += Drupal.theme('acquiaLiftPersonalizeNoMenuItem', {
        type: 'variations'
      });
      this.$el.html(html);
    },

    render: function () {
      if (!this.model) {
        this.$el.hide();
        return;
      }
      var numOptions = this.model.get('optionSets').length;
      this.$el.toggle(numOptions == 0);
    }
  });

  /**
   * Display the content variation count for the active campaign.
   */
  Drupal.acquiaLiftUI.MenuContentVariationsCountView = ViewBase.extend({
    /**
     * {@inheritdoc}
     */
    initialize: function (options) {
      this.listenTo(this.model, 'change:isActive', this.render);
      this.listenTo(this.model, 'change:optionSets', this.render);
      this.listenTo(this.model, 'change:variations', this.render);
      this.listenTo(this.model, 'change:activeVariation', this.render);

      this.build();
      this.render();
    },

    /**
     * {@inheritdoc}
     */
    render: function () {
      var variations = this.model.getNumberOfVariations();
      if (this.model.get('isActive')) {
        this.$el
          .toggleClass('acquia-lift-empty', !variations)
          .css('display', 'inline-block')
          .find('span').text(variations);
      } else {
        this.$el.css('display', 'none');
      }
    },

    /**
     * {@inheritdoc}
     */
    build: function() {
      if (this.model) {
        this.$el.attr('id', 'acquia-lift-menu-option-sets-count--' + this.model.get('name'));
      }
    }
  });

  /**
   * A "view" for the variation preview.
   *
   * This does not map to a specific
   * element in the DOM, but rather triggers updates to the view via
   * personalize executors in reaction to changes in the model.
   *
   * It receives the campaign collection as the "collection" in the initialize
   * function.  The model is always set to the currently active campaign.
   */
  Drupal.acquiaLiftUI.MenuVariationPreviewView = ViewBase.extend({
    /**
     * {@inheritDoc}
     */
    initialize: function (options) {
      this.collection = options.collection;

      this.listenTo(this.collection, 'change:isActive', this.onActiveCampaignChange);
      // Call the campaign change function to initialize the first campaign.
      this.onActiveCampaignChange(this.collection.findWhere({'isActive': true}));
    },

    onActiveCampaignChange: function (changed) {
      if (!changed) {
        if (this.model) {
          this.stopListening(this.model);
        }
        this.model = undefined;
        return;
      }
      if (changed.get('isActive')) {
        // Bind to change events from the new model.
        this.model = changed;
        var optionSets = this.model.get('optionSets');
        this.listenTo(this.model, 'change:activeVariation', this.onVariationChange);
        this.listenTo(optionSets, 'change:activeOption', this.onVariationChange);
        this.listenTo(optionSets, 'remove', this.onOptionSetRemove);
      } else {
        this.stopListening(changed);
      }
    },

    onVariationChange: function(changedModel) {
      if (!this.model) {
        return;
      }
      // Note that the model passed into this callback will be the
      // changed option set model.
      if (changedModel instanceof Drupal.acquiaLiftUI.MenuOptionSetModel) {
        var activeOption = changedModel.get('activeOption');
        if (!activeOption) {
          return;
        }
        var current = changedModel.get('options').findWhere({'option_id': activeOption});
        if (!current) {
          return;
        }
        Drupal.personalize.executors[changedModel.get('executor')].execute($(changedModel.get('selector')), current.get('option_id'), changedModel.get('osid'));
      }
    },

    // When an option set is removed, then set the preview back to the control
    // for the now deleted option set.
    onOptionSetRemove: function (removed) {
      Drupal.personalize.executors[removed.get('executor')].execute($(removed.get('selector')), Drupal.settings.personalize.controlOptionName, removed.get('osid'));
    }
  });

  /***************************************************************
   *
   *            G O A L S
   *
   ***************************************************************/

  /**
   * View for the top-level goal menu.
   */
  Drupal.acquiaLiftUI.MenuGoalsMenuView = ViewBase.extend({
    events: {
      'click': 'onClick'
    },

    /**
     * {@inheritDoc}
     */
    initialize: function (options) {
      this.campaignCollection = options.campaignCollection;

      this.listenTo(this.campaignCollection, 'change:isActive', this.render);
      
      this.$addLink = this.$el.closest('li').find('#acquia-lift-menu-goal-add');
      this.$addLinkDisabled = '';
      
      this.build();
      this.render();
    },

    /**
     * {@inheritDoc}
     */
    render: function () {
      var current = this.campaignCollection.findWhere({'isActive': true});
      if (!current) {
        return;
      }
      // Enable/disable the 'add goal' options
      var editable = current.get('editable');
      this.$addLink.toggle(editable);
      this.$addLinkDisabled.toggle(!editable);
    },

    /**
     * {@inheritDoc}
     */
    build: function() {
      // Add a decoy link for the disabled state of adding a new option set.
      // While ideally we'd just toggle a class, the integration with ctools
      // and drupal ajax links makes this very difficult and messy.
      this.$addLink.after('<a class="' + liftAddMenuClasses + ' acquia-lift-disabled" href="#">' + this.$addLink.text() + '</a>');
      this.$addLinkDisabled = this.$addLink.parent().find('.acquia-lift-disabled');
    },

    /**
     * Responds to clicks.
     *
     * @param jQuery.Event event
     */
    onClick: function (event) {
      event.preventDefault();
      event.stopPropagation();
      return false;
    }
  });

  /**
   * Renders the goals for a campaign.
   */
  Drupal.acquiaLiftUI.MenuGoalsView = ViewBase.extend({
    /**
     * {@inheritdoc}
     */
    initialize: function (options) {
      this.listenTo(this.model, 'change:isActive', this.render);
      this.listenTo(this.model, 'change:goals', this.rebuild);
      this.rebuild();
    },

    /**
     * Regenerates the list HTML and adds to the element.
     */
    rebuild: function() {
      this.build();
      this.render();
      // Re-run navbar handling to pick up new menu options.
      _.debounce(Drupal.acquiaLiftUI.utilities.updateNavBar, 300);
    },

    /**
     * {@inheritdoc}
     */
    render: function () {
      this.$el
        // Toggle visibility of the goal set based on the active status of the
        // associated campaign.
        .toggle(this.model.get('isActive'));
    },

    /**
     * {@inheritdoc}
     */
    build: function () {
      var html = Drupal.theme('acquiaLiftCampaignGoals', this.model, Drupal.settings.acquia_lift.customActions, this.model.get('editable'));
      this.$el.html(html);
    }
  });

  /**
   * Displays the number of goals in a campaign.
   */
  Drupal.acquiaLiftUI.MenuGoalsCountView = ViewBase.extend({
    /**
     * {@inheritdoc}
     */
    initialize: function (options) {
      this.model.on('change:goals', this.render, this);
      this.model.on('change:isActive', this.render, this);

      this.build();
      this.render();
    },

    /**
     * {@inheritdoc}
     */
    render: function () {
      var count = this.model.get('goals').length;
      if (this.model.get('isActive')) {
        this.$el
          .toggleClass('acquia-lift-empty', !count)
          .css('display', 'inline-block')
          .find('span').text(count);
      } else {
        this.$el.css('display', 'none');
      }
    },

    /**
     * {@inheritdoc}
     */
    build: function() {
      if (this.model) {
        this.$el.attr('id', 'acquia-lift-menu-goals-count--' + this.model.get('name'));
      }
    }
  });

  /**
   * The "add a goal" link.
   */
  Drupal.acquiaLiftUI.MenuGoalAddView = ViewBase.extend({
    events: {
      'click': 'onClick'
    },

    /**
     * {@inheritDoc}
     */
    initialize: function(options) {
      var that = this;

      this.addLabel = this.$el.text();
      this.onVisitorActionsEditModeProxy = $.proxy(this.onVisitorActionsEditMode, this);
      $(document).on('visitorActionsUIEditMode', function (event, data) {
        that.onVisitorActionsEditModeProxy(event, data);
      });

      // Give the goals model a chance to load and then check for the initial
      // state.
      _.delay(function() {
        var visitorActionsModel = getVisitorActionsAppModel();
        var startingInEdit = visitorActionsModel && visitorActionsModel.get('editMode');
        that.onVisitorActionsEditMode(null, startingInEdit);
      })
    },

    /**
     * {@inheritDoc}
     */
    render: function() {
      var visitorActionsModel = getVisitorActionsAppModel();
      if (visitorActionsModel && visitorActionsModel.get('editMode')) {
        this.$el.text(Drupal.t('Exit goals mode'));
      } else {
        this.$el.text(this.addLabel);
      }
    },

    /**
     * Responds when the visitor actions edit mode is triggered.
     */
    onVisitorActionsEditMode: function(event, editMode) {
      this.render();
      if (editMode) {
        // The next time we click the link we want it to just shut down
        // visitor actions and not open a modal window.
        this.$el.off();
        this.$el.on('click', this.onClick);
        // It has been essentially "unprocessed" so let it get re-processed
        // again later.
        this.$el.removeClass('ctools-use-modal-processed');
      } else {
        // Next time this link is clicked it should open the modal.
        this.$el.off('click', this.onClick);
        Drupal.attachBehaviors(this.$el.parent());
      }
    },

    /**
     * Responds to clicks on the link.
     *
     * If goal selection is currently on, then trigger and event to turn it
     * off - otherwise let the default handling take care of things.
     */
    onClick: function(e) {
      var visitorActionsModel = getVisitorActionsAppModel();
      if (visitorActionsModel && visitorActionsModel.get('editMode')) {
        // Note that sending shutdown here causes a loop of events so
        // we work through a connector toggle process.
        // @see acquia_lift.modal.js
        $(document).trigger('acquiaLiftVisitorActionsConnectorToggle');
      }
      e.preventDefault();
      e.stopImmediatePropagation();
    }
  });

  /***************************************************************
   *
   *            S I N G L E  L I N K  V I E W S
   *
   ***************************************************************/

  /**
   * A view for a single campaign link.
   */
  Drupal.acquiaLiftUI.MenuCampaignLinkView = ViewBase.extend({

    /**
     * {@inheritdoc}
     */
    initialize: function (options) {
      this.collection = options.collection;
      // Make sure we are looking at the element within the menu.
      if (!this.model || this.$el.parents('.acquia-lift-controls').length == 0) {
        return;
      }
      this.model.on('change', this.render, this);
      this.render();
    },

    /**
     * {@inheritdoc}
     */
    render: function () {
      var activeCampaign = this.collection.findWhere({'isActive': true});
      if (!activeCampaign) {
        this.$el
          .find('a[href]')
          .attr('href', '')
          .end()
          .hide();
      }
      else {
        // The link will be empty if this link is not available for this
        // campaign agent type.
        var link = this.getLink(activeCampaign);
        if (link.length == 0) {
          link = 'javascript:void(0);';
          this.$el.find('a[href]').addClass('acquia-lift-menu-disabled');
        } else {
          this.$el.find('a[href]').removeClass('acquia-lift-menu-disabled');
        }
        this.$el
          .find('a[href]')
          .attr('href', link)
          .end()
          .show();
      }
    },

    /**
     * A helper method to return the link for this view.
     *
     * This should be
     * overridden by extending views.
     *
     * @param model
     *   An instance of the current campaign model.
     */
    getLink: function (model) {
      throw("The getLink method not implemented in this view.");
    }
  });

  /**
   * Updates the reports link to reflect the active campaign.
   */
  Drupal.acquiaLiftUI.MenuReportsView = Drupal.acquiaLiftUI.MenuCampaignLinkView.extend({

    /**
     * {@inheritdoc}
     */
    getLink: function (model) {
      return model.get('links').report;
    }
  });

  /**
   * Updates the targeting link to reflect the active campaign.
   */
  Drupal.acquiaLiftUI.MenuTargetingView = Drupal.acquiaLiftUI.MenuCampaignLinkView.extend({

    /**
     * {@inheritdoc}
     */
    getLink: function (model) {
      return model.get('links').targeting;
    }
  });

  /**
   * Updates the scheduling link to reflect the active campaign.
   */
  Drupal.acquiaLiftUI.MenuSchedulingView = Drupal.acquiaLiftUI.MenuCampaignLinkView.extend({

    /**
     * {@inheritdoc}
     */
    getLink: function (model) {
      return model.get('links').scheduling;
    }
  });

  /**
   * Updates the review link to reflect the active campaign.
   */
  Drupal.acquiaLiftUI.MenuReviewView = Drupal.acquiaLiftUI.MenuCampaignLinkView.extend({

    /**
     * {@inheritdoc}
     */
    getLink: function (model) {
      return model.get('links').review;
    }
  });

  /***************************************************************
   *
   *      C O N T E N T  T R I G G E R S / C A N D I D A T E S
   *
   ***************************************************************/

  /**
   * Toggles the 'add content variation' trigger.
   *
   * The model is the variation mode model which keeps track of whether element
   * mode is active or inactive.
   */
  Drupal.acquiaLiftUI.MenuContentVariationTriggerView = ViewBase.extend({

    /**
     * {@inheritdoc}
     */
    initialize: function (options) {
      var that = this;

      this.campaignCollection = options.campaignCollection;

      _.bindAll(this, 'onClick');

      this.listenTo(this.model, 'change:isActive', this.onEditModeChange);
      this.listenTo(this.campaignCollection, 'change:isActive', this.onCampaignChange);

      this.onVariationEditModeProxy = $.proxy(this.onVariationEditMode, this);
      $(document).on('acquiaLiftVariationMode', function (event, data) {
        that.onVariationEditModeProxy(event, data);
      });

      // Set the initial link state based on the campaign type.
      var activeCampaign = this.campaignCollection.findWhere({'isActive': true});
      if (activeCampaign) {
        // Note that this callback will end with a call to render().
        this.onCampaignChange(activeCampaign, true);
      } else {
        this.render();
      }
    },

    /**
     * {@inheritdoc}
     */
    render: function () {
      var isActive = this.model.get('isActive');
      this.$el.toggleClass('acquia-lift-active', isActive);

      if (this.$el.parents('.acquia-lift-controls').length == 0) {
        return;
      }
      // Update the text if within the menu.
      var text = isActive ? Drupal.t('Exit edit mode') : Drupal.t('Add variation set');
      this.$el.text(text);
    },

    onEditModeChange: function () {
      this.updateListeners();
      this.render();
    },

    /**
     * Responds to clicks.
     *
     * @param jQuery.Event event
     */
    onClick: function (event) {
      if (this.model.get('isActive')) {
        this.model.endEditMode();
      } else {
        this.model.startAddMode();
      }
      event.preventDefault();
      event.stopImmediatePropagation();
      this.updateListeners();
      return false;
    },

    /**
     * Change handler when a new campaign is selected.
     *
     * @param model
     *   The campaign model that was selected.
     * @param isActive
     *   The new active status.
     */
    onCampaignChange: function(model, isActive) {
      if (this.model) {
        this.model.endEditMode();
      }
      this.updateListeners();
      this.render(this.model);
    },

    /**
     * Update the event listeners for clicks.
     */
    updateListeners: function () {
      this.$el.off();
      // If the application is in variation mode, the next click should
      // be to exit.
      if (this.model.get('isActive')) {
        this.$el.on('click', this.onClick);
      } else {
        // The next click should open a modal.
        // Remove the -processed flags that CTools adds so that it can be
        // re-processed again.
        this.$el.removeClass('ctools-use-modal-processed');
        Drupal.attachBehaviors(this.$el.parent());
      }
    },

    /**
     * Listens to changes broadcast from the variation application.
     */
    onVariationEditMode: function (event, data) {
      this.model.set('isActive', data.start);
    }
  });

}(Drupal, Drupal.jQuery, _, Backbone));

/**
 * Custom Drupal AJAX commands used for the unified navigation bar tray.
 */
(function(Drupal, $, _) {

  /**
   * Custom AJAX command to preview a specific option set variation.
   *
   * The response should include a data object with the following keys:
   * - agentName: The name of the campaign for this element variation.
   * - osid:  The option set id for the option set to preview.
   * - optionId: The option id to preview.
   */
  Drupal.ajax.prototype.commands.acquia_lift_variation_preview = function (ajax, response, status) {

    function previewVariation() {
      if (!Drupal.acquiaLiftUI.views.optionSets || !Drupal.acquiaLiftUI.views.optionSets[response.data.osid]) {
        return;
      }
      var view = Drupal.acquiaLiftUI.views.optionSets[response.data.osid];
      view.selectOption(response.data.osid, response.data.optionId, true);
    }

    // Allow waiting for only two cycles.
    // @todo: Look into a fully asynchronous solution for this.
    _.defer(function() {
      if (Drupal.acquiaLiftUI.views.optionSets && Drupal.acquiaLiftUI.views.optionSets[response.data.osid]) {
        previewVariation();
      }
      else {
        _.defer(previewVariation);
      }
    });
  };

  /**
   * Custom AJAX command to indicate a deleted element variation.
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
            // Don't delete the data for the option sets in
            // Drupal.settings.personalize.option_sets as we need this in
            // order to go back to the control variation preview.
            for (var option_set_id in Drupal.settings.personalize.option_sets) {
              if (Drupal.settings.personalize.option_sets[option_set_id]['agent'] == empty_agent) {
                Drupal.settings.personalize.option_sets[option_set_id].removed = true;
              }
            }
            Drupal.settings.acquia_lift.campaigns[empty_agent].optionSetTypes = [];
            // Notify of the deleted option sets.
            $(document).trigger('acquiaLiftOptionSetsEmpty', [empty_agent]);
          } else {
            if (option_sets[osid] === 'empty') {
              Drupal.settings.personalize.option_sets[osid].removed = true;
            } else {
              Drupal.settings.personalize.option_sets[osid] = option_sets[osid];
            }
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
      Drupal.settings.acquia_lift.campaigns[campaignId] = campaigns[campaignId];
    }
  }

}(Drupal, Drupal.jQuery, _));

/**
 * Drupal behaviors to initialize the Backbone applications to handle
 * the unified navigation bar tray.
 *
 * This requires the Backbone application classes to be defined first.
 */

(function (Drupal, $, _) {

  var reportPath = Drupal.settings.basePath + Drupal.settings.pathPrefix + 'admin/structure/personalize/manage/acquia-lift-placeholder/report';

  Drupal.behaviors.acquiaLiftPersonalize = {
    attach: function (context) {
      var settings = {
        'option_sets': Drupal.settings.personalize.option_sets,
        'activeCampaign': Drupal.settings.personalize.activeCampaign,
        'campaigns': Drupal.settings.acquia_lift.campaigns
      };
      var liftOptionSetSettings = Drupal.settings.acquia_lift.option_sets || {};
      var ui = Drupal.acquiaLiftUI;
      var addedCampaigns = {};
      var addedOptionSets = {};
      var activeCampaign = '';

      if (settings) {
        // Build models for campaigns that don't have them yet.
        if (!ui.collections.campaigns) {
          ui.collections.campaigns = new ui.MenuCampaignCollection([]);
        }
        Drupal.acquiaLiftUI.utilities.looper(settings.campaigns, function (obj, key) {
          var currentModel = ui.collections.campaigns.findWhere({name: obj.name});
          if (currentModel) {
            for (var prop in obj) {
              if (obj.hasOwnProperty(prop)) {
                currentModel.set(prop, obj[prop]);
              }
            }
          } else {
            var model = new Drupal.acquiaLiftUI.MenuCampaignModel(obj);
            ui.collections.campaigns.add(model);
            addedCampaigns[obj.name] = model;
          }
        });

        // Merging settings' option_sets into campaigns' option_sets.
        Drupal.acquiaLiftUI.utilities.looper(settings.option_sets, function (obj, key) {
          // Add in any Lift-specific details.
          if (liftOptionSetSettings.hasOwnProperty(key)) {
            for (var i in obj.options) {
              var option_id = obj.options[i].option_id;
              if (liftOptionSetSettings[key].hasOwnProperty(option_id)) {
                obj.options[i].editable = liftOptionSetSettings[key][option_id].editable;
                obj.options[i].deletable = liftOptionSetSettings[key][option_id].deletable;
              }
            }
          }
          var campaignModel = ui.collections.campaigns.findWhere({name: obj.agent});
          if (campaignModel) {
            var optionSets = campaignModel.get('optionSets');
            var optionSet = optionSets.findWhere({'osid': key});
            if (obj.hasOwnProperty('removed')) {
              // Remove the option set from its campaign.
              optionSets.remove(optionSet);
            } else {
              // Add the option set collection to the campaign.
              // Merge doesn't work in this case so we need to manually merge.
              if (optionSet) {
                for (var prop in obj) {
                  if (obj.hasOwnProperty(prop)) {
                    optionSet.set(prop, obj[prop]);
                  }
                }
              } else {
                optionSet = new Drupal.acquiaLiftUI.MenuOptionSetModel(obj);
                optionSets.add(optionSet);
                addedOptionSets[obj.osid] = optionSet;
              }
            }
          }
        });

        // Create a model for element variation management state
        if (!ui.models.variationModeModel) {
          ui.models.variationModeModel = new ui.MenuElementVariationModeModel();
        }

        // Create the menu view to handle general show/hide functionality for
        // whole menu sections.
        if (!ui.hasOwnProperty('menuView')) {
          var menu = $('.navbar-tray-acquia-lift', context).get(0);
          ui.menuView = new ui.MenuView({
            collection: ui.collections.campaigns,
            el: menu
          });
        }

        // Initialize the executor preview view functionality.
        if (!ui.views.previewView) {
          ui.views.previewView = new Drupal.acquiaLiftUI.MenuVariationPreviewView({'collection': ui.collections.campaigns});
        }

        // Add the empty campaign variations placeholder.
        if ($('[data-acquia-lift-personalize-type="option_sets"]').length > 0 && !ui.views.emptyVariationsView) {
          var emptyElement = document.createElement('li');
          ui.views.emptyVariationsView = new ui.MenuOptionSetEmptyView({
            el: emptyElement,
            collection: ui.collections.campaigns
          });
          $('[data-acquia-lift-personalize-type="option_sets"]').prepend(ui.views.emptyVariationsView.el);
        }

        // Process the Campaigns, Content Variations and Goals top-level links
        // in the Acquia Lift menu.
        // This is processing that runs only once for the lifetime of the page.
        _.each(['campaigns', 'option_sets', 'goals'], function (category) {
          $('[data-acquia-lift-personalize="' + category + '"]').once('acquia-lift-personalize-menu-controls').each(function (index, item) {
            // Option menus.
            var $link = $(item);
            var type = $link.data('acquia-lift-personalize');
            var $controls = $link.closest('li').removeClass('leaf').addClass('expanded');
            var $element, collection;
            // Load the preview assets.
            if (($controls.once('acquia-lift')).length) {
              $controls.children('ul')
                .addClass(['acquia-lift-' + type.replace('_', '-'), 'menu'].join(' '))
                .attr('data-acquia-lift-personalize-type', type);
            }
            // Create a new ul element to hold the list of items so
            // they can scroll independently of the add link.
            var $menu = $('[data-acquia-lift-personalize-type="' + type + '"]');
            var scrollable = document.createElement('ul');
            scrollable.className += Drupal.settings.acquia_lift.menuClass + " acquia-lift-scrollable";
            $menu.wrap('<div class="menu-wrapper">').before(scrollable);

            // Attach a view that will report the number of campaigns
            // if this link is in the Navbar.
            if ($link.closest('.navbar-tray').length) {
              switch (category) {
                case 'campaigns':
                  collection = ui.collections[category];
                  // Create the view to show the selected name.
                  ui.views.push(new ui.MenuCampaignsView({
                    el: $link,
                    collection: collection
                  }));
                  if (collection.length == 0) {
                    // There are no campaigns.
                    element = document.createElement('li');
                    ui.views.noCampaignsView = new ui.MenuCampaignView({
                      el: element,
                      model: null
                    });
                    $('[data-acquia-lift-personalize-type="campaigns"]').prepend(element);
                  }
                  break;
                case 'option_sets': {
                  Drupal.acquiaLiftUI.views.variationSetsMenuView = new Drupal.acquiaLiftUI.MenuContentVariationsMenuView({
                    campaignCollection: ui.collections.campaigns,
                    el: $link[0]
                  });
                  $link.wrap('<div class="navbar-box">');
                  $link.addClass('navbar-menu-item');
                  break;
                }
                case 'goals': {
                  Drupal.acquiaLiftUI.views.goalsMenuView = new Drupal.acquiaLiftUI.MenuGoalsMenuView({
                    el: $link[0],
                    campaignCollection: ui.collections.campaigns
                  });
                  break;
                }
              }
            }
          });
        });

        // Add the "Add a goal" functionality.
        $('.acquia-lift-goals-new').once('acquia-lift-personalize-menu-add-goal', function() {
          ui.views.push(new ui.MenuGoalAddView({
            el: this
          }))
        });

        // Add an option set count view for each newly added campaign model.
        $('[data-acquia-lift-personalize="option_sets"]').each(function (index, item) {
          var $link = $(item);
          if ($link.closest('.navbar-tray').length) {
            _.each(addedCampaigns, function (campaignModel, key) {
              // Add an empty count for each campaign's set of options.
              var $element = $(Drupal.theme('acquiaLiftCount'));
              ui.views.push((new ui['MenuContentVariationsCountView']({
                el: $element.get(0),
                model: campaignModel
              })));
              $element.prependTo($link);
            });
          }
        });

        // Add a goals count view for each newly added campaign model.
        $('[data-acquia-lift-personalize="goals"]').each(function (index, item) {
          var $link = $(item);
          if ($link.closest('.navbar-tray').length) {
            // Loop through the campaigns and add an empty count for each one.
            _.each(addedCampaigns, function (campaignModel, key) {
              var $element = $(Drupal.theme('acquiaLiftCount'));
              ui.views.push((new ui['MenuGoalsCountView']({
                el: $element.get(0),
                model: campaignModel
              })));
              $element.prependTo($link);
            });
          }
        });

        // Remove any empty campaign views if the campaigns are now populated.
        if (ui.collections.campaigns.length > 0 && ui.views.hasOwnProperty('noCampaignsView')) {
          ui.views.noCampaignsView.remove();
        }

        // Build Views for contents of the Campaigns, Content Variations and Goals
        // top-level links in the Acquia Lift menu.
        _.each(['campaigns', 'option_sets'], function (category) {
          var $typeMenus = $('[data-acquia-lift-personalize-type="' + category + '"]');
          var $scrollable = $typeMenus.siblings('.acquia-lift-scrollable');
          if ($typeMenus.length) {
            $typeMenus
              .each(function (index, element) {
                var $menu = $(element);
                var type = $menu.data('acquia-lift-personalize-type');
                var model, element, campaignName, campaignModel, optionSets;
                var $holder = $scrollable.length > 0 ? $scrollable : $menu;
                Drupal.acquiaLiftUI.utilities.looper(settings[type], function (obj, key) {
                  // Find the right model.
                  if (type === 'campaigns') {
                    // If the menu already has a link for this setting, abort.
                    if (!$menu.find('[data-acquia-lift-personalize-agent="' + key + '"].acquia-lift-campaign').length) {
                      campaignName = key;
                      campaignModel = model = ui.collections[type].findWhere({'name': key});
                    }
                  }
                  // Create views for the campaign model if it was just added.
                  if (model && addedCampaigns.hasOwnProperty(campaignName)) {
                    element = document.createElement('li');
                    if (type === 'campaigns') {
                      // Add campaign view.
                      ui.views.push(new ui.MenuCampaignView({
                        el: element,
                        model: model
                      }));
                    }

                    $holder.prepend(element);

                    // Build a view for campaign goals.
                    if (type === 'campaigns') {
                      var $goalsMenu = $('[data-acquia-lift-personalize-type="goals"]');
                      element = document.createElement('li');
                      var goalsView = new ui.MenuGoalsView({
                        model: model,
                        el: element
                      });
                      var $goalsScrollable = $goalsMenu.siblings('.acquia-lift-scrollable');
                      var $goalsMenuList = $goalsScrollable.length > 0 ? $goalsScrollable : $goalsMenu;
                      ui.views.push(goalsView);
                      $goalsMenuList.prepend(goalsView.el);
                    }
                  }
                });

                if (category === 'option_sets') {
                  // Add any new option sets.
                  Drupal.acquiaLiftUI.utilities.looper(addedOptionSets, function (model, osid) {
                    if (!Drupal.acquiaLiftUI.views.optionSets[osid]) {
                      campaignModel = ui.collections.campaigns.findWhere({'name': model.get('agent')});
                      element = document.createElement('li');
                      var view = new Drupal.acquiaLiftUI.MenuOptionSetView({
                        campaignModel: campaignModel,
                        model: model,
                        el: element
                      });
                      Drupal.acquiaLiftUI.views.optionSets[model.get('osid')] = view;
                      ui.views.push(view);
                      $holder.prepend(view.el);
                    }
                  });
                }
              });
          }
        });

        // Build Views for all content variations within the campaign.
        var $contentVariations = $('[data-acquia-lift-personalize-agent].acquia-lift-content-variation').once('acquia-lift-personalize-option-sets');
        if ($contentVariations.length) {
          $contentVariations
            .each(function (index, element) {
              var $group = $(element);
              var campaign = $group.data('acquia-lift-personalize-agent');
              var model = ui.collections['campaigns'].findWhere({'name': campaign});
              ui.views.push((new ui.MenuContentVariationsView({
                el: $group.closest('li').get(0),
                model: model
              })));
            });
        }

        // Create views for single campaign link menu items.
        var linkTypes = {
          'reports': ui.MenuReportsView,
          'targeting': ui.MenuTargetingView,
          'scheduling': ui.MenuSchedulingView,
          'review': ui.MenuReviewView
        };

        for (var linkType in linkTypes) {
          if (ui.collections.campaigns.length > 0) {
            $('[data-acquia-lift-personalize="' + linkType + '"]')
              .once('acquia-lift-personalize-' + linkType)
              .each(function (index, element) {
                ui.views.push((new linkTypes[linkType]({
                  el: element.parentNode,
                  model: ui.collections['campaigns'],
                  collection: ui.collections['campaigns']
                })));
              });
          } else {
            $('[data-acquia-lift-personalize="' + linkType + '"]').hide();
          }
        }

        // Add the message placeholders.
        if ($('[data-acquia-lift-personalize-type="option_sets"]').length > 0 && !ui.views.messageOptionSetsView) {
          var messageElement = document.createElement('li');
          ui.views.messageOptionSetsView = new ui.MenuCampaignMessageView({
            el: messageElement,
            collection: ui.collections.campaigns
          });
          $('[data-acquia-lift-personalize-type="option_sets"]').closest('li').find('.acquia-lift-scrollable').prepend(ui.views.messageOptionSetsView.el);
        }
        if ($('[data-acquia-lift-personalize-type="goals"]').length > 0 && !ui.views.messageGoalsView) {
          var messageElement = document.createElement('li');
          ui.views.messageGoalsView = new ui.MenuCampaignMessageView({
            el: messageElement,
            collection: ui.collections.campaigns
          });
          $('[data-acquia-lift-personalize-type="goals"]').closest('li').find('.acquia-lift-scrollable').prepend(ui.views.messageGoalsView.el);
        }

        // Refresh event delegation. This is necessary to rebind event delegation
        // to HTML that's been moved inside a jQuery dialog.
        _.each(ui.views, function (view) {
          view.delegateEvents();
        });
        // Refresh the model data for option sets.
        ui.collections.campaigns.each(function (campaignModel) {
          campaignModel.refreshData();
        });
        if (!ui.collections['campaigns']) {
          return;
        }

        // Update the active campaign.
        // If it was just added and is set as the active campaign then it takes
        // priority over a campaign that was previously set as active.
        if (addedCampaigns.hasOwnProperty(settings.activeCampaign)) {
          activeCampaign = settings.activeCampaign;
        } else {
          // Use the current if set, otherwise read from settings.
          var current = ui.collections['campaigns'].findWhere({'isActive': true});
          if (current) {
            activeCampaign = current.get('name');
          } else {
            activeCampaign = settings.activeCampaign;
          }
        }
        // Make sure the activeCampaign requested is available on this page.
        var current = ui.collections['campaigns'].findWhere({'name': activeCampaign});
        if (!current || !current.includeInNavigation()) {
          activeCampaign = '';
        }

        Drupal.acquiaLiftUI.setActiveCampaign(activeCampaign);
        Drupal.acquiaLiftUI.utilities.setInitialized(true);
        Drupal.acquiaLiftUI.utilities.updateNavbar();
      }
    }
  };

  Drupal.behaviors.acquiaLiftUnibarListeners = {
    attach: function (context) {
      $('body').once('acquia-lift-unibar-listeners', function () {

        // Generate a place-holder element to handle the Lift settings updates
        // via Drupal's AJAX handling.  This ensures that theme styles can be
        // limited to those already on the page as well as automatically
        // handling Drupal commands upon return.
        var settingsElement = document.createElement('div');
        var elementId = settingsElement.id = 'acquia-lift-settings-' + new Date().getTime();
        $('body').append(settingsElement);

        Drupal.ajax[elementId] = new Drupal.ajax(elementId, settingsElement, {
          url: Drupal.settings.basePath + Drupal.settings.pathPrefix + 'acquia_lift/settings',
          event: 'acquiaLiftSettingsUpdate',
          progress: {
            type: '',
            message: ''
          },
          success: function (response, status) {
            Drupal.ajax.prototype.success.call(this, response, status);
            Drupal.attachBehaviors(settingsElement);
          }
        });

        // Each time the queue synchronization is complete it means that
        // the status could have changed for a particular campaign.
        $(document).bind('acquiaLiftQueueSyncComplete', function () {
          // Trigger the event that will load from the Drupal AJAX object
          // created above.
          $('#' + elementId).trigger('acquiaLiftSettingsUpdate');
        });
      })
    }
  };

  Drupal.behaviors.acquiaLiftContentVariations = {
    attach: function (context) {
      var ui = Drupal.acquiaLiftUI;
      // Create a model for element variation management state
      if (!ui.models.variationModeModel) {
        ui.models.variationModeModel = new ui.MenuElementVariationModeModel();
      }

      // Keep the element variation editing and in-context goal creation in
      // mutually exclusive active states.
      $('body').once('acquia-lift-personalize', function () {
        // Creating any item from the menu is considering starting a new menu action.
        $('.acquia-lift-menu-create').once().each(function() {
          $(this).on('click', function () {
            $(document).trigger('acquiaLiftMenuAction');
          })
        });
        // Shut down goals editing if a new menu action is started.
        $(document).on('acquiaLiftMenuAction', function () {
          _.defer(function() {
            Drupal.acquiaLiftUI.utilities.shutDownGoalsUI();
          });
        });

        // Turn off content variations highlighting if visitor actions editing
        // is enabled.
        $(document).bind('visitorActionsUIEditMode', function (event, isActive) {
          if (isActive) {
            // Prevent infinite loops of updating models triggering change events
            // by delaying this update to the next evaluation cycle.
            _.delay(function () {
              ui.models.variationModeModel.endEditMode();
            });
          }
        });
        // Turn off visitor actions modes when entering variation mode.
        $(document).bind('acquiaLiftVariationMode', function (event, data) {
          if (data.start) {
            _.delay(function() {
              Drupal.acquiaLiftUI.utilities.shutDownGoalsUI();
            });
          }
        });
      });

      // Build Views for the Add Content Variation triggers.
      $('[data-acquia-lift-personalize-mode="content-variation"]')
        .once('acquia-lift-personalize-trigger')
        .each(function (index, element) {
          ui.views.push((new ui.MenuContentVariationTriggerView({
            el: element,
            model: ui.models.variationModeModel,
            campaignCollection: ui.collections.campaigns
          })));
        });
    }
  };

}(Drupal, Drupal.jQuery, _));

//# sourceMappingURL=acquia_lift.personalize.js.map