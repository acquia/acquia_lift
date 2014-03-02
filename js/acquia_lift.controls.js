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
    var $anchor = $('#toolbar-link-admin-acquia_lift').once('acquiaLiftOverride')

    if ($anchor.length) {
      $anchor.bind('click.acquiaLiftOverride', function (event) {
        event.stopPropagation();
        event.preventDefault();
      });
      attachControlsLoadHandler();
    }
  }
}

/*
 * Attaches a click handler to the Acquia Lift link in the Toolbar.
 */
function attachControlsLoadHandler () {
  $('#toolbar-link-admin-acquia_lift').once('acquia-lift-controls').bind('click.acquia-lift', function (event) {
      loadAssets($(this), function ($controls) {
        $controls
          .dialog({
            title: Drupal.t('Acquia Lift controls'),
            close: function () {
              $(this).dialog('destroy').remove();
              attachControlsLoadHandler();
            },
            open: function () {
              Drupal.attachBehaviors();
            },
            minWidth: 360,
            dialogClass: 'acquia-lift-controls-dialog'
          })
          .show();
      });
    });
}

/**
 * Detaches the click handler on the Acquia Lift link in the Toolbar.
 */
function detachControlsLoadHandler () {
  $('#toolbar-link-admin-acquia_lift').unbind('click.acquia-lift').removeOnce('acquia-lift-controls');
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
