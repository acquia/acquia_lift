/**
 * @file
 * acquia_lift.admin_menu.unified_navbar.js
 *
 * Provides functionality to integrate the "admin_menu" module with the Acquia
 * Lift unified navigation bar.
 */
(function (Drupal, $, debounce, displace) {
  Drupal.navbar = Drupal.navbar || {};
  Drupal.behaviors.acquiaLiftUnifiedNavbarIntegration = {
    attach: function (context, settings) {
      var self = this;
      // Make the "Acquia Lift" top level navigation item toggle the
      // unified navbar.
      $('#navbar-administration').once('acquiaLiftAdminMenuIntegration', function() {
        Drupal.navbar.calculateDynamicOffset(false);
        var adminMenu = self.getAdminMenu();
        if (adminMenu.length == 0) {
          // The admin_menu does not broadcast when it is added and does not
          // re-load Drupal behaviors.  It does, however, trigger a window resize
          // event when loaded in order to adjust placement.
          $(window).bind('resize.AcquiaLiftAdminMenuWait', self.checkForAdminMenu);
          return;
        }
        // If the admin menu is loaded then go ahead and add the unified navbar.
        self.checkForAdminMenu();
      });
    },

    /**
     * Attach the Acquia Lift navbar listeners once the admin menu is present.
     *
     * @param e
     * @param dispatch
     */
    checkForAdminMenu: function (e) {
      var adminMenu = self.getAdminMenu();
      if (adminMenu.length == 0) {
        return;
      }
      $(window).unbind('resize.AcquiaLiftAdminMenuWait', self.checkForAdminMenu);
      var $anchor = adminMenu.find('a[href~="/admin/acquia_lift"]');
      if ($anchor.length && Drupal.navbar.hasOwnProperty('toggleUnifiedNavbar')) {
        $anchor.bind('click.acquiaLiftOverride', function (event) {
          event.stopPropagation();
          event.preventDefault();
          // Toggle the Acquia Lift unified navigation.
          Drupal.navbar.toggleUnifiedNavbar();
          self.updateUnifiedToolbarPosition(null, false);
        });
      }
      // Update the padding offset of the unified navbar when the admin_menu
      // height changes or re-orients.
      $(window).bind('resize.acquiaLiftAdminMenuResize', debounce(self.updateUnifiedToolbarPosition, 200));
      $(document).bind('drupalNavbarOrientationChange', self.updateUnifiedToolbarPosition);
      // Call it once to set the initial position.
      self.updateUnifiedToolbarPosition(null, false);
    },

    /**
     * Called when the window resizes to recalculate the placement of the
     * unified toolbar beneath the main toolbar (which could have resized).
     *
     * Note that only the window broadcast resize events (not divs).
     *
     * @param e
     *   The event object.
     * @param dispatch
     *   True to dispatch displacement changes, false to ignore.
     */
    updateUnifiedToolbarPosition: function(e, dispatch) {
      var heightCss = self.getAdminMenu().css('height');
      var $horizontal = $('div#navbar-item-tray.navbar-tray-acquia-lift.navbar-tray.navbar-active.navbar-tray-horizontal');
      var $vertical = $('div#navbar-item-tray.navbar-tray-acquia-lift.navbar-tray.navbar-active.navbar-tray-vertical');
      // @todo: doesn't seem right to adjust all three.
      $('body.navbar-horizontal #navbar-administration.navbar-oriented').css('top', heightCss);
      //$('body.navbar-horizontal #navbar-administration.navbar-oriented .navbar-bar').css('top', heightCss);
      $('body #navbar-administration.navbar-oriented .navbar-tray').css('top', heightCss);
      // Because the admin_menu is positioned via margins we need to ignore
      // it when specifying the displacement for body content and explicitly
      // define the displacement top based on the size of the unified navbar.
      $('#navbar-bar.navbar-bar').attr('data-offset-top', 0);
      if ($horizontal.length > 0) {
        $horizontal.attr('data-offset-top', $horizontal.height());
        console.log('set tray top offset to ' + $horizontal.height());
      } else if ($vertical.length > 0) {
        $vertical.removeAttr('data-offset-top');
        console.log('removing tray offset');
      }
      dispatch = typeof(dispatch) == 'undefined' ? true : dispatch;
      if (dispatch) {
        displace(true);
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
  var self = Drupal.behaviors.acquiaLiftUnifiedNavbarIntegration;

}(Drupal, jQuery, Drupal.debounce, Drupal.displace));
