(function ($, Drupal, drupalSettings) {
  'use strict';
  if (drupalSettings['uuidsSlotsUrl'] === undefined) {
    return;
  }
  var url = drupalSettings['uuidsSlotsUrl'],
    entityTypeId = 'node',
    username = 'admin',
    password = 'admin';
  fetch(url, {
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
      $('#' + uuidsSlots[uuid] + ' .content').html(drupalSettings['decisionContent'][entityTypeId][uuid]);
    }
  });
})(jQuery, Drupal, drupalSettings);
