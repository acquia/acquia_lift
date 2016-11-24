/**
 * @file
 * Contains the definition of the behaviour openAcquiaLiftExperienceBuilder.
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
      alert('Sorry, Acquia Lift could not be found');
    }
    // Do not act on the href link.
    return false;
  }

  /**
   * Attaches the JS test behavior to to weight div.
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
