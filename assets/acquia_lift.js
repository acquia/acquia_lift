/**
 * @file
 * Contains the definition of the behavior openAcquiaLiftExperienceBuilder.
 */

(function ($, Drupal) {

  'use strict';

  /**
   * Opens the Acquia Lift Experience Builder, if it is available.
   */
  function openExperienceBuilder() {
    if (window.AcquiaLiftPublicApi) {
      window.AcquiaLiftPublicApi.activate();
    }
    else {
      alert('Acquia Lift Experience Builder could not be loaded. Please check Lift admin page and make sure your account credentials are already set. If this problem persists, please contact the Acquia Lift Team for support.');
    }
    // Do not act on the href link.
    return false;
  }

  /**
   * Attaches the JS behavior to the openLiftLink id in the toolbar.
   */
  Drupal.behaviors.acquiaLiftOpenExperienceBuilder = {
    attach: function (context, settings) {
      $(context)
        .find('.toolbar .toolbar-bar #openLiftLink')
        .once('acquia-lift-open-experience-builder')
        .on('click', openExperienceBuilder);
    }
  };
})(jQuery, Drupal);
