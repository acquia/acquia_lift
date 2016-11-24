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
    }

    /**
     * Attaches the JS test behavior to to weight div.
     */
    Drupal.behaviors.acquiaLiftOpenExperienceBuilder = {
        attach: function (context, settings) {
            $('#openLiftLink').on('click', function() {
                openExperienceBuilder();
                // Do not act on the href link.
                return false;
            });
        }
    };
})(jQuery, Drupal);
