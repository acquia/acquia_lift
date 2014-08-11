/**
 * @file
 * acquia_lift.personalize.js
 */
(function (Drupal, $, _, Backbone) {
  // Removes leading '/', '?' and '#' characters from a string.
  var pathRegex = /^(?:[\/\?\#])*(.*)/;

  var reportPath = '/admin/structure/personalize/manage/acquia-lift-placeholder/report';
  var statusPath = '/admin/structure/personalize/manage/acquia-lift-placeholder/status';

  var updateNavbar = function() {
    if (initialized && Drupal.behaviors.acquiaLiftNavbarMenu) {
      Drupal.behaviors.acquiaLiftNavbarMenu.attach();
    }
  }
  var initialized = false;

  Drupal.behaviors.acquiaLiftPersonalize = {
    attach: function (context) {
      var settings = Drupal.settings.personalize;
      var ui = Drupal.acquiaLiftUI;
      var addedCampaigns = {};
      if (settings) {
        // Build models for menus that don't have them yet.
        if (!ui.collections.campaigns) {
          ui.collections.campaigns = new ui.MenuCampaignCollection([]);
        }
        looper(settings.campaigns, function (obj, key) {
          if (!ui.collections.campaigns.findWhere({name: obj.name})) {
            var model = Drupal.acquiaLiftUI.factories.MenuFactory.createCampaignModel(obj);
            ui.collections.campaigns.add(model);
            addedCampaigns[obj.name] = model;
          }
        });
        looper(settings.option_sets, function (obj, key) {
          var campaignModel = ui.collections.campaigns.findWhere({name: obj.agent});
          var optionSets = campaignModel.get('optionSets');
          var optionSet = optionSets.findWhere({'osid': key});
          // Merge doesn't work in this case so we need to manually merge.
          if (optionSet) {
            for (var prop in obj) {
              if (obj.hasOwnProperty(prop)) {
                optionSet.set(prop, obj[prop]);
              }
            }
          } else {
            optionSets.add(new Drupal.acquiaLiftUI.MenuOptionSetModel(obj));
          }
        });
        // Clear the variations for all page variation campaigns.
        ui.collections.campaigns.each(function (model) {
          if (model instanceof Drupal.acquiaLiftUI.MenuCampaignABModel) {
            model.get('optionSets').resetVariations();
          }
        });

        // Create a model for page variation management state
        if (!ui.models.pageVariationModeModel) {
          ui.models.pageVariationModeModel = new ui.MenuPageVariationModeModel();
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
            // Attach a view that will report the number of campaigns
            // if this link is in the Navbar.
            if ($link.closest('.navbar-tray').length) {
              switch (category) {
                case 'campaigns':
                  collection = ui.collections[category];
                  $element = $(Drupal.theme('acquiaLiftCount'));
                  ui.views.push((new ui.MenuCampaignCountView({
                    el: $element.get(0),
                    model: collection
                  })));
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
                      model: null,
                    });
                    $('[data-acquia-lift-personalize-type="campaigns"]').prepend(element);
                  }
                  $element.prependTo($link);
                  break;
                case 'option_sets': {
                  Drupal.acquiaLiftUI.views.variationSetsMenuView = new Drupal.acquiaLiftUI.MenuContentVariationsMenuView({
                    campaignCollection: ui.collections.campaigns,
                    el: $link[0]
                  });
                  $element = $(Drupal.theme('acquiaLiftPageVariationToggle'));
                  ui.views.pageVariationToggle = new ui.MenuPageVariationsToggleView({
                    model: ui.models.pageVariationModeModel,
                    campaignCollection: ui.collections.campaigns,
                    el: $element.get(0)
                  });
                  $link.after($element);
                }
              }
            }
          });
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
                model: campaignModel,
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
          var campaignsWithOptions = {};
          var viewName = null;
          if ($typeMenus.length) {
            $typeMenus
              .each(function (index, element) {
                var $menu = $(element);
                var type = $menu.data('acquia-lift-personalize-type');
                var model, element, campaignName, campaignModel, optionSets;
                looper(settings[type], function (obj, key) {
                  // Find the right model.
                  switch (type) {
                    case 'option_sets':
                      // If the menu already has a link for this setting, abort.
                      if (!$menu.find('[data-acquia-lift-personalize-agent="' + obj.agent + '"][data-acquia-lift-personalize-id="' + key + '"].acquia-lift-preview-page-variation').length) {
                        campaignName = obj.agent;
                        campaignsWithOptions[obj.agent] = obj.agent;
                        campaignModel = ui.collections.campaigns.findWhere({'name': campaignName});
                        optionSets = campaignModel.get('optionSets');
                        model = optionSets.findWhere({'osid': key});
                        viewName = 'MenuOptionView';
                      }
                      break;
                    case 'campaigns':
                      // If the menu already has a link for this setting, abort.
                      if (!$menu.find('[data-acquia-lift-personalize-agent="' + key + '"].acquia-lift-campaign').length) {
                        campaignName = key;
                        campaignModel = model = ui.collections[type].findWhere({'name': key});
                        viewName = 'MenuCampaignView';
                      }
                      break;
                  }
                  // Create views for the campaign model if it was just added.
                  if (model && addedCampaigns.hasOwnProperty(campaignName)) {
                    element = document.createElement('li');
                    if (type == 'campaigns') {
                      ui.views.push(new ui.MenuCampaignView({
                        el: element,
                        model: model
                      }));
                    } else {
                      ui.views.push(ui.factories.MenuFactory.createContentVariationView(model, campaignModel, element));
                    }
                    $menu.prepend(element);
                    // Build a view for campaign goals.
                    if (type === 'campaigns') {
                      var $goalsMenu = $('[data-acquia-lift-personalize-type="goals"]');
                      var modelGoals = model.get('goals');
                      if (modelGoals) {
                        looper(modelGoals, function (obj, key) {
                          element = document.createElement('li');
                          ui.views.push((new ui.MenuGoalsView({
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
                        ui.views.push((new ui.MenuGoalsView({
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
                    if (addedCampaigns.hasOwnProperty(key) && !campaignsWithOptions.hasOwnProperty(key)) {
                      // Add a content variations view for any campaigns that don't have existing
                      // option sets (and therefore would have been missed).
                      model = ui.collections.campaigns.findWhere({'name': key});
                      element = document.createElement('li');
                      ui.factories.MenuFactory.createEmptyContentVariationView(model, element);
                      $menu.prepend(element);
                    }
                  })
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

        // Create View for the Report link.
        if (ui.collections.campaigns.length > 0) {
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
        if (ui.collections.campaigns.length > 0) {
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
        ui.collections.campaigns.each(function (campaignModel) {
          campaignModel.refreshData();
        });
        if (!ui.collections['campaigns']) {
          return;
        }
        var activeCampaign = '';
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
        Drupal.acquiaLiftUI.setActiveCampaign(activeCampaign);
        initialized = true;
        updateNavbar();
      }
    }
  };

  Drupal.behaviors.acquiaLiftContentVariations = {
    attach: function (context) {
      var settings = Drupal.settings.personalize;
      var ui = Drupal.acquiaLiftUI;
      // Create a model for a Content Variation management state.
      if (!ui.models.contentVariationModeModel) {
        ui.models.contentVariationModeModel = new ui.MenuContentVariationModeModel();
      }
      // Create a model for page variation management state
      if (!ui.models.pageVariationModeModel) {
        ui.models.pageVariationModeModel = new ui.MenuPageVariationModeModel();
      }

      // Keep the in-context content variation editing, page variation editing,
      // and in-context goal creation in mutually exclusive active states.
      $('body').once('acquia-lift-personalize', function () {
        // Turn off content variations highlighting if visitor actions editing
        // is enabled.
        $(document).bind('visitorActionsUIEditMode', function (event, isActive) {
          if (isActive) {
            // Prevent infinite loops of updating models triggering change events
            // by delaying this update to the next evaluation cycle.
            window.setTimeout(function () {
              ui.models.contentVariationModeModel.endEditMode();
              ui.models.pageVariationModeModel.endEditMode();
            }, 0);
          }
        });
        // Signal when content variation highlighting is active.
        ui.models.contentVariationModeModel.on('change:isActive', function (model, isActive) {
          if (isActive) {
            $(document).trigger('visitorActionsUIShutdown');
            ui.models.pageVariationModeModel.endEditMode();
          }
        });
        // Turn off content variation and visitor actions modes when entering
        // page variation mode.
        $(document).bind('acquiaLiftPageVariationMode', function (event, data) {
          if (data.start) {
            ui.models.contentVariationModeModel.endEditMode();
            $(document).trigger('visitorActionsUIShutdown');
          }
        });
      });

      // Build Views for the Add Content Variation triggers.
      $('[data-acquia-lift-personalize-mode="content-variation"]')
        .once('acquia-lift-personalize-trigger')
        .each(function (index, element) {
          ui.views.push((new ui.MenuContentVariationTriggerView({
            el: element,
            contentVariationModel: ui.models.contentVariationModeModel,
            pageVariationModel: ui.models.pageVariationModeModel,
            campaignCollection: ui.collections.campaigns
          })));
        });
      // Find content variation candidates.
      $('[data-personalize-entity-id]')
        .once('acquia-lift-personalize-variation-candidate')
        .each(function (index, element) {
          ui.views.push((new ui.MenuContentVariationCandidateView({
            el: element,
            model: ui.models.contentVariationModeModel
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

  var contentModeModelBase = Backbone.Model.extend({
    defaults: {
      isActive: false
    },

    /**
     * Helper function to start adding a content variation.
     */
    startAddMode: function () {
      this.set('isActive', true);
    },

    /**
     * Helper function to start editing a content variation.
     *
     * @param variationIndex
     *   Index of the variation to edit within the current campaign context.
     */
    startEditMode: function (variationIndex) {
      this.set('isActive', true);
    },

    /**
     * Helper function to end editing mode for a page variation.
     */
    endEditMode: function () {
      this.set('isActive', false);
    }
  });

  $.extend(Drupal.acquiaLiftUI, {
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
      Drupal.settings.personalize.activeCampaign = activeCampaign;
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
        verified: false,
        type: ''
      },

      /**
       * {@inheritDoc}
       */
      initialize: function(options) {
        this.set('optionSets', new Drupal.acquiaLiftUI.MenuOptionSetCollection());
        this.listenTo(this.get('optionSets'), 'add', this.triggerOptionSetChange);
        this.listenTo(this.get('optionSets'), 'remove', this.triggerOptionSetChange);
        this.listenTo(this.get('optionSets'), 'change:variations', this.triggerOptionSetChange);
      },

      triggerOptionSetChange: function (event) {
        this.trigger('change:optionSets');
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
        var that = this;
        var optionSets = this.get('optionSets');
        optionSets.each(function (model) {
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
    MenuOptionSetModel: Backbone.Model.extend({
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
        winner: null
      },

      /**
       * {@inheritDoc}
       */
      initialize: function(options) {
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
        if (typeof property == 'object' && property.hasOwnProperty('options')) {
          property.options = this.setOptions(property.options);
        } else if (property === 'options' && value instanceof Array) {
          value = this.setOptions(value);
        }
        this.parent('set', property, value);
      },

      setOptions: function (options) {
        var current,
          triggerChange = false,
          optionsCollection = this.get('options') || new Drupal.acquiaLiftUI.MenuOptionCollection();

        _.each(options, function(option, index) {
          // Update the model properties if the model is already in options.
          if (current = optionsCollection.findWhere({'option_id': option.option_id})) {
            _.each(option, function(optionValue, optionProp) {
              if (current.get(optionProp) !== optionValue) {
                current.set(optionProp, optionValue);
                triggerChange = true;
              }
            });
          } else {
            // Otherwise just add the new option.
            option.original_index = index;
            optionsCollection.add(option);
            triggerChange = true;
          }
        });
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
    }),

    /**
     * The model for a single option within an option set or page variation.
     */
    MenuOptionModel: Backbone.Model.extend({
      defaults: {
        option_id: '',
        option_label: '',
        original_index: null
      }
    }),

    /**
     * The Model for a 'add content variation' state.
     */
    MenuContentVariationModeModel: contentModeModelBase.extend({}),

    /**
     * The model for 'add page variation' state.
     */
    MenuPageVariationModeModel: contentModeModelBase.extend({
      defaults: {
        isActive: false,
        isEditMode: false,
        variationIndex: -1
      },

      /**
       * Helper function to start adding a page variation.
       *
       * This handles setting all the correct model parameters and sending an
       * event notification.
       */
      startAddMode: function () {
        // Don't restart if alredy in create mode.
        if (this.get('isActive') && !this.get('isEditMode')) {
          return;
        }
        this.set('variationIndex', -1);
        this.set('isEditMode', false);
        this.set('isActive', true);
        this.notifyTrigger();
      },

      /**
       * Helper function to start editing a page variation.
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
       * Helper function to end editing mode for a page variation.
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
        $(document).trigger('acquiaLiftPageVariationModeTrigger', [data]);
      }
    }),

    /**
     * View for the top-level content variations menu.
     */
    MenuContentVariationsMenuView: ViewBase.extend({

      /**
       * {@inheritDoc}
       */
      initialize: function(options) {
        this.campaignCollection = options.campaignCollection;
        this.listenTo(this.campaignCollection, 'change:isActive', this.render);
        this.listenTo(this.campaignCollection, 'change:activeVariation', this.render);
        this.listenTo(this.campaignCollection, 'change:variations', this.render);
      },

      /**
       * {@inheritDoc}
       */
      render: function() {
        var currentCampaign = this.campaignCollection.findWhere({'isActive': true});
        var text = Drupal.t('Variation Sets');
        var $count = this.$el.find('i.acquia-lift-personalize-type-count').detach();
        if (!currentCampaign) {
          return;
        }
        if (currentCampaign instanceof Drupal.acquiaLiftUI.MenuCampaignABModel) {
          var currentVariation = currentCampaign.getCurrentVariationLabel();
          if (currentVariation) {
            text = Drupal.t('Variation: @variation', {'@variation': currentVariation});
          } else {
            text = Drupal.t('Variations');
          }
        }
        this.$el.text(text);
        if ($count) {
          this.$el.prepend($count);
        }
      }
    }),

    /**
     * View for all content variations in a campaign.
     *
     * The model in this view is actually the campaign model.
     */
    MenuContentVariationsView: ViewBase.extend({

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
    MenuOptionSetEmptyView: ViewBase.extend({
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
     * Backbone View/Controller for the page variations of a campaign.
     */
    MenuPageVariationsView: ViewBase.extend({
      events: {
        'click .acquia-lift-preview-option': 'onClick'
      },

      /**
       * {@inheritDoc}
       */
      initialize: function (options) {
        // the model is the campaign model.
        this.listenTo(this.model, 'destroy', this.remove);
        this.listenTo(this.model, 'change:isActive', this.render);
        this.listenTo(this.model, 'change:variations', this.rebuild);
        this.listenTo(this.model, 'change:activeVariation', this.render);

        this.onOptionShowProxy = $.proxy(this.onOptionShow, this);
        this.onPageVariationEditModeProxy = $.proxy(this.onPageVariationEditMode, this);

        $(document).on('personalizeOptionChange', this.onOptionShowProxy);
        $(document).on('acquiaLiftPageVariationMode', this.onPageVariationEditModeProxy, this);

        this.rebuild();
      },

      /**
       * {@inheritDoc}
       */
      render: function(model) {
        this.$el
          .find('[data-acquia-lift-personalize-page-variation]')
          .removeClass('acquia-lift-active')
          .attr('aria-pressed', 'false');
        var activeVariation = model.get('activeVariation');
        var variationData = (isNaN(activeVariation) || activeVariation == -1) ? 'new' : activeVariation;
        this.$el.find('[data-acquia-lift-personalize-page-variation="' + variationData + '"]')
          .addClass('acquia-lift-active')
          .attr('aria-pressed', 'true');
      },

      /**
       * Regenerates the list HTML and adds to the element.
       * This is necessary when the option set collection changes.
       *
       * @param model
       */
      rebuild: function() {
        this.build(this.model);
        this.render(this.model);
        // Re-run navbar handling to pick up new menu options.
        _.debounce(updateNavbar, 300);
        _.debounce(Drupal.attachBehaviors(this.$el), 300);
      },

      /**
       * {@inheritDoc}
       */
      build: function(model) {
        var html = '';
        html += Drupal.theme('acquiaLiftPageVariationsItem', model);
        this.$el.html(html);
      },

      /**
       * {@inheritdoc}
       */
      remove: function () {
        $(document).off('personalizeOptionChange', this.onOptionShowProxy);
        $(document).off('acquiaLiftPageVariationMode', this.onPageVariationEditModeProxy);
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
        var variation_index = $(event.target).data('acquia-lift-personalize-page-variation');
        // Clicked new variation name when in add mode.
        if (isNaN(variation_index)) {
          return;
        }
        var variations = this.model.get('optionSets').getVariations();
        if (variation_index >= variations.length) {
          return;
        }
        this.model.set('activeVariation', variation_index);
        var variation = variations[variation_index];
        var i, num = variation.options.length, current;
        // Run the executor for each option in the variation.
        for (i=0; i < num; i++) {
          current = variation.options[i];
          Drupal.personalize.executors[current.executor].execute($(current.selector), current.option.option_id, current.osid);
        }
        event.preventDefault();
        event.stopPropagation();
      },

      /**
       * Select a specific variation to show.
       *
       * @param number variationIndex
       *   The variation index to show.
       */
      selectVariation: function (variationIndex) {
        var variationData = variationIndex < 0 ? 'new' : variationIndex;
        _.defer(function($context, variationId) {
          $context.find('[data-acquia-lift-personalize-page-variation="' + variationId + '"]').trigger('click');
        }, this.$el, variationData)
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
        var optionSets = this.model.get('optionSets');
        var optionSet = optionSets.findWhere({osid: osid});
        if (!optionSet) {
          return;
        }
        var options = optionSet.get('options');
        var option = options.findWhere({'option_id': choice_name});
        var variationIndex = options.indexOf(option);
        if (variationIndex < 0) {
          return;
        }
        this.model.set('activeVariation', variationIndex);
      },

      /**
       * Response to a change in edit mode for the page variation application.
       *
       * @param event
       *   The jQuery event object
       * @param data
       *   An object of event data including the keys:
       *   - start: true if edit mode started, false if ended.
       *   - campaign: the machine name of the campaign holding variations.
       *   - variationIndex: the index of the variation for editing or -1
       *     if adding a new variation.
       */
      onPageVariationEditMode: function (event, data) {
        // Make sure it's for this campaign.
        if (this.model.get('name') !== data.campaign) {
          return;
        }
        if (data.start) {
          if (data.variationIndex < 0) {
            // If add mode, then create a temporary variation listing.
            var nextIndex = this.model.getNumberOfVariations();
            // The first option is always control so the numbering displayed
            // actually matches the index number.
            var variationNumber = Math.max(nextIndex, 1);
            if (nextIndex == 0) {
              // Add a control variation display as well.
              this.$el.find('ul.menu').append(Drupal.theme('acquiaLiftNewVariationMenuItem', -1));
            }
            this.$el.find('ul.menu').append(Drupal.theme('acquiaLiftNewVariationMenuItem', variationNumber));
            this.$el.find('ul.menu li.acquia-lift-empty').hide();
            // Indicate in the model that we are adding.
            this.model.set('activeVariation', -1);
            this.render(this.model);
          } else {
            // If in edit mode, make sure that the edited variation index is
            // indicated.
            var $li = this.$el.find('[data-acquia-lift-personalize-page-variation="' + data.variationIndex + '"]');
            $li.trigger('click');
          }
          updateNavbar();
        } else {
          // If exiting, remove any temporary variation listings.
          this.$el.find('ul.menu li.acquia-lift-empty').show();
          this.$el.find('.acquia-lift-page-variation-new').closest('li').remove();
          // If the model is set at adding, change it back to the control option.
          if (this.model.get('activeVariation') == -1) {
            this.model.set('activeVariation', 0);
          }
        }
      }
    }),

    /**
     * Backbone View/Controller for an option set within a campaign.
     */
    MenuOptionSetView: ViewBase.extend({

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
    MenuContentVariationsCountView: ViewBase.extend({
      /**
       * {@inheritdoc}
       */
      initialize: function (options) {
        this.listenTo(this.model, 'change:isActive', this.render);
        this.listenTo(this.model, 'change:optionSets', this.render);
        this.listenTo(this.model, 'change:activeVariation', this.render);

        this.render();
      },

      /**
       * {@inheritdoc}
       */
      render: function () {
        var variations = this.model.getNumberOfVariations();
        if (this.model instanceof Drupal.acquiaLiftUI.MenuCampaignABModel && this.model.get('activeVariation') == -1) {
          // We are in add mode so adjust the number to show.
          variations++;
        }
        if (this.model.get('isActive')) {
          this.$el
            .toggleClass('acquia-lift-empty', !variations)
            .css('display', 'inline-block')
            .find('span').text(variations);
        } else {
          this.$el.css('display', 'none');
        }
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
        if (model.get('isActive')) {
          this.$el
            .toggleClass('acquia-lift-empty', !count)
            .css('display', 'inline-block')
            .find('span').text(count);
        } else {
          this.$el.css('display', 'none');
        }

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
        this.build(this.model);
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
     * The toggle functionality for editing page variations.
     */
    MenuPageVariationsToggleView: ViewBase.extend({
      events: {
        'click': 'onClick'
      },

      /**
       * @{inheritDoc}
       *
       * The model is the page variations mode model.
       */
      initialize: function (options) {
        this.campaignCollection = options.campaignCollection;
        this.listenTo(this.campaignCollection, 'change:isActive', this.render);
        this.listenTo(this.campaignCollection, 'change:activeVariation', this.render);
        this.listenTo(this.model, 'change:isActive', this.render);
        this.build();
        this.render();
      },

      /**
       * {@inheritDoc}
       */
      render: function() {
        var currentCampaign = this.campaignCollection.findWhere({'isActive': true});
        if (!currentCampaign) {
          return;
        }
        this.$el
          .toggleClass('acquia-lift-page-variation-toggle-disabled', currentCampaign.get('activeVariation') == 0) // There is no toggle available for the control variation.
          .toggleClass('acquia-lift-page-variation-toggle-active', this.model.get('isActive'))
          .toggle(currentCampaign instanceof Drupal.acquiaLiftUI.MenuCampaignABModel);
      },

      /**
       * {@inheritDoc}
       */
      build: function() {
        this.$el.text(Drupal.t('Toggle edit variation'));
      },

      /**
       * Event handler for clicking on the toggle link.
       * @param event
       */
      onClick: function (event) {
        var currentCampaign = this.campaignCollection.findWhere({'isActive': true});
        if (!currentCampaign) {
          return;
        }
        if (this.model.get('isActive')) {
          this.model.endEditMode();
        } else {
          var currentVariationIndex = currentCampaign.get('activeVariation');
          if (currentVariationIndex == 0) {
            // Cannot edit the control variation.
            return;
          }
          this.model.startEditMode(currentVariationIndex);
        }
      }
    }),

    /**
     * Toggles the 'add content variation' trigger.
     */
    MenuContentVariationTriggerView: ViewBase.extend({
      events: {
        'click': 'onClick'
      },

      /**
       * {@inheritdoc}
       */
      initialize: function (options) {
        this.contentVariationModel = options.contentVariationModel;
        this.pageVariationModel = options.pageVariationModel;
        this.campaignCollection = options.campaignCollection;

        // Model property holds a reference to the relevant type of creation
        // mode model based on the type of campaign selected.
        if (this.campaignCollection.findWhere({'isActive': true}) instanceof Drupal.acquiaLiftUI.MenuCampaignABModel) {
          this.model = this.pageVariationModel;
        } else {
          this.model = this.contentVariationModel;
        }

        this.listenTo(this.contentVariationModel, 'change:isActive', this.render);
        this.listenTo(this.pageVariationModel, 'change:isActive', this.render);
        this.listenTo(this.campaignCollection, 'change:isActive', this.onCampaignChange);

        this.onPageVariationEditModeProxy = $.proxy(this.onPageVariationEditMode, this);
        $(document).on('acquiaLiftPageVariationMode', this.onPageVariationEditModeProxy, this);

        this.render(this.model);
      },

      /**
       * {@inheritdoc}
       */
      render: function (model) {
        var isActive = model.get('isActive');
        var text = '';
        if (isActive) {
          text = Drupal.t('Exit edit mode');
        } else {
          text = model instanceof Drupal.acquiaLiftUI.MenuPageVariationModeModel ? Drupal.t('Add a variation') : Drupal.t('Add a variation set');
        }
        this.$el.toggleClass('acquia-lift-active', isActive);
        this.$el.text(text);
      },

      /**
       * Responds to clicks.
       *
       * @param jQuery.Event event
       */
      onClick: function (event) {
        event.preventDefault();

        if (this.model.get('isActive')) {
          this.model.endEditMode();
        } else {
          this.model.startAddMode();
        }
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
        this.model.endEditMode();
        if (isActive) {
          if (model instanceof Drupal.acquiaLiftUI.MenuCampaignABModel) {
            this.model = this.pageVariationModel;
          } else {
            this.model = this.contentVariationModel;
          }
          this.render(this.model);
        }
      },

      /**
       * Listens to changes broadcast from the page variation application.
       */
      onPageVariationEditMode: function (event, data) {
        this.pageVariationModel.set('isActive', data.start);
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

  $.extend(Drupal.acquiaLiftUI, {
    /**
     * The model for a simple A/B test campaign.
     */
    MenuCampaignABModel: Drupal.acquiaLiftUI.MenuCampaignModel.extend({

      /**
       * {@inheritDoc}
       */
      initialize: function() {
        this.parent('inherit');
        this.set('activeVariation', 0);
        this.listenTo(this, 'change:variations', this.triggerVariationChange);
        this.listenTo(this.get('optionSets'), 'change:variations', this.triggerVariationChange);
      },

      triggerOptionSetChange: function (event) {
        this.trigger('change:variations');
      },

      /**
       * {@inheritDoc}
       */
      getNumberOfVariations: function () {
        var optionSets = this.get('optionSets');
        return optionSets.getVariations().length;
      },

      /**
       * Get the current variation shown.
       */
      getCurrentVariationLabel: function() {
        var variationIndex = this.get('activeVariation');
        if (variationIndex < 0) {
          // Currently adding a new variation
          var currentNum = this.getNumberOfVariations();
          // Don't need to add 1 to the current total because of the control
          // option at position 0 unless it's new in which case it should say
          // "Variation #1" rather than 0.
          currentNum = currentNum == 0 ? 1 : currentNum;
          return Drupal.t('Variation #@num', {'@num': currentNum});
        }
        var optionSets = this.get('optionSets');
        var variations = optionSets.getVariations();
        if (variations.length > variationIndex) {
          return variations[variationIndex].label;
        }
        return null;
      },

      /**
       * {@inheritDoc}
       */
      refreshData: function () {
        var that = this;
        var optionSets = this.get('optionSets');
        if (optionSets.length > 0) {
          var sample = optionSets.at(0);
          var decisionName = sample.get('decision_name');
        } else {
          return;
        }
        // Default the selected variation to the first/control option if it
        // has not yet already been set.
        if (!isNaN(this.get('activeVariation'))) {
          return;
        }
        var index = 0;
        var found = false;
        // If there is an option pre-selected, then it should be the default
        // active option.
        // NOTE you cannot break out of each functions, so the found variable.
        optionSets.each(function(optionSet) {
          if (!found && Drupal.settings.personalize.preselected) {
            var preselectedOptionName = Drupal.settings.personalize.preselected[optionSet.get('osid')] || null;
            if (preselectedOptionName) {
              index = optionSet.get('option_names').indexOf(preselectedOptionName);
              if (index < 0) {
                index = 0;
              } else {
                found = true;
              }
            }
          }
        });
        if (!found && sample.get('winner') != null) {
          // Otherwise a winner should be the default if one has been defined.
          index = sample.get('winner');
        }
        // The first option key isn't always 0.
        var options = sample.get('options');
        if (!options.hasOwnProperty(index)) {
          var keys = _.keys(options);
          keys.sort;
          index = keys[0];
        }
        this.set('activeVariation', index);
      }
    })
  });

  /**
   * Factory methods.
   */
  Drupal.acquiaLiftUI.views.pageVariations = Drupal.acquiaLiftUI.views.pageVariations || {};
  Drupal.acquiaLiftUI.factories = Drupal.acquiaLiftUI.factories || {};
  Drupal.acquiaLiftUI.factories.MenuFactory = Drupal.acquiaLiftUI.factories.MenuFactory || {
    /**
     * Factory method to create the correct type of content variation set view
     * based on the type of data displayed.
     *
     * @param Drupal.acquiaLiftUI.MenuOptionSetModel model
     *   The model that will be the base for this view.
     * @param Drupal.acquiaLiftUI.MenuCampaignModel model
     *   The campaign model that owns the option set.
     * @param element
     *   The DOM element for the Backbone view.
     */
    createContentVariationView: function (model, campaignModel, element) {
      if (campaignModel instanceof Drupal.acquiaLiftUI.MenuCampaignABModel) {
        // There is only one page variation view per page per campaign, but
        // this may be called multiple times due to multiple option sets.
        var view, campaignName = campaignModel.get('name');
        if (!Drupal.acquiaLiftUI.views.pageVariations.hasOwnProperty(campaignName)) {
          view = Drupal.acquiaLiftUI.views.pageVariations[campaignName] = new Drupal.acquiaLiftUI.MenuPageVariationsView({
            model: campaignModel,
            el: element
          });
        }
        view = Drupal.acquiaLiftUI.views.pageVariations[campaignName];
      } else {
        view = new Drupal.acquiaLiftUI.MenuOptionSetView({
          model: model,
          el: element
        });
      }
      return view;
    },

    /**
     * Factory method to create the correct type of content variation set view
     * for a campaign with no content variations yet created.
     *
     * @param Drupal.acquiaLiftUI.MenuCampaignModel model
     *   The campaign model that owns the option set.
     * @param element
     *   The DOM element for the Backbone view.
     */
    createEmptyContentVariationView: function (model, element) {
      if (model instanceof Drupal.acquiaLiftUI.MenuCampaignABModel) {
        Drupal.acquiaLiftUI.views.pageVariations[model.get('name')] = new Drupal.acquiaLiftUI.MenuPageVariationsView({
          model: model,
          el: element
        });
      } else {
        Drupal.acquiaLiftUI.views.push(new Drupal.acquiaLiftUI.MenuOptionSetEmptyView({
          el: element,
          model: model
        }));
      }
    },

    /**
     * Factory method to create the correct type of campaign model based
     * on the type of data.
     *
     * @param object data
     *   The data to create the model from.
     */
    createCampaignModel: function (data) {
      if (data.type == 'acquia_lift_simple_ab') {
        return new Drupal.acquiaLiftUI.MenuCampaignABModel(data);
      } else {
        return new Drupal.acquiaLiftUI.MenuCampaignModel(data);
      }
    }
  };

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
    }),

    /**
     * A collection of option sets within a model.
     */
    MenuOptionSetCollection: Backbone.Collection.extend({
      model: Drupal.acquiaLiftUI.MenuOptionSetModel,

      /**
       * {@inheritDoc}
       */
      initialize: function() {
        // Allow certain model changes to trigger a general change event for
        // the entire collection.
        this.variations = null;
        this.on('change:options', this.triggerChange, this);
      },

      triggerChange: function() {
        this.trigger('change:variations');
      },

      /**
       * Causes the cached variation list to be reset.
       */
      resetVariations: function() {
        this.variations = null;
      },

      /**
       * Generates the variations listing for page variations made up of the
       * option sets within this collection.
       *
       * The results are cached within a local variable that is invalidated
       * when the variations/options change.
       */
      getVariations: function() {
        if (this.variations !== null) {
          return this.variations;
        }
        if (this.length == 0) {
          return [];
        }
        var i,
          sample = this.at(0),
          sampleOptions = sample ? sample.get('options') : null,
          num = sampleOptions ? sampleOptions.length : 0,
          variations = [],
          variation,
          options,
          option,
          valid,
          variationNum;
        for (i=0; i < num; i++) {
          valid = true;
          variationNum = i+1;
          variation = {
            index: i,
            options: [],
            agent: sample.get('agent')
          };
          this.each(function (model) {
            options = model.get('options');
            if (options instanceof Backbone.Collection) {
              options = options.toJSON();
            }
            if (options.length <= i) {
              // This variation is invalid because it does not have an option
              // in each option set.
              valid = false;
            } else {
              option = {
                decision_name: model.get('decision_name'),
                executor: model.get('executor'),
                osid: model.get('osid'),
                plugin: model.get('plugin'),
                selector: model.get('selector'),
                stateful: model.get('stateful'),
                winner: model.get('winner'),
                option: options[i]
              };
              variation.label = options[i].option_label;
              variation.options.push(option);
            }
          });
          if (valid) {
            variations.push(variation);
          }
        }
        this.variations = variations;
        return this.variations;
      }
    }),

    /**
     *  A collection of options.
     */
    MenuOptionCollection: Backbone.Collection.extend({
      model: Drupal.acquiaLiftUI.MenuOptionModel,

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
   * @param mixed options
   *   An array of options for preselection or a single options object.
   *   Each option object has the following keys:
   *   - osID: The ID of the option set.
   *   - os: The option set object.
   *
   * @return string
   */
  function generateHref (options) {
    var base = Drupal.settings.basePath;
    var path = location.pathname && pathRegex.exec(location.pathname)[1] || '';
    var param = Drupal.settings.personalize.optionPreselectParam;

    var href = base + path + '?' + param + '=';
    if (!options instanceof Array) {
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
      looper(existingSelection.split(','), function (str, key) {
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
   * Returns the Backbone View of the Visitor Actions add action controller.
   *
   * @return Backbone.View
   */
  function getVisitorActionsAppModel () {
    return Drupal.visitorActions && Drupal.visitorActions.ui && Drupal.visitorActions.ui.models && Drupal.visitorActions.ui.models.appModel;
  }

  /**
   * Themes a list of page variations for a campaign.
   *
   * @param MenuCampaignModel model
   *   The campaign model to create page variations display for.
   */
  Drupal.theme.acquiaLiftPageVariationsItem = function (model) {
    var optionSets = model.get('optionSets');
    var variations = optionSets.getVariations();

    var attrs = [
      'class="acquia-lift-preview-page-variation acquia-lift-content-variation navbar-menu-item"' +
      'data-acquia-lift-personalize-agent="' + model.get('name') + '"'
    ];
    var item = '';
    item += '<span ' + attrs.join(' ') + '>';
    item += Drupal.t('Variations');
    item += '</span>\n';

    item += '<ul class="menu">' + "\n";
    // Handle empty page variations.
    if (variations.length == 0) {
      item += '<li class="acquia-lift-empty">' + Drupal.theme('acquiaLiftPersonalizeNoMenuItem', {type: 'page variations'}) + '</li>\n';
    } else {
      _.each(variations, function (variation, index, list) {
        item += Drupal.theme('acquiaLiftPreviewPageVariationMenuItem', variation);
      });
    }
    item += '</ul>\n';
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
   *
   * @return string
   */
  Drupal.theme.acquiaLiftOptionSetItem = function (options) {
    var attrs = [
      'class="acquia-lift-preview-option-set acquia-lift-content-variation acquia-lift-preview-option-set-' + formatClass(options.osID)  + '"',
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
   *
   * @return string
   */
  Drupal.theme.acquiaLiftOptionSetMenu = function (options) {
    var menu = '<ul class="menu">' + "\n";
    var osID = options.osID;
    var os = options.os;
    var os_selector = os.selector;
    options.os.options.each(function(model) {
      menu += Drupal.theme('acquiaLiftPreviewOptionMenuItem', {
        id: model.get('option_id'),
        label: model.get('option_label'),
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
  Drupal.theme.acquiaLiftPreviewOptionMenuItem = function (options) {
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
   * Themes a list item for a page variation within the list of variations.
   *
   * @param object variation
   *   The variation details including:
   *   - agent: the name of the agent/campaign for this variation
   *   - options: an array of variation options
   *   - index: the variation index value
   *   - label: the variation label
   */
  Drupal.theme.acquiaLiftPreviewPageVariationMenuItem = function (variation) {
    var item = '';
    var hrefOptions = [];
    _.each(variation.options, function (option, index, list) {
      hrefOptions.push({
        osID: option.osid,
        id: option.option.option_id
      });
    });
    var attrs = [
      'class="acquia-lift-preview-option acquia-lift-preview-page-variation--' + variation.index + ' visitor-actions-ui-ignore"',
      'href="' + generateHref(hrefOptions) + '"',
      'data-acquia-lift-personalize-page-variation="' + variation.index + '"',
      'aria-role="button"',
      'aria-pressed="false"'
    ];

    var renameHref = Drupal.settings.basePath + 'admin/structure/acquia_lift/pagevariation/rename/' + variation.agent + '/' + variation.index + '/nojs';
    var renameAttrs = [
      'class="acquia-lift-variation-rename acquia-lift-menu-link ctools-use-modal ctools-modal-acquia-lift-style-short"',
      'title="' + Drupal.t('Rename Variation #@num', {'@num': variation.index}) + '"',
      'aria-role="button"',
      'aria-pressed="false"',
      'href="' + renameHref + '"'
    ];

    item += '<li>\n<div class="acquia-lift-menu-item">';
    item += '<a ' + attrs.join(' ') + '>' + Drupal.checkPlain(variation.label) + '</a> \n';
    if (variation.index > 0) {
      item += '<a ' + renameAttrs.join(' ') + '>' + Drupal.t('Rename') + '</a>\n';
    }
    item += '</div>';

    return item;
  }

  /**
   * Themes a list item for a new page variation that has not yet been saved.
   *
   * @param variation_number
   *   The number to display for this variation.  -1 is passed to indicate a
   *   temporary control variation option display.
   */
  Drupal.theme.acquiaLiftNewVariationMenuItem = function (variation_number) {
    var isControl = variation_number == -1;
    var attrs = [
      'class="acquia-lift-preview-option acquia-lift-page-variation-new',
      'aria-role="button"',
      'aria-pressed="false"'
    ];
    var variationLabel = isControl ? Drupal.t(Drupal.settings.personalize.controlOptionLabel) : Drupal.t('Variation #@varnum', {'@varnum': variation_number});
    if (isControl) {
      attrs.push('data-acquia-lift-personalize-page-variation="control"');
    } else {
      attrs.push('data-acquia-lift-personalize-page-variation="new"');
    }

    var item = '';
    item += '<li>\n<a ' + attrs.join(' ') + '>\n';
    item += variationLabel + '\n';
    item += '</a>\n</li>\n';

    return item;
  }


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
  Drupal.theme.acquiaLiftCount = function (options) {
    return '<i class="acquia-lift-personalize-type-count acquia-lift-empty"><span>0</span>&nbsp;</i>';
  };

  /**
   * Returns the HTML for the selected campaign context label.
   */
  Drupal.theme.acquiaLiftCampaignContext = function (options) {
    var label = Drupal.t('Campaign: ');
    label += '<span class="acquia-lift-active">' + options.label + '</span>';
    return label;
  }

  /**
   * Returns the HTML for the page variation edit toggle link.
   */
  Drupal.theme.acquiaLiftPageVariationToggle = function () {
    var label = Drupal.t('Toggle variation mode');
    return '<a class="acquia-lift-page-variation-toggle">' + label + '</a>';
  }

  /**
   * Throbber.
   */
  Drupal.theme.acquiaLiftThrobber = function () {
    return '<div class="ajax-progress ajax-progress-throbber"><div class="throbber">&nbsp;</div></div>';
  };

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

}(Drupal, jQuery, _, Backbone));
