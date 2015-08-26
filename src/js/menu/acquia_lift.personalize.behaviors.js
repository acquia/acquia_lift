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

  Drupal.behaviors.acquiaLiftAdditionalMenuItems = {
    attach: function(context, settings) {
      // Adjust the debugger link if included in the unibar.
      $('#navbar-administration .acquia-lift-debugger').once('acquiaLiftDebugLink', function() {
        // Adjust the link to toggle the debugger.  This cannot be done via
        // Drupal because the menu functionality cannot accomodate query
        // parameters.
        var debuggerRunning = $(this).attr('data-acquia-lift-debugger-running') == 'true' ? true : false;
        var toggle = debuggerRunning ? 0 : 1;
        var href = $(this).attr('href');
        $(this).attr('href', href + '?acquia_lift_inspect_mode=' + toggle);
        $(this).closest('li').addClass('acquia-lift-navbar-secondary');
      });

      // Any secondary link should also be moved into a container that can
      // be placed
      $('#navbar-administration a.acquia-lift-navbar-secondary').once('acquiaLiftNavbarSecondary', function() {
        $(this).closest('li').addClass('acquia-lift-navbar-secondary');
        // Get the first of the previous siblings that isn't a secondary navigation and mark it with a CSS selector.
        $(this).closest('li').prevAll().not('.acquia-lift-navbar-secondary').first().addClass('acquia-lift-navbar-marker');
      });
    }
  };

}(Drupal, Drupal.jQuery, _));
