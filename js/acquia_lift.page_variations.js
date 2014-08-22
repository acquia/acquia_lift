/**
 * @file acquia_lift.elements.js
 */
(function($, Drupal, Dialog, Backbone, _) {

  /**
   * Gets a jQuery element array of all elements available for the DOM
   * selector.
   */
  function getAvailableElements() {
    var ignoreRegions = Drupal.settings.acquia_lift.dom_selector_ignore;

    // Reduce the ignore region class list to a selector that includes
    // each region and all of its children, for example:
    // .page-top, .page-top *, .page-bottom, .page-bottom *
    var ignoreSelector = _.reduce(ignoreRegions, function (memo, current) {
      if (memo.length > 0) {
        memo += ', ';
      }
      return memo + '.' + current + ', .' + current + ' *';
    }, '');
    var $available = $('body').find('*').not(ignoreSelector).not('script, br, wbr, noscript').filter(function () {
      var el = this;
      var id = el.id || '';
      var className = typeof this.className === 'string' && this.className || '';
      var href = this.attributes['href'] && this.attributes['href'].value || '';
      // Eliminate any visitor actions components.
      var rVA = /^visitor-actions/;
      // Eliminate local tasks and contextual links.
      var rTask = /local-task|contextual/;
      // Eliminate admin links.
      var rAdmin = /^\/?admin/;
      // Eliminate node action links.
      var rNode = /^\/?node(\/)?(\d)*(?=\/add|\/edit|\/delete)/;
      // Reject the element if any tests match.
      if (rVA.test(id) || rTask.test(className) || rAdmin.test(href) || rNode.test(href)) {
        return false;
      }
      // Keep the element as the default.
      return true;
    })
    return $available;
  }


  Drupal.acquiaLiftPageVariations = Drupal.acquiaLiftPageVariations || {};
  Drupal.acquiaLiftPageVariations.app = Drupal.acquiaLiftPageVariations.app || {};

  /*******************************************************************
   * M O D E L S
   *******************************************************************/
  Drupal.acquiaLiftPageVariations.models = Drupal.acquiaLiftPageVariations.models || {
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
    ElementVariationModel: Backbone.Model.extend({}),


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

  /*******************************************************************
   * C O L L E C T I O N S
   *******************************************************************/
  Drupal.acquiaLiftPageVariations.collections = Drupal.acquiaLiftPageVariations.collections || {
    ElementVariationCollection: Backbone.Collection.extend({
      model: Drupal.acquiaLiftPageVariations.models.ElementVariationModel
    })
  };

  /*******************************************************************
   * V I E W S
   *******************************************************************/
  Drupal.acquiaLiftPageVariations.views = Drupal.acquiaLiftPageVariations.views || {
    /**
     * Backbone View for the full page variation flow.
     */
    AppView: Backbone.View.extend({
      contextualMenuModel: null,
      variationTypeFormModel: null,
      anchor: null,
      // An array of jQuery instances that are available to the DOM selector.
      $watchElements: null,

      initialize: function (options) {
        _.bindAll(this, 'createContextualMenu', 'onElementSelected');

        var that = this;
        this.$watchElements = options.$watchElements;
        this.$el.DOMSelector({
          $watchElements: this.$watchElements,
          onElementSelect: function (element, selector) {
            that.onElementSelected(element, selector);
          }
        });
        Backbone.on('acquiaLiftPageVariationType', this.createVariationTypeDialog, this);
        this.listenTo(this.model, 'change:editMode', this.render);
        this.listenTo(this.model, 'change:editMode', this.updateEditMode);
        this.render(this.model, this.model.get('editMode'));
      },

      /**
       * {@inheritDoc}
       */
      render: function (model, editMode) {
        if (editMode) {
          this.$el.DOMSelector("startWatching");
        } else {
          this.$el.DOMSelector("stopWatching");
        }
      },

      /**
       * Handles showing/hiding a highlight around the anchoring element.
       * @param bool show
       *   True if showing the highlight, false if no highlight should be shown.
       */
      highlightAnchor: function(show) {
        var highlightClass = 'acquia-lift-page-variation-item';
        if (!this.anchor) {
          return;
        }
        if (show) {
          $(this.anchor).addClass(highlightClass);
        } else {
          $(this.anchor).removeClass(highlightClass);
        }
      },

      /**
       * Updates the application based on changes in edit mode in model.
       */
      updateEditMode: function(model, editMode) {
        var data = {};
        var variationIndex = model.get('variationIndex');
        if (editMode) {
          variationIndex = isNaN(variationIndex) ? -1 : variationIndex;
          if (this.contextualMenuModel) {
            this.contextualMenuModel.set('active', true);
          }
          if (this.variationTypeFormModel) {
            this.variationTypeFormModel.set('active', true);
          }
          data.started = editMode;
          data.mode = (variationIndex == -1) ? 'add' : 'edit';
          data.campaign = Drupal.settings.personalize.activeCampaign;
          data.variationIndex = variationIndex;
          $(document).trigger('acquiaLiftPageVariationsMode', data);
        } else {
          this.highlightAnchor(false);
          if (this.contextualMenuModel) {
            this.contextualMenuModel.destroy();
            this.contextualMenuModel = null;
          }
          if (this.variationTypeFormModel) {
            this.variationTypeFormModel.destroy();
            this.variationTypeFormModel = null;
          }
        }
      },

      /**
       * Deactivates the view and the page variation process.
       */
      deactivate: function () {
        this.$watchElements.DOMSelector("stopWatching");
      },

      /**
       * Event callback for when an element is selected in the DOM selector.
       */
      onElementSelected: function (element, selector) {
        this.$el.DOMSelector('stopWatching');
        this.createContextualMenu(element, selector);
      },

      /**
       * Creates a contextual page variation selection menu at the specified
       * element.
       */
      createContextualMenu: function (element, selector) {
        this.anchor = element;
        this.highlightAnchor(true);
        this.contextualMenuModel = new Drupal.visitorActions.ui.dialog.models.DialogModel({
          selector: selector,
          id: this.getTemporaryID()
        });
        var dialogView = new Drupal.acquiaLiftPageVariations.views.PageVariationMenuView({
          el: element,
          model: this.contextualMenuModel
        });
        this.contextualMenuModel.set('active', this.model.get('editMode'));
      },

      /**
       * Creates a variation type dialog for a specific variation type based on
       * the type selected.
       *
       * @param event
       *   The triggering event that includes the model data/JSON for the selected
       *   ElementVariationModel.
       */
      createVariationTypeDialog: function(event) {
        var formPath = Drupal.settings.basePath +
          'admin/structure/acquia_lift/pagevariation/' +
          Drupal.encodePath(event.data.id);
        this.variationTypeFormModel = new Drupal.acquiaLiftPageVariations.models.VariationTypeFormModel({
          selector: event.data.selector,
          id: this.getTemporaryID(),
          formPath: formPath,
          type: event.data.id,
          typeLabel: event.data.name,
          variationIndex: this.model.get('variationIndex')
        });
        var dialogView = new Drupal.acquiaLiftPageVariations.views.VariationTypeFormView({
          el: event.data.anchor,
          model: this.variationTypeFormModel
        });
        this.variationTypeFormModel.set('active', this.model.get('editMode'));
      },

      /**
       * Generates a page-level temporary unique identifier.
       */
      getTemporaryID: function() {
        return 'acquiaLiftPageVariations-' + new Date().getTime();
      }
    }),

    /**
     * Backbone view that displays the form to enter the value for a new
     * page variation of a specific variation type.
     */
    VariationTypeFormView: Dialog.views.ElementDialogView.extend({
      className: 'acquia-lift-variation-type-form',
      /**
       * {@inheritDoc}
       */
      initialize: function (options) {
        options.myVerticalEdge = 'top';
        options.anchorVerticalEdge = 'bottom';
        this.parent('inherit', options);
      },

      /**
       * {@inheritDoc}
       */
      render: function(model, active) {
        var that = this;
        this.parent('render', model, active);
        // Add a title to this dialog.
        var title = Drupal.theme('acquiaLiftPageVariationsTypeFormTitle', {
          variationType: this.model.get('typeLabel'),
          elementType: this.anchor.nodeName
        });
        this.$el.find('.visitor-actions-ui-dialog-content').prepend($(title));
      },

      /**
       * {@inheritDoc}
       */
      formSuccessHandler: function (ajax, response, status) {
        this.parent('formSuccessHandler', ajax, response, status);
        this.$el.find('[name="selector"]').val(this.model.get('selector'));
        this.$el.find('[name="pages"]').val(Drupal.settings.visitor_actions.currentPath);
        this.$el.find('[name="agent"]').val(Drupal.settings.personalize.activeCampaign);
        this.$el.find('[name="variation_number"]').val(this.model.get('variationIndex'));
      },

      /**
       * Completely remove children and unbind events.
       */
      remove: function() {
        this.unbind();
        Backbone.View.prototype.remove.call(this);
      }
    }),

    /**
     * Contextual menu view to allow selection of the type of variation to
     * create.
     */
    PageVariationMenuView: Dialog.views.ElementDialogView.extend({
      className: 'acquia-lift-context-menu',

      /**
       * {@inheritDoc}
       */
      initialize: function (options) {
        this.parent('inherit', options);
        Backbone.on('acquiaLiftPageVariationTypeSelected', this.onVariationTypeSelected, this);
        this.list = null;
      },

      /**
       * {@inheritDoc}
       */
      render: function (model, active) {
        var that = this;
        this.parent('render', model, active);
        // Generate the contextual menu HTML.
        var titleHtml = Drupal.theme('acquiaLiftPageVariationsMenuTitle', {
          elementType: this.anchor.nodeName
        });

        // Generate the collection of options.
        var collection = new Drupal.acquiaLiftPageVariations.collections.ElementVariationCollection();
        var modelAttributes = _.map(Drupal.settings.personalize_elements.contextualVariationTypes, function(label, type) {
          return {id: type, name: label}
        });
        collection.add(modelAttributes);
        this.list = new Drupal.acquiaLiftPageVariations.views.PageVariationMenuListView({collection: collection});
        this.list.render();
        this.$el.find('.visitor-actions-ui-dialog-content').html(titleHtml).append(this.list.el);
        this.position(function () {
          that.show();
        });
      },

      /**
       * Called when the user selects a variation type.
       *
       * Closes the contextual menu and removes it as it is no longer needed.
       * It also triggers a new event that includes the variation type data
       * from the original event plus the anchor information from this menu.
       */
      onVariationTypeSelected: function(event) {
        event.data.anchor = this.anchor;
        event.data.selector = this.model.get('selector');
        this.remove();
        Backbone.trigger('acquiaLiftPageVariationType', {data: event.data});
      },

      /**
       * Completely remove children and unbind events.
       */
      remove: function() {
        if (this.list) {
          this.list.remove();
        }
        Backbone.off('acquiaLiftPageVariationTypeSelected');
        this.unbind();
        Backbone.View.prototype.remove.call(this);
      }
    }),

    /**
     * A view for the list of variation options presented within the contextual
     * menu.
     */
    PageVariationMenuListView: Backbone.View.extend({
      tagName: 'ul',
      className: 'acquia-lift-page-variation-list',

      /**
       * {@inheritDoc}
       */
      initialize: function (options) {
        _.bindAll(this, 'renderItem');
        this.subviews = [];
      },

      /**
       * Renders a single page variation menu item.
       */
      renderItem: function (model) {
        var itemView = new Drupal.acquiaLiftPageVariations.views.PageVariationMenuListItemView({model: model});
        itemView.render();
        this.$el.append(itemView.el);
        this.subviews.push(itemView);
      },

      /**
       * {@inheritDoc}
       */
      render: function () {
        this.collection.each(this.renderItem);
      },

      /**
       * Completely remove children and unbind events.
       */
      remove: function() {
        _.invoke(this.subviews, 'remove');
        this.unbind();
        Backbone.View.prototype.remove.call(this);
      }
    }),

    /**
     * Backbone view for a single variation option presented within the
     * contextual menu.
     */
    PageVariationMenuListItemView: Backbone.View.extend({
      tagName: 'li',

      /**
       * {@inheritDoc}
       */
      initialize: function(options) {
        _.bindAll(this, 'clicked');
      },

      /**
       * Event definitions: defines click handler when a variation type link
       * is clicked.
       */
      events: {
        "click a": "clicked"
      },

      /**
       * Event handler when a menu list item is clicked.
       * @param e
       */
      clicked: function (e){
        e.preventDefault();
        Backbone.trigger('acquiaLiftPageVariationTypeSelected', {data: this.model.toJSON()});
      },

      /**
       * {@inheritDoc}
       */
      render: function(){
        var html = Drupal.theme('acquiaLiftPageVariationsMenuItem', this.model.toJSON());
        this.$el.append(html);
      },

      /**
       * Completely remove children and unbind events.
       */
      remove: function() {
        this.unbind();
        Backbone.View.prototype.remove.call(this);
      }
    })
  }

  /**
   * Theme function to generate the title for a page variations contextual menu.
   * @param options
   *   An object of options with a key for elementType.
   */
  Drupal.theme.acquiaLiftPageVariationsMenuTitle = function (options) {
    return '<h2>&lt;' + options.elementType + ' &gt;</h2>';
  }

  /**
   * A theme function to generate the HTML for a single menu item link.
   *
   * @param object item
   *   An object with the following keys:
   *   - id: The type of menu option
   *   - name:  The label to display for this menu option
   */
  Drupal.theme.acquiaLiftPageVariationsMenuItem = function (item) {
    return '<a href="#" data-id="' + item.id + '">' + item.name + '</a>';
  }

  /**
   * Theme function to generate the title element for a variation type form.
   *
   * @param object item
   *   An object with the following keys:
   *   - elementType: the type of element that is being action on.
   *   - variationType: the type of variation to apply to the element.
   */
  Drupal.theme.acquiaLiftPageVariationsTypeFormTitle = function (item) {
    return '<h2>' + item.variationType + ': ' + '&lt;' + item.elementType + '&gt;</h2>';
  }

  /**
   * A command to trigger the page element selection process.
   *
   * The response should include a data object with the following keys:
   * - start: Boolean indicating if page variation mode should be on (true)
   *   or off (false).
   * - variationIndex: The variation index to edit.  This can be an existing
   *   variation index to edit, or -1 to create a new variation.
   */
  Drupal.ajax.prototype.commands.acquia_lift_page_variation_toggle = function (ajax, response, status) {
    if (response.data.start) {
      if (!Drupal.acquiaLiftPageVariations.app.appModel) {
        Drupal.acquiaLiftPageVariations.app.appModel = new Drupal.acquiaLiftPageVariations.models.AppModel();
      }
      if (!Drupal.acquiaLiftPageVariations.app.appView) {
        var $elements = getAvailableElements();
        if ($elements.length > 0) {
          Drupal.acquiaLiftPageVariations.app.appView = new Drupal.acquiaLiftPageVariations.views.AppView({
            model: Drupal.acquiaLiftPageVariations.app.appModel,
            $watchElements: $elements,
            $el: $('body')
          });
        }
      }
      var editVariation = response.data.variationIndex || -1;
      Drupal.acquiaLiftPageVariations.app.appModel.set('variationIndex', editVariation);
      Drupal.acquiaLiftPageVariations.app.appModel.set('editMode', true);
      // Notify that the mode has actually been changed.
      response.data.variationIndex = editVariation;
    } else {
      if (Drupal.acquiaLiftPageVariations.app.appModel) {
        Drupal.acquiaLiftPageVariations.app.appModel.set('editMode', false);
      }
    }
    response.data.campaign = Drupal.settings.personalize.activeCampaign;
    // Let the other menu stuff clear out before we set a new variation mode.
    _.defer(function () {
      $(document).trigger('acquiaLiftPageVariationMode', [response.data]);
    });
  };

  /**
   * Add an event listener for a page variation mode trigger request.
   *
   * This utilizes the custom toggle command in order to allow front-end and
   * back-end requests for the functionality to be handled the same way.
   */
  $(document).on('acquiaLiftPageVariationModeTrigger', function(e, data) {
    var response = {
      data: data
    };
    Drupal.ajax.prototype.commands.acquia_lift_page_variation_toggle(Drupal.ajax, response, 200);
  });

  }(jQuery, Drupal, Drupal.visitorActions.ui.dialog, Backbone, _));
