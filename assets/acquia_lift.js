/**
 * @file
 * Contains the definition of the behaviour openAcquiaLiftExperienceBuilder.
 */

(function ($, Drupal) {

    'use strict';

    /**
     * Attaches the JS test behavior to to weight div.
     */
    Drupal.behaviors.acquiaLiftOpenExperienceBuilder = {
        attach: function (context, settings) {
            $('.openLiftLink').click(function() {
                if (window.AcquiaLiftPublicApi) {
                    window.AcquiaLiftPublicApi.activate();
                } else {
                    alert('Sorry, Acquia Lift could not be found');
                }
            });
        }
    };
})(jQuery, Drupal);
