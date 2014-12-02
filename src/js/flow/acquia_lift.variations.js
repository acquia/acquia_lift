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
  })


}(Drupal.jQuery, Drupal));

