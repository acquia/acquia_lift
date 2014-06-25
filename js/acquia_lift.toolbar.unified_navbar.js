/**
 * @file
 * acquia_lift.toolbar.unified_navbar.js
 *
 * Provides functionality to integrate the "toolbar" module with the Acquia
 * Lift unified navigation bar.
 */
(function (Drupal, $) {
  Drupal.navbar = Drupal.navbar || {};
  Drupal.behaviors.acquiaLiftUnifiedNavbarIntegration = {
    attach: function (context, settings) {
      var $anchor = this.getAdminMenu().once('acquiaLiftOverride').find('a[href~="/admin/acquia_lift"]');

      if ($anchor.length && Drupal.navbar.hasOwnProperty('toggleUnifiedNavbar')) {
        $anchor.bind('click.acquiaLiftOverride', function (event) {
          event.stopPropagation();
          event.preventDefault();
          // Toggle the Acquia Lift unified navigation.
          Drupal.navbar.toggleUnifiedNavbar();
        });
      }
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
}(Drupal, jQuery));
