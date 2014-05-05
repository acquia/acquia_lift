/**
 * @file
 * acquia_lift.controls.js
 *
 * Behaviors to provide Acquia Lift controls when Navbar is not available.
 */
(function ($) {

// The Drupal.ajax instance loading the assets.
var assetsLoader;

/**
 * Launches a dialog with personalization controls.
 */
Drupal.behaviors.acquiaLiftControls = {
  attach: function (context) {
    var $anchor = this.getAdminMenu().once('acquiaLiftOverride').find('a[href~="/admin/acquia_lift"]');

    if ($anchor.length) {
      $anchor.bind('click.acquiaLiftOverride', function (event) {
        event.stopPropagation();
        event.preventDefault();
      });
      $anchor.attachControlsLoadHandler();
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

/*
 * Attaches a click handler to the Acquia Lift link in the Toolbar.
 */
$.fn.attachControlsLoadHandler = function () {
    var $thisLink = $(this);
    $thisLink.once('acquia-lift-controls').bind('click.acquia-lift', function (event) {
      loadAssets($(this), function ($controls) {
        $controls
          .dialog({
            title: Drupal.t('Acquia Lift controls'),
            close: function () {
              $(this).dialog('destroy').remove();
              $thisLink.attachControlsLoadHandler();
            },
            open: function () {
              Drupal.attachBehaviors();
            },
            create: function (event) {
              $(event.target).parent().css('position', 'fixed');
            },
            resizeStart: function (event) {
              $(event.target).parent().css('position', 'fixed');
            },
            resizeStop: function (event) {
              $(event.target).parent().css('position', 'fixed');
            },
            minWidth: 360,
            dialogClass: 'acquia-lift-controls-dialog'
          })
          .show();
      });
    });
}

/**
 * Adjusts the Acquia Lift controls markup to create an accordion pane
 * within the dialog of control options.
 */
Drupal.behaviors.acquiaLiftControlsDialog = {
  attach: function (context, settings) {
    // Organize the menu list into content suitable for an accordion control.
    $('.acquia-lift-controls-dialog', context).once().find('ul.menu:first').each(function() {
      // Mark the list items that should be the accordion headers.
      $(this).children('li').children('a').wrap('<h3></h3>');
      $(this).children('li').children('h3').addClass('acquia-lift-accordion-header');

      // Add links in for those that aren't repeated within the nested list.
      var $allCampaigns = $('li a.acquia-lift-campaign-list', this);
      $('li a.acquia-lift-campaign-list', this).parent().next('ul').prepend('<li><a href="' + $allCampaigns.attr('href') + '">' + $allCampaigns.text() + '</a></li>');
      var $reports = $('li a.acquia-lift-results-list', this);
      $('li h3 a.acquia-lift-results-list', this).parent().after('<ul class="menu"><li><a href="' + $reports.attr('href') + '">' + $reports.text() + '</a></li></ul>');
      $('li > h3 > a', this).removeClass('acquia-lift-active');

      // Now turn the updated structure into an accordion.
      $(this).accordion({
        header: '.acquia-lift-accordion-header',
        heightStyle: "content",
        autoHeight: false,
        clearStyle: true,
        collapsible: true,
        active: false
      });
    });
  }
}

/**
 * Detaches the click handler on the Acquia Lift link in the Toolbar.
 */
function detachControlsLoadHandler () {
  Drupal.behaviors.acquiaLiftControls.getAdminMenu().find('a[href~="/admin/acquia_lift"]').unbind('click.acquia-lift').removeOnce('acquia-lift-controls');
}

/**
 * Loads the acquia_lift.preview.js file.
 *
 * @param jQuery $anchor
 *   The DOM node to use as an anchor for the placeholder that triggers the
 *   Drupal.ajax request.
 */
function loadAssets ($anchor, callback) {
  if (!assetsLoader) {
    // Load the acquia-lift preview assets.
    var id = 'acquia-lift-' + (new Date()).getTime();
    var $placeholder = $('<div id="' + id + '" style="display:none;"><div class="bd"></div></div>').appendTo($anchor.eq(0));

    /**
     * Override the Drupal.ajax error handler for the form redirection error.
     *
     * Remove the alert() call.
     */
    var ajaxError = Drupal.ajax.prototype.error;
    Drupal.ajax.prototype.error = function (response, uri) {
      // Remove the progress element.
      if (this.progress.element) {
        $(this.progress.element).remove();
      }
      if (this.progress.object) {
        this.progress.object.stopMonitoring();
      }
      // Undo hide.
      $(this.wrapper).show();
      // Re-enable the element.
      $(this.element).removeClass('progress-disabled').removeAttr('disabled');
      // Reattach behaviors, if they were detached in beforeSerialize().
      if (this.form) {
        var settings = response.settings || this.settings || Drupal.settings;
        Drupal.attachBehaviors(this.form, settings);
      }
    };

    Drupal.ajax[id] = new Drupal.ajax(id, $('#' + id), {
      url: Drupal.settings.basePath + 'acquia_lift/controls/assets',
      event: 'load.acquia-lift',
      wrapper: id + ' .bd',
      progress: {
        type: null
      },
      success: function (response, status) {
        Drupal.ajax.prototype.success.call(this, response, status);
        // Clean up this Drupal.ajax object.
        delete Drupal.ajax[id];
        $placeholder.unbind().remove();
        assetsLoader = null;
        detachControlsLoadHandler();
        callback.call(null, $placeholder);
      },
      error: function () {
        delete Drupal.ajax[id];
        $placeholder.unbind().remove();
        assetsLoader = null;
      },
      complete: function () {
        // Put the original Drupal.ajax error handler back.
        Drupal.ajax.prototype.error = ajaxError;
        ajaxError = null;
      }
    });
    // This placeholder only needs to trigger this event once, so unbind
    // the event immediately.
    $placeholder
      .trigger('load.acquia-lift');
  }
}
}(jQuery));
