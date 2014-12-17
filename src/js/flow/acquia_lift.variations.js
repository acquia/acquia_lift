/**
 * @file acquia_lift.variations.js
 *
 * General functionality required for all acquia_lift.variations application
 * components.
 */

/**
 * @file acquia_lift.elements.js
 */
(function($, Drupal) {

  Drupal.acquiaLiftVariations = Drupal.acquiaLiftVariations || {};
  Drupal.acquiaLiftVariations.app = Drupal.acquiaLiftVariations.app || {};

  /**
   * Gets a jQuery element array of all elements available for the DOM
   * selector.
   */
  Drupal.acquiaLiftVariations.getAvailableElements = function () {
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
  };

  /**
   * A command to trigger the page element selection process.
   *
   * The response should include a data object with the following keys:
   * - start: Boolean indicating if page variation mode should be on (true)
   *   or off (false).
   * - type: Indicates the type of variation mode: one of 'page' or 'element'.
   * - variationIndex: The variation index to edit.  This can be an existing
   *   variation index to edit, or -1 to create a new variation.
   */
  Drupal.ajax.prototype.commands.acquia_lift_variation_toggle = function (ajax, response, status) {
    if (response.data.start) {
      initializeApplication();
      // Set the model to page variation mode and set up the relevant data.
      var editVariation = response.data.variationIndex || -1;
      Drupal.acquiaLiftVariations.app.appModel.setModelMode(response.data.type === 'page');
      Drupal.acquiaLiftVariations.app.appModel.set('variationIndex', editVariation);
      Drupal.acquiaLiftVariations.app.appModel.set('editMode', true);
      // Notify that the mode has actually been changed.
      response.data.variationIndex = editVariation;
    } else {
      // End editing for the application.
      if (Drupal.acquiaLiftVariations.app.appModel) {
        Drupal.acquiaLiftVariations.app.appModel.set('editMode', false);
      }
    }
    // Let the other menu stuff clear out before we set a new variation mode.
    response.data.campaign = Drupal.settings.personalize.activeCampaign;
    _.defer(function () {
      $(document).trigger('acquiaLiftVariationMode', [response.data]);
    });
  };

  /**
   * A command to open a particular selector details form either to edit
   * an existing option or to add a new option to an existing option set on the
   * same selector/variation type.
   *
   * The response should include a data object with the following keys:
   * - type: Indicates the type of variation mode: one of 'page' or 'element'.
   * - variationType: The type of variation, e.g., editText, addClass, etc.
   * - selector: The selector for the affected DOM element.
   * If type == page:
   * - variationIndex:  The variation index to edit.  A variationIndex of -1
   *   indicates creating a new variation.
   * If type == element
   * - osid: (optional) the option set id of an existing option set that is
   *   being modified either by adding a variation or by editing a variation
   *   within.
   */
  Drupal.ajax.prototype.commands.acquia_lift_variation_edit = function (ajax, response, status) {
    var data = response.data || {}, $selector = null;
    // Validate selector.
    try {
      if (data.selector) {
        var $selector = $(data.selector);
        // If the selector is not a unique match, then this can't proceed.
        // @todo: Log this using debugger tool.
        if ($selector.length !== 1) {
          return;
        }
      }
    } catch (err) {
      // @todo: Log this using debugger tool.
      // Selector is not correctly formatted.
      return;
    }
    // Validate variation type.
    if (!Drupal.settings.personalize_elements.contextualVariationTypes.hasOwnProperty(data.variationType)) {
      return;
    }
    var variationTypeData = Drupal.settings.personalize_elements.contextualVariationTypes[data.variationType];

    // Set up application.
    initializeApplication();
    var editVariation = response.data.variationIndex || -1;
    Drupal.acquiaLiftVariations.app.appModel.setModelMode(response.data.type === 'page');
    Drupal.acquiaLiftVariations.app.appModel.set('variationIndex', editVariation);
    Drupal.acquiaLiftVariations.app.appModel.set('editMode', true);

    // Generate required event data for details form.
    var editEvent = {};
    editEvent.data = {
      anchor: $selector.get(0),
      id: data.variationType,
      limitByChildrenType: variationTypeData.limitByChildrenType,
      name: variationTypeData.name,
      selector: data.selector
    };
    if (data.osid) {
      editEvent.data.osid = data.osid;
    }

    // Open the view.
    Drupal.acquiaLiftVariations.app.appView.createVariationTypeDialog(editEvent);
  }

  /**
   * Helper function to initialize the application.
   */
  function initializeApplication() {
    // Initialize Backbone application.
    if (!Drupal.acquiaLiftVariations.app.appModel) {
      Drupal.acquiaLiftVariations.app.appModel = new Drupal.acquiaLiftVariations.models.AppModel();
    }
    if (!Drupal.acquiaLiftVariations.app.appView) {
      Drupal.acquiaLiftVariations.app.appView = new Drupal.acquiaLiftVariations.views.AppView({
        model: Drupal.acquiaLiftVariations.app.appModel,
        $el: $('body')
      });
    }
  }

  /**
   * Add an event listener for a page variation mode trigger request.
   *
   * This utilizes the custom toggle command in order to allow front-end and
   * back-end requests for the functionality to be handled the same way.
   */
  $(document).on('acquiaLiftPageVariationModeTrigger', function(e, data) {
    data['type'] = 'page';
    var response = {
      data: data
    };
    Drupal.ajax.prototype.commands.acquia_lift_variation_toggle(Drupal.ajax, response, 200);
  });

  /**
   * Add an event listener for an element variation set mode trigger request.
   *
   * This utilizes the custom toggle command in order to allow front-end
   * and back-end requests for the functionality to be handled the same way.
   */
  $(document).on('acquiaLiftElementVariationModeTrigger', function(e, data) {
    data['type'] = 'element';
    var response = {
      data: data
    };
    Drupal.ajax.prototype.commands.acquia_lift_variation_toggle(Drupal.ajax, response, 200);
  });

  /**
   * Add an event listener to open up a specific variation type details form
   * on a specific element in order to add an element variation.
   *
   * Data is an object with the following keys:
   * - variationType: The type of variation, e.g., editText, addClass, etc.
   * - selector: The selector for the affected DOM element.
   * - osid: The option set id for the parent option set.

   */
  $(document).on('acquiaLiftElementVariationAdd', function(e, data) {
    data['type'] = 'element';
    var response = {
      data: data
    };
    Drupal.ajax.prototype.commands.acquia_lift_variation_edit(Drupal.ajax, response, 200);
  });


}(Drupal.jQuery, Drupal));

