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
