(function ($, Drupal, drupalSettings) {
  'use strict';

  var entityTypeId = 'node',
    username = 'admin',
    password = 'admin';
  fetch('/api/acquia-perz/uuids-slots/' + entityTypeId, {
      method: 'GET',
      headers: {
        "Content-Type": "application/hal+json",
        'Accept': 'application/json',
        'Authorization': 'Basic ' + window.btoa(username + ":" + password)
      }
    }
  )
  .then(r => r.json())
  .then(uuidsSlots => {
    for (const uuid in uuidsSlots) {
      var slotId = uuidsSlots[uuid];
      $('#' + slotId + ' .content').html(drupalSettings['decisionContent'][entityTypeId][uuid]);
    }
  });
})(jQuery, Drupal, drupalSettings);
