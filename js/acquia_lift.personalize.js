/**
 * @file
 * acquia_lift.personalize.js
 */
(function (Drupal, $, _, Backbone) {
// Removes leading '/', '?' and '#' characters from a string.
var pathRegex = /^(?:[\/\?\#])*(.*)/;

var reportPath = '/admin/structure/personalize/manage/acquia-lift-placeholder/report';
var statusPath = '/admin/structure/personalize/manage/acquia-lift-placeholder/status';

Drupal.behaviors.acquiaLiftPersonalize = {
  attach: function (context) {
    var settings = Drupal.settings.personalize;
    var ui = Drupal.acquiaLiftUI;
    if (settings) {
      // Build models for menus that don't have them yet.
      if (!ui.collections['campaigns']) {
        ui.collections['campaigns'] = new ui.MenuCampaignCollection([]);
      }
      looper(settings.campaigns, function (obj, key) {
        if (!ui.collections['campaigns'].findWhere({name: obj.name})) {
          ui.collections['campaigns'].add(obj);
        }
      });
      looper(settings.option_sets, function (obj, key) {
        var campaign = obj.agent;
        if (!ui.collections['option_sets'][campaign]) {
          ui.collections['option_sets'][campaign] = new Backbone.Collection([], {
            model: ui[ui.objectMap['option_sets'] + 'Model']
          });
        }
        if (!ui.collections['option_sets'][campaign].findWhere({osid: obj.osid})) {
          ui.collections['option_sets'][campaign].add(obj);
        }
      });

      // Create the menu view to handle general show/hide functionality for
      // whole menu sections.
      if (!ui.hasOwnProperty('menuView')) {
        var menu = $('.navbar-tray-acquia-lift', context).get(0);
        ui.menuView = new ui.MenuView({
          collection: ui.collections.campaigns,
          el: menu
        });
      }

      // Process the Campaigns, Content Variations and Goals top-level links
      // in the Acquia Lift menu.
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
          // Attach a view to each link that will report the number of items
          // under that category if this link is in the Navbar.
          if ($link.closest('.navbar-tray').length) {
            switch (category) {
              case 'campaigns':
                collection = ui.collections[category];
                $element = $(Drupal.theme('aquiaLiftCount'));
                if (collection && collection.length > 0) {
                  ui.views.push((new ui[ui.objectMap[category] + 'CountView']({
                    el: $element.get(0),
                    model: collection
                  })));
                  // Create the view to show the selected name.
                  ui.views.push(new ui.MenuCampaignsView({
                    el: $link,
                    collection: collection
                  }));
                } else {
                  // There are no campaigns.
                  element = document.createElement('li');
                  ui.views.push((new ui.MenuCampaignView({
                    el: element,
                    model: null
                  })));
                  $('[data-acquia-lift-personalize-type="campaigns"]').prepend(element);
                }
                $element.prependTo($link);
                break;
              case 'option_sets':
                // Loop through the campaigns and add an empty count for each one.
                if (ui.collections.campaigns && ui.collections.campaigns.length > 0) {
                  _.each(ui.collections.campaigns.models, function (campaignModel) {
                    $element = $(Drupal.theme('aquiaLiftCount'));
                    ui.views.push((new ui[ui.objectMap[category] + 'CountEmptyView']({
                      el: $element.get(0),
                      model: campaignModel,
                      optionSetCollections: ui.collections[category]
                    })));
                    $element.prependTo($link);
                  });
                }
                // Loop through the option set models, keyed by campaign, and
                // create a view for each.
                looper(ui.collections[category], function (collection, campaign) {
                  $element = $(Drupal.theme('aquiaLiftCount'));
                  ui.views.push((new ui[ui.objectMap[category] + 'CountView']({
                    el: $element.get(0),
                    model: collection,
                    campaignModel: ui.collections.campaigns.findWhere({name: campaign})
                  })));
                  $element.prependTo($link);
                });
                break;
              case 'goals':
                // Loop through the campaigns and add an empty count for each one.
                if (ui.collections.campaigns && ui.collections.campaigns.length > 0) {
                  _.each(ui.collections.campaigns.models, function (campaignModel) {
                    $element = $(Drupal.theme('aquiaLiftCount'));
                    ui.views.push((new ui[ui.objectMap[category] + 'CountView']({
                      el: $element.get(0),
                      model: campaignModel
                    })));
                    $element.prependTo($link);
                  });
                }
                break;
            }
          }
        });
      });

      // Build Views for contents of the Campaigns, Content Variations and Goals
      // top-level links in the Acquia Lift menu.
      _.each(['campaigns', 'option_sets'], function (category) {
        var $typeMenus = $('[data-acquia-lift-personalize-type="' + category + '"]').once('acquia-lift-personalize-menu-contents');
        var campaignsWithOptions = {};
        if ($typeMenus.length) {
          $typeMenus
            .each(function (index, element) {
              var $menu = $(element);
              var type = $menu.data('acquia-lift-personalize-type');
              var model, element;
                looper(settings[type], function (obj, key) {
                // Find the right model.
                switch (type) {
                  case 'option_sets':
                    // If the menu already has a link for this setting, abort.
                    if (!$menu.find('[data-acquia-lift-personalize-agent="' + obj.agent + '"][data-acquia-lift-personalize-id="' + key + '"].acquia-lift-preview-option-set').length) {
                      var campaign = obj.agent;
                      campaignsWithOptions[obj.agent] = obj.agent;
                      model = ui.collections[type][campaign].findWhere({'osid': key});
                    }
                    break;
                  case 'campaigns':
                      // If the menu already has a link for this setting, abort.
                    if (!$menu.find('[data-acquia-lift-personalize-agent="' + key + '"].acquia-lift-campaign').length) {
                      model = ui.collections[type].findWhere({'name': key});
                    }
                    break;
                }
                // Create a model for the menu item.
                if (model) {
                  element = document.createElement('li');
                  ui.views.push((new ui[ui.objectMap[type] + 'View']({
                    el: element,
                    model: model
                  })));
                  $menu.prepend(element);
                  // Build a view for campaign goals.
                  if (type === 'campaigns') {
                    var $goalsMenu = $('[data-acquia-lift-personalize-type="goals"]');
                    var modelGoals = model.get('goals');
                    if (modelGoals) {
                      looper(modelGoals, function (obj, key) {
                        element = document.createElement('li');
                        ui.views.push((new ui[ui.objectMap['goals'] + 'View']({
                          el: element,
                          model: model,
                          goalID: key,
                          goalLabel: obj
                        })));
                        $goalsMenu.prepend(element);
                      });
                    } else {
                      // Build an empty campaign goal view.
                      element = document.createElement('li');
                      ui.views.push((new ui[ui.objectMap['goals'] + 'View']({
                        el: element,
                        model: model,
                        goalID: null,
                        goalLabel: null
                      })));
                      $goalsMenu.prepend(element);
                    }
                  }
                }
              });

              if (category === 'option_sets') {
                looper(settings.campaigns, function (obj, key) {
                  if (!campaignsWithOptions.hasOwnProperty(key)) {
                    // Add a view to indicate that there are no option sets for
                    // a campaign.
                    model = ui.collections.campaigns.findWhere({'name': key});
                    element = document.createElement('li');
                    ui.views.push((new ui.MenuOptionEmptyView({
                      el: element,
                      model: model
                    })));
                    $menu.prepend(element);
                  }
                })
              }
            });
          // If Navbar module is enabled, process the links.
          if (Drupal.behaviors.acquiaLiftNavbarMenu) {
            Drupal.behaviors.acquiaLiftNavbarMenu.attach();
          }
        }
      });

      // Build Views for the Option Set groups.
      var $optionSetGroups = $('[data-acquia-lift-personalize-agent].acquia-lift-preview-option-set').once('acquia-lift-personalize-option-sets');
      if ($optionSetGroups.length) {
        $optionSetGroups
          .each(function (index, element) {
            var $group = $(element);
            var campaign = $group.data('acquia-lift-personalize-agent');
            var model = ui.collections['campaigns'].findWhere({'name': campaign});
            ui.views.push((new ui.MenuOptionSetView({
              el: $group.closest('li').get(0),
              model: model
            })));
          });
        // If Navbar module is enabled, process the links.
        if (Drupal.behaviors.acquiaLiftNavbarMenu) {
          Drupal.behaviors.acquiaLiftNavbarMenu.attach();
        }
      }

      // Create View for the Report link.
      if (ui.collections.campaigns) {
        $('[href="' + reportPath + '"]')
          .once('acquia-lift-personalize-report')
          .each(function (index, element) {
            ui.views.push((new ui.MenuReportsView({
              el: element.parentNode,
              model: ui.collections['campaigns'],
              collection: ui.collections['campaigns']
            })));
          });
      } else {
        $('[href="' + reportPath + '"]').hide();
      }

      // Create a View for the Status link.
      if (ui.collections.campaigns) {
        $('[href="' + statusPath + '"]')
          .once('acquia-lift-personalize-status')
          .each(function (index, element) {
            ui.views.push(new ui.MenuStatusView({
              el: element.parentNode,
              model: ui.collections['campaigns'],
              collection: ui.collections['campaigns']
            }));
          });
      } else {
        $('[href="' + statusPath + '"]').hide();
      }

      // Refresh event delegation. This is necessary to rebind event delegation
      // to HTML that's been moved inside a jQuery dialog.
      _.each(ui.views, function (view) {
        view.delegateEvents();
      });
      // Refresh the model data for option sets.
      looper(settings['option_sets'], function (obj, key) {
        // Find the right model.
        var campaign = obj.agent;
        var model = ui.collections['option_sets'][campaign].findWhere({'osid': key});
        if (!model) {
          return;
        }
        // If the activeOption has not been set, set it to a default.
        if (!model.get('activeOption')) {
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
          } else if (obj.winner !== null && obj.winner !== undefined) {
            // Otherwise a winner should be the default if one has been defined.
            index = obj.winner;
          }
          // The first option key isn't always 0.
          if (!obj.options.hasOwnProperty(index)) {
            var keys = _.keys(obj.options);
            keys.sort;
            index = keys[0];
          }
          model.set('activeOption', obj.options[index].option_id);
        }
      });
      if (!ui.collections['campaigns']) {
        return;
      }
      var activeCampaign = ui.collections['campaigns'].findWhere({'isActive': true}) || settings.activeCampaign;
      Drupal.acquiaLiftUI.setActiveCampaign(activeCampaign);
    }
  }
};

Drupal.behaviors.acquiaLiftContentVariations = {
  attach: function (context) {
    var settings = Drupal.settings.personalize;
    var ui = Drupal.acquiaLiftUI;
    // Create a model for a Content Variation management state.
    if (!ui.models.contentVariationModel) {
      ui.models.contentVariationModel = new ui.MenuContentVariationModel();
    }

    // Keep the in-context content variation editing and in-context goal
    // creation in mutually exclusive active states.
    $('body').once('acquia-lift-personalize', function () {
      // Turn off content variations highlighting if visitor actions editing
      // is enabled.
      $(document).bind('visitorActionsUIEditMode', function (event, isActive) {
        if (isActive) {
          // Prevent infinite loops of updating models triggering change events
          // by delaying this update to the next evaluation cycle.
          window.setTimeout(function () {
            ui.models.contentVariationModel.set('isActive', false);
          }, 0);
        }
      });
      // Signal when content variation highlighting is active.
      ui.models.contentVariationModel.on('change:isActive', function (model, isActive) {
        if (isActive) {
          $(document).trigger('visitorActionsUIShutdown');
        }
      });
    });

    // Build Views for the Add Content Variation triggers.
    $('[data-acquia-lift-personalize-mode="content-variation"]')
      .once('acquia-lift-personalize-trigger')
      .each(function (index, element) {
        ui.views.push((new ui.MenuAddContentVariationTriggerView({
          el: element,
          model: ui.models.contentVariationModel
        })));
      });
    // Find content variation candidates.
    $('[data-personalize-entity-id]')
      .once('acquia-lift-personalize-variation-candidate')
      .each(function (index, element) {
        ui.views.push((new ui.MenuContentVariationCandidateView({
          el: element,
          model: ui.models.contentVariationModel
        })));
      });
  }
};

Drupal.acquiaLiftUI = Drupal.acquiaLiftUI || {};
Drupal.acquiaLiftUI.views = Drupal.acquiaLiftUI.views || [];
Drupal.acquiaLiftUI.models = Drupal.acquiaLiftUI.models || {};
Drupal.acquiaLiftUI.collections = Drupal.acquiaLiftUI.collections || {};
Drupal.acquiaLiftUI.collections['option_sets'] = Drupal.acquiaLiftUI.collections['option_sets'] || {};

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

$.extend(Drupal.acquiaLiftUI, {
  // Common View/Model name components.
  objectMap: {
    option_sets: 'MenuOption',
    campaigns: 'MenuCampaign',
    goals: 'MenuGoals'
  },

  /**
   * Finds the model of the active campaign sets isActive to true.
   *
   * @param string activeCampaign
   *   The name of the active campaign.
   */
  setActiveCampaign: function (activeCampaign) {
    // Refresh the model data for the campaigns.
    looper(Drupal.settings.personalize['campaigns'] || {}, function (obj, key) {
      if (key === activeCampaign) {
        Drupal.acquiaLiftUI.collections['campaigns'].findWhere({'name': key}).set('isActive', true);
      }
    });
  },

  /**
   * Updates the active campaign session variable on the server.
   *
   * @param string name
   *   The name of the selected campaign.
   */
  setActiveCampaignAjax: function (name, view) {
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
  },

  /**
   * The model for a campaign.
   */
  MenuCampaignModel: Backbone.Model.extend({
    defaults: {
      label: '',
      links: {},
      name: '',
      isActive: false,
      verified: false
    },

    /**
     * Updates the status of a campaign.
     *
     * @param newStatus
     *   The new status value for the campaign.
     */
    updateStatus: function (newStatus) {
      var updateUrl = '/admin/structure/personalize/manage/' + this.get('name') + '/ajax_status/' + newStatus;
      var model = this;
      $.getJSON( updateUrl, function( data ) {
        if (data.success) {
          // Update the model current and next status values.
          model.set('status', data.currentStatus);
          model.set('nextStatus', data.nextStatus);

          // We also need to update the status value of the campaign in the
          // Drupal.setttings object.
          // @todo: Make this an event dispatch that is handled outside of this
          // application scope.
          // Leaving for now since the reliance on drupal settings is all over
          // the application so it's not horrible.
          Drupal.settings.personalize.campaigns[model.get('name')].status = data.currentStatus;
        }
      });
    }
  }),

  /**
   * The model for a menu of option set links.
   */
  MenuOptionModel: Backbone.Model.extend({
    defaults: {
      name: '',
      agent: '',
      agent_info: {},
      decision_name: '',
      executor: 'show',
      label: '',
      mvt: '',
      option_names: [],
      options: [],
      activeOption: null,
      osid: '',
      stateful: 1,
      type: null
    }
  }),

  /**
   * The Model for a 'add content variation' state.
   */
  MenuContentVariationModel: Backbone.Model.extend({
    defaults: {
      isActive: false
    }
  }),

  /**
   * View for an option set (which contains options).
   */
  MenuOptionSetView: ViewBase.extend({

    /**
     * {@inheritdoc}
     */
    initialize: function (options) {
      this.model.on('change:isActive', this.render, this);
      this.render(this.model, this.model.get('isActive'));
    },

    /**
     * {@inheritdoc}
     */
    render: function (model, isActive) {
      this.$el
        // Toggle visibility of the option set based on the active status of the
        // associated campaign.
        .toggle(isActive);
    }
  }),

  /**
   * View to show when there are no option sets for a campaign.
   */
  MenuOptionEmptyView: ViewBase.extend({
    initialize: function (options) {
      this.model.on('change:isActive', this.render, this);

      this.build();
      this.render(this.model, this.model.get('isActive'));
    },

    build: function () {
      var html = '';
      html += Drupal.theme('acquiaLiftPersonalizeNoMenuItem', {
        type: 'variation sets'
      });
      this.$el.html(html);
    },

    render: function(model, isActive) {
      this.$el.toggle(isActive);
    }
  }),

  /**
   * Backbone View/Controller for an option within an option set.
   */
  MenuOptionView: ViewBase.extend({

    events: {
      'click .acquia-lift-preview-option': 'onClick'
    },

    /**
     * {@inheritdoc}
     */
    initialize: function (options) {
      this.model.on('change', this.render, this);
      this.model.on('destroy', this.remove, this);

      this.onOptionShowProxy = $.proxy(this.onOptionShow, this);
      $(document).on('personalizeOptionChange', this.onOptionShowProxy);

      this.build(this.model);
      this.render(this.model);
    },

    /**
     * {@inheritdoc}
     */
    render: function (model) {
      this.$el
        .find('[data-acquia-lift-personalize-option-set-option]')
        .removeClass('acquia-lift-active')
        .attr('aria-pressed', 'false');
      this.$el
        .find('[data-acquia-lift-personalize-option-set-option="' + model.get('activeOption') + '"]')
        .addClass('acquia-lift-active')
        .attr('aria-pressed', 'true');
    },

    /**
     * {@inheritdoc}
     */
    build: function (model) {
      var html = '';
      html += Drupal.theme('acquiaLiftOptionSetItem', {
        osID: model.get('osid'),
        os: model.attributes
      });
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
     * Responds to clicks.
     *
     * @param jQuery.Event event
     */
    onClick: function (event) {
      if (!$(event.target).hasClass('acquia-lift-preview-option')) {
        return;
      }
      var optionid = $(event.target).data('acquia-lift-personalize-option-set-option');
      var selector = $(event.target).data('acquia-lift-personalize-option-set-selector');
      var osid = this.model.get('osid');

      // Swaps the current option in an option set for the indicated option.
      Drupal.personalize.executors[this.model.get('executor')].execute($(selector), optionid, this.model.get('osid'), true);
      event.preventDefault();
      event.stopPropagation();
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
      if (this.model.get('osid') === osid) {
        this.model.set('activeOption', choice_name);
      }
    }
  }),

  /**
   * View/controller for full menu list.
   */
  MenuView: ViewBase.extend({

    /**
     * {@inheritdoc}
     */
    initialize: function (options) {
      // The campaign collection.
      this.collection = options.collection;
      this.collection.on('change', this.render, this);
      this.render();
    },

    /**
     * {@inheritdoc}
     */
    render: function() {
      var hasCampaigns = this.collection.length > 0;
      var activeCampaign = this.collection.findWhere({'isActive': true});
      // Show or hide relevant menus.
      if (hasCampaigns && activeCampaign) {
        this.$el.find('[data-acquia-lift-personalize="goals"]').parents('li').show();
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
  }),

  /**
   * View/controller for the campaign menu header.
   */
  MenuCampaignsView: ViewBase.extend({

    /**
     * {@inheritdoc}
     */
    initialize: function (options) {
      this.collection = options.collection;
      this.collection.on('change', this.render, this);
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
        var label = Drupal.t('All campaigns');
      } else {
        var label = Drupal.theme.acquiaLiftCampaignContext({'label': activeCampaign.get('label')});
      }
      this.$el.html(label);
      if ($count.length > 0) {
        this.$el.prepend($count);
      }
    }
  }),

  /**
   * Backbone View/Controller for a single campaigns.
   */
  MenuCampaignView: ViewBase.extend({

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
      }

      this.build(this.model);
      this.render(this.model, this.model ? this.model.get('isActive') : false);
    },

    /**
     * {@inheritdoc}
     */
    render: function (model, isActive) {
      // The menu li element.
      this.$el.toggleClass('acquia-lift-active', isActive);
      // The link element.
      this.$el.find('.acquia-lift-campaign').attr('aria-pressed', isActive);
    },

    /**
     * {@inheritdoc}
     */
    build: function (model) {
      var html = '';
      if (model) {
        html += Drupal.theme('acquiaLiftPersonalizeCampaignMenuItem', {
          link: {
            'id': model.get('name'),
            'label': model.get('label'),
            'href': model.get('links').view
          },
          edit: {
            'href': model.get('links').edit
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
        event.preventDefault();
        event.stopPropagation();
      }
    }
  }),

  /**
   * Renders a campaign goal item.
   */
  MenuGoalsView: ViewBase.extend({

    /**
     * {@inheritdoc}
     */
    initialize: function (options) {

      this.goalID = options.goalID;
      this.goalLabel = options.goalLabel;

      this.model.on('change:isActive', this.render, this);

      this.build(this.model);
      this.render(this.model, this.model.get('isActive'));
    },

    /**
     * {@inheritdoc}
     */
    render: function (model, isActive) {
      this.$el
        // Toggle visibility of the goal set based on the active status of the
        // associated campaign.
        .toggle(isActive);
    },

    /**
     * {@inheritdoc}
     */
    build: function (model) {
      var html = '';
      if (this.goalID) {
        html += Drupal.theme('acquiaLiftPersonalizeGoal', {
          campaignID: model.get('name'),
          name: this.goalID,
          label: this.goalLabel
        });
      } else {
        html += Drupal.theme('acquiaLiftPersonalizeNoMenuItem', {
          type: 'goals'
        });
      }
      this.$el.html(html);
    }
  }),

  /**
   * Display the count of campaigns.
   */
  MenuCampaignCountView: ViewBase.extend({
    /**
     * {@inheritdoc}
     */
    initialize: function (options) {
      this.model.on('add', this.render, this);
      this.model.on('remove', this.render, this);

      this.render(this.model);
    },

    /**
     * {@inheritdoc}
     */
    render: function () {
      var count = this.model.length;
      this.$el.toggleClass('acquia-lift-empty', !count);
      this.$el.find('span').text(this.model.length);
    }
  }),

  /**
   * Display the content variation count for the active campaign.
   */
  MenuOptionCountView: ViewBase.extend({
    /**
     * {@inheritdoc}
     */
    initialize: function (options) {

      this.campaignModel = options.campaignModel;

      this.model.on('add', this.render, this);
      this.model.on('remove', this.render, this);
      this.campaignModel.on('change:isActive', this.render, this);

      this.render();
    },

    /**
     * {@inheritdoc}
     */
    render: function () {
      this.$el
        .toggle(this.campaignModel.get('isActive'))
        .toggleClass('acquia-lift-empty', !this.model.length)
        .find('span').text(this.model.length);
    }
  }),

  /**
   * Listens to changes in a Campaign model; renders a zero count on the
   * Content Variations link.
   */
  MenuOptionCountEmptyView: ViewBase.extend({
    /**
     * {@inheritdoc}
     */
    initialize: function (options) {
      this.optionSetCollections = options.optionSetCollections;

      this.model.on('change:isActive', this.render, this);

      this.render();
    },

    /**
     * {@inheritdoc}
     */
    render: function () {
      var isActive = this.model.get('isActive');
      var name = this.model.get('name');
      var isEmpty = !(this.optionSetCollections && this.optionSetCollections[name] && this.optionSetCollections[name].length);
      this.$el.toggle(isActive && isEmpty);
      this.$el.toggleClass('acquia-lift-empty', isEmpty);
    }
  }),

  /**
   * Displays the number of goals in a campaign.
   */
  MenuGoalsCountView: ViewBase.extend({
    /**
     * {@inheritdoc}
     */
    initialize: function (options) {
      this.model.on('change:goals', this.render, this);
      this.model.on('change:isActive', this.render, this);

      this.render(this.model);
    },

    /**
     * {@inheritdoc}
     */
    render: function (model) {
      var count = size(model.get('goals'));
      this.$el
        .toggle(model.get('isActive'))
        .toggleClass('acquia-lift-empty', !count)
        .find('span').text(count);

    }
  }),

  /**
   * Updates the results link to reflect the active campaign.
   */
  MenuReportsView: ViewBase.extend({

    /**
     * {@inheritdoc}
     */
    initialize: function (options) {
      this.collection = options.collection;
      if (!this.model) {
        return;
      }
      this.model.on('change', this.render, this);
      this.render(this.model);
    },

    /**
     * {@inheritdoc}
     */
    render: function (model) {
      var activeCampaign = this.collection.findWhere({'isActive': true});
      if (!activeCampaign) {
        this.$el
          .find('a[href]')
          .attr('href', '')
          .end()
          .hide();
      }
      else {
        // The report link will be empty if reports are not available for this
        // campaign agent type.
        var reportLink = activeCampaign.get('links').report;
        if (reportLink.length == 0) {
          reportLink = 'javascript:void(0);';
          this.$el.find('a[href]').addClass('acquia-lift-menu-disabled');
        } else {
          this.$el.find('a[href]').removeClass('acquia-lift-menu-disabled');
        }
        var name = activeCampaign.get('name');
        var label = activeCampaign.get('label');
        this.$el
          .find('a[href]')
          .attr('href', reportLink)
          .text(Drupal.t('Reports'))
          .end()
          .show();
      }
    }
  }),

  /**
   * Updates the status link to the correct verb for each campaign.
   *
   * Also handles Ajax submission to change the status of the selected campaign.
   */
  MenuStatusView: ViewBase.extend({

    events: {
      'click .acquia-lift-status-update': 'updateStatus'
    },

    /**
     * {@inheritdoc}
     */
    initialize: function (options) {
      _.bindAll(this, "updateStatus");
      this.collection = options.collection;
      if (!this.model) {
        return;
      }
      this.model.on('change', this.render, this);
      this.render(this.model);
    },

    /**
     * {@inheritdoc}
     */
    render: function (model) {
      var activeCampaign = this.collection.findWhere({'isActive': true});
      if (!activeCampaign) {
        this.$el.hide();
      }
      else {
        var nextStatus = activeCampaign.get('nextStatus');
        this.$el
          .find('a[href]')
          .attr('href', 'javascript:void(0);')
          .text(nextStatus.text)
          .data('acquia-lift-campaign-status', nextStatus.status)
          .removeClass('acquia-lift-menu-disabled')
          .end()
          .show();
        // The campaign must be verified in order to change the status.
        if (activeCampaign.get('verified') == true) {
          this.$el.find('a[href]').removeClass('acquia-lift-menu-disabled');
        } else {
          this.$el.find('a[href]').addClass('acquia-lift-menu-disabled');
        }
      }
    },

    /**
     * {@inheritdoc}
     */
    build: function(model) {
      this.$el
        .find('a[href]')
        .attr('href', 'javascript:void(0)')
        .addClass('acquia-lift-status-update');
    },

    /**
     * Update the status of the current campaign to its next status value.
     *
     * @param event
     *   Click event that triggered this function.
     */
    updateStatus: function(event) {
      var newStatus = $(event.target).data('acquia-lift-campaign-status');
      var activeModel = this.collection.findWhere({'isActive': true});
      if (!newStatus || !activeModel || activeModel.get('verified') == false) {
        return;
      }
      // Make link disabled while update happens.
      // The disabled class will be removed when re-rendered.
      this.$el.find('a[href]').addClass('acquia-lift-menu-disabled');
      activeModel.updateStatus(newStatus);
    }
  }),


  /**
   * Toggles the 'add content variation' trigger.
   */
  MenuAddContentVariationTriggerView: ViewBase.extend({
    events: {
      'click': 'onClick'
    },
    /**
     * {@inheritdoc}
     */
    initialize: function (options) {
      this.model.on('change', this.render, this);
      this.render(this.model);
    },

    /**
     * {@inheritdoc}
     */
    render: function (model) {
      this.$el.toggleClass('acquia-lift-active', model.get('isActive'));
    },

    /**
     * Responds to clicks.
     *
     * @param jQuery.Event event
     */
    onClick: function (event) {
      event.preventDefault();
      this.model.set('isActive', !this.model.get('isActive'));
    }
  }),

  /**
   * Adds an identifying class to elements on the page that can be varied.
   */
  MenuContentVariationCandidateView: ViewBase.extend({
    /**
     * {@inheritdoc}
     */
    initialize: function (options) {
      this.model.on('change', this.render, this);
    },

    /**
     * {@inheritdoc}
     */
    render: function (model) {
      var isActive = model.get('isActive');
      this.$el.toggleClass('acquia-lift-content-variation-candidate', isActive);
      // Pull the Personalize contextual link out of the list and highlight it.
      if (isActive) {
        var $wrapper = this.$el.find('.contextual-links-wrapper:first');
        var $link = $wrapper.find('.personalize-this-contextual-link').detach();
        $wrapper.children('.contextual-links-trigger').addClass('acquia-lift-hidden');
        $wrapper.prepend($link);
      }
      // Repair the contextual links.
      else {
        var $wrapper = this.$el.find('.contextual-links-wrapper:first');
        var $link = $wrapper.find('.personalize-this-contextual-link')
        $wrapper.find('.contextual-links .personalize').append($link);
        $wrapper.children('.contextual-links-trigger').removeClass('acquia-lift-hidden');
      }
    },

    /**
     * {@inheritdoc}
     */
    remove: function () {
      ViewBase.prototype.remove.call(this, true);
    }
  })
});

/**
 * Collections
 */
$.extend(Drupal.acquiaLiftUI, {

  /**
   * Provides campaign model management.
   */
  MenuCampaignCollection: Backbone.Collection.extend({
    model: Drupal.acquiaLiftUI.MenuCampaignModel,
    initialize: function () {
      this.on('change:isActive', this.changeVisibility, this);
    },
    changeVisibility: function (changedModel, value, options) {
      // When a campaign is deactivated, we don't need to enforce anything.
      if (changedModel.get('isActive') === false) {
        return;
      }

      // This campaign was activated; deactivate all other campaigns.
      changedModel.collection.chain()
        .filter(function (model) {
          return model.get('isActive') === true && model !== changedModel;
        })
        .each(function (model) {
          model.set('isActive', false);
        });
    }
  })
});

/**
 * Determine the length of an object.
 *
 * @param object obj
 *   The object whose size will be determined.
 *
 * @return number
 *   The size of the object determined by the number of keys.
 */
function size (obj) {
  var size = 0, key;
  for (key in obj) {
    if (obj.hasOwnProperty(key)) {
      size++;
    }
  }
  return size;
}

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
function looper (obj, cb) {
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
 * Replaces spaces and underscored with dashes in a string.
 *
 * @param string str
 *   The string to be cleaned.
 *
 * @return str
 *   The updated string.
 */
function formatClass (str) {
  return str.trim().replace(/[\s\_]+/g, '-');
}

/**
 * Returns an href with the preview parameter for the option set option.
 *
 * The URL fragment is maintained.
 *
 * @param object options
 *   Keys:
 *   - osID: The ID of the option set.
 *   - os: The option set object.
 *
 * @return string
 */
function generateHref (options) {
  var base = Drupal.settings.basePath;
  var path = location.pathname && pathRegex.exec(location.pathname)[1] || '';
  var param = Drupal.settings.personalize.optionPreselectParam;

  var href = base + path;
  href += '?' + param + '=' + options.osID + '--' + options.id;
  // Now we need to add on any other Option Sets for which a preview option
  // had been selected so that we can preview more than one at a time.
  var existingSelection = decodeURI(window.location.search.replace(new RegExp("^(?:.*[&\\?]" + encodeURI(param).replace(/[\.\+\*]/g, "\\$&") + "(?:\\=([^&]*))?)?.*$", "i"), "$1"));
  if (existingSelection.length > 0) {
    looper(existingSelection.split(','), function (str, key) {
      // Exclude any preselection for the Option Set we're generating a link for.
      if (str.indexOf(options.osID + '--') == -1) {
        href += ',' + str;
      }
    });
  }

  return href;
}

/**
 * Returns the Backbone View of the Visitor Actions add action controller.
 *
 * @return Backbone.View
 */
function getVisitorActionsAppModel () {
  return Drupal.visitorActions && Drupal.visitorActions.ui && Drupal.visitorActions.ui.models && Drupal.visitorActions.ui.models.appModel;
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
 *
 * @return string
 */
Drupal.theme.acquiaLiftOptionSetItem = function (options) {
  var attrs = [
    'class="acquia-lift-preview-option-set acquia-lift-preview-option-set-' + formatClass(options.osID)  + '"',
    'data-acquia-lift-personalize-id="' + options.osID + '"',
    'data-acquia-lift-personalize-agent="' + options.os.agent + '"'
  ];
  var item = '';
  item += '<span ' + attrs.join(' ') + '>' + Drupal.checkPlain(options.os.label) + '</span>';
  item += Drupal.theme('acquiaLiftPersonalizeMenu', options);
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
 *
 * @return string
 */
Drupal.theme.acquiaLiftPersonalizeMenu = function (options) {
  var menu = '<ul class="menu">' + "\n";
  var osID = options.osID;
  var os = options.os;
  var os_selector = os.selector;
  looper(options.os.options || {}, function (obj, key) {
    menu += Drupal.theme('acquiaLiftPreviewMenuItem', {
      id: obj.option_id,
      label: obj.option_label,
      osID: osID,
      osSelector: os_selector
    });
  });
  menu += '</ul>\n';
  return menu;
};

/**
 * Returns a campaign goal menu item.
 *
 * @param object options
 *   Keys:
 *   - campaignID: The ID of the campaign for these goals.
 *   - name: The goal ID.
 *   - label: The goal label.
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

  item += '\n<span ' + attrs.join(' ') + '>\n';
  item +=  Drupal.t('@text', {'@text': options.label}) + '\n';
  item += '</span>\n';

  return item;
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

/**
 * Returns a menu item that contains a link to an option set option.
 *
 * @param object options
 *   Keys:
 *   - id: The id of the option set option.
 *   - label: The label of the option set option.
 *   - osID: The ID of the option set.
 *   - osSelector: The selector representing the option set.
 *
 * @return string
 */
Drupal.theme.acquiaLiftPreviewMenuItem = function (options) {
  var item = '';
  // Prepare the selector string to be passed as a data attribute.
  var selector = options.osSelector.replace(/\"/g, '\'');
  var attrs = [
    'class="acquia-lift-preview-option acquia-lift-preview-option--' + formatClass(options.id) + ' visitor-actions-ui-ignore"',
    'href="' + generateHref(options) + '"',
    'data-acquia-lift-personalize-option-set="' + options.osID + '"',
    'data-acquia-lift-personalize-option-set-selector="' + selector + '"',
    'data-acquia-lift-personalize-option-set-option="' + options.id + '"',
    'aria-role="button"',
    'aria-pressed="false"'
  ];

  item += '<li>\n<a ' + attrs.join(' ') + '>\n';
  item +=  Drupal.t('Preview @text', {'@text': options.label}) + '\n';
  item += '</a>\n</li>\n';

  return item;
};

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
    'title="' + Drupal.t('Edit the @campaign campaign', {'@campaign': options.link.label}) + '"',
    'href="' + options.edit.href + '"'
  ];

  var linkAttrs = [
    'class="acquia-lift-campaign acquia-lift-campaign--' + formatClass(options.link.id) + ' visitor-actions-ui-ignore"',
    'href="' + options.link.href + '"',
    'title="' + Drupal.t('Preview the @campaign campaign', {'@campaign': options.link.label}) + '"',
    'data-acquia-lift-personalize-agent="' + options.link.id + '"',
    'aria-role="button"',
    'aria-pressed="false"'
  ];

  var item = '<div class="acquia-lift-menu-item"><a ' + linkAttrs.join(' ') + '>' + options.link.label + '</a>';
  item += '<a ' + editAttrs.join(' ') + '>' + Drupal.t('Edit') + '</a>\n';
  item += '</div>\n';
  //item += '<a ' + linkAttrs.join(' ') + '>' + options.link.label + '</a>\n';

  return item;
};

/**
 * Returns HTML for a count display element.
 */
Drupal.theme.aquiaLiftCount = function (options) {
  return '<i class="acquia-lift-personalize-type-count acquia-lift-empty"><span>0</span>&nbsp;</i>';
};

/**
 * Returns the HTML for the selected campaign context label.
 */
Drupal.theme.acquiaLiftCampaignContext = function (options) {
  var label = Drupal.t('Selected: ');
  label += '<span class="acquia-lift-active">' + options.label + '</span>';
  return label;
}

/**
 * Throbber.
 */
Drupal.theme.acquiaLiftThrobber = function () {
  return '<div class="ajax-progress ajax-progress-throbber"><div class="throbber">&nbsp;</div></div>';
};

}(Drupal, jQuery, _, Backbone));
