/**
 * @file acquia_lift.elements.js
 */
(function($, Drupal, Dialog, Backbone, _) {

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
        editMode: true
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
    ElementVariationModel: Backbone.Model.extend({})
  }

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
      initialize: function (options) {
        _.bindAll(this, 'createContextualMenu');

        var that = this;
        this.$el.DOMSelector({
          onElementSelect: function (element, selector) {
            that.createContextualMenu(element, selector);
          }
        });
        Backbone.on('acquiaLiftPageVariationType', this.createVariationTypeDialog, this);
        this.listenTo(this.model, 'change:editMode', this.render);
        this.render(this.model, this.model.get('editMode'));
      },

      render: function (model, editMode) {
        if (editMode) {
          this.$el.DOMSelector("startWatching");
        } else {
          this.$el.DOMSelector("stopWatching");
        }
      },

      /**
       * Deactivates the view and the page variation process.
       */
      deactivate: function () {
        this.$el.DOMSelector("stopWatching");
      },

      /**
       * Creates a contextual page variation selection menu at the specified
       * element.
       */
      createContextualMenu: function (element, selector) {
        var dialogModel = new Drupal.visitorActions.ui.dialog.models.DialogModel({
          selector: selector,
          id: this.getTemporaryID()
        });
        var dialogView = new Drupal.acquiaLiftPageVariations.views.PageVariationMenuView({
          el: element,
          model: dialogModel
        });
        dialogModel.set('active', this.model.get('editMode'));
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
          Drupal.encodePath(event.data.id) +
          '?path=' + Drupal.encodePath(Drupal.settings.visitor_actions.currentPath);
        var dialogModel = new Drupal.visitorActions.ui.dialog.models.DialogModel({
          selector: event.data.selector,
          id: this.getTemporaryID(),
          formPath: formPath
        });
        var dialogView = new Drupal.acquiaLiftPageVariations.views.VariationTypeFormView({
          el: event.data.anchor,
          model: dialogModel
        });
        dialogModel.set('active', this.model.get('editMode'));
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

      /**
       * {@inheritDoc}
       */
      initialize: function (options) {
        options.myVerticalEdge = 'top';
        options.anchorVerticalEdge = 'bottom';
        options.delay = 100;
        options.collision = 'none'; // TODO: change to flipfit.
        this.parent('inherit', options);
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

      /**
       * {@inheritDoc}
       */
      initialize: function (options) {
        options.myHorizontalEdge = 'left';
        options.anchorHorizontalEdge = 'right';
        options.myVerticalEdge = 'top';
        options.anchorVerticalEdge = 'top';
        options.delay = 100;
        // @todo Need to figure out appropriate collision type, flipfit isn't
        // working in this scenario for some reason.
        options.collision = 'none';
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
        var html = Drupal.theme('acquiaLiftPageVariationsMenuTitle', {
          elementType: this.anchor.nodeName
        });
        // Add it to the dialog view.
        this.$el.find('.visitor-actions-ui-placeholder').html(html);

        // Generate the collection of options.
        var collection = new Drupal.acquiaLiftPageVariations.collections.ElementVariationCollection();
        var modelAttributes = _.map(Drupal.settings.personalize_elements.contextualVariationTypes, function(label, type) {
          return {id: type, name: label}
        });
        collection.add(modelAttributes);
        this.list = new Drupal.acquiaLiftPageVariations.views.PageVariationMenuListView({collection: collection});
        this.list.render();
        this.$el.append(this.list.el);
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
        Backbone.trigger('acquiaLiftPageVariationType', {data: event.data});
        this.remove();
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
    return '<h2>' + options.elementType + '</h2>';
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
   * A command to trigger the page element selection process.
   */
  Drupal.ajax.prototype.commands.acquia_lift_page_variation_toggle = function (ajax, response, status) {
    if (response.data.start) {
      if (!Drupal.acquiaLiftPageVariations.app.appModel) {
        Drupal.acquiaLiftPageVariations.app.appModel = new Drupal.acquiaLiftPageVariations.models.AppModel();
      }
      if (Drupal.acquiaLiftPageVariations.app.appView) {
        Drupal.acquiaLiftPageVariations.app.appView.deactivate();
      }
      var $wrapper = $(response.data.wrapper);
      if ($wrapper.length) {
        Drupal.acquiaLiftPageVariations.app.appView = new Drupal.acquiaLiftPageVariations.views.AppView({
          model: Drupal.acquiaLiftPageVariations.app.appModel,
          $el: $wrapper,
          el: $wrapper[0]
        });
      }
    } else {
      if (Drupal.acquiaLiftPageVariations.app.appModel) {
        Drupal.acquiaLiftPageVariations.app.appModel.set('editMode', false);
      }
    }
  };
}(jQuery, Drupal, Drupal.visitorActions.ui.dialog, Backbone, _));
