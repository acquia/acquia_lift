/**
 * @file
 * acquia_lift.toolbar.unified_navbar.js
 *
 * Provides functionality to integrate the "toolbar" module with the Acquia
 * Lift unified navigation bar.
 */
(function (Drupal, $, debounce) {
  Drupal.navbar = Drupal.navbar || {};
  Drupal.behaviors.acquiaLiftUnifiedNavbarIntegration = {
    attach: function (context, settings) {

      // Make the "Acquia Lift" top level navigation item toggle the
      // unified navbar.
      var $anchor = this.getAdminMenu().once('acquiaLiftOverride').find('a[href~="/admin/acquia_lift"]');
      if ($anchor.length && Drupal.navbar.hasOwnProperty('toggleUnifiedNavbar')) {
        $anchor.bind('click.acquiaLiftOverride', function (event) {
          event.stopPropagation();
          event.preventDefault();
          // Toggle the Acquia Lift unified navigation.
          Drupal.navbar.toggleUnifiedNavbar();
        });
      }

      // Update the padding offset of the unified navbar when the toolbar
      // height changes.
      $(window).bind('resize.acquiaLiftToolbarResize', debounce(this.updateUnifiedToolbarPosition, 200));
      // Call it once to set the initial position.
      this.updateUnifiedToolbarPosition();
    },

    /**
     * Called when the window resizes to recalculate the placement of the
     * unified toolbar beneath the main toolbar (which could have resized).
     *
     * Note that only the window broadcast resize events (not divs).
     * @param e
     *   The event object.
     */
    updateUnifiedToolbarPosition: function(e) {
      var heightCss = self.getAdminMenu().css('height');
      $('body #toolbar + #navbar-administration.navbar-oriented .navbar-tray').css('top', heightCss);
    },

    // Helper method to get admin menu container,
    // override it to provide support of custom admin menu.
    // By default supports https://drupal.org/project/admin_menu, https://drupal.org/project/admin
    // and default toolbar core module.
    // https://drupal.org/project/navbar is supported @see acquia_lift.navbar.js
    getAdminMenu: function () {
      return $('#admin-toolbar, #admin-menu, #toolbar');
    }
  }
  var self = Drupal.behaviors.acquiaLiftUnifiedNavbarIntegration;

}(Drupal, jQuery, Drupal.debounce));
