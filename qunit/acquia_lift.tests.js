/**
 * @file acquia_lift.tests.js
 */

QUnit.module("Acquia Lift Data Collection tests", {
  setup: function() {
  }
});

QUnit.test("Test generate tracking id", function(assert) {
  expect(3);
  var is_valid_uuid;

  // Test generate.
  Drupal.acquia_lift.generateTrackingId();
  var tracking_id_1 = drupalSettings.acquia_lift.pageContext.trackingId,
  is_valid_uuid = /^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$/.test(tracking_id_1);
  assert.ok(is_valid_uuid, 'A valid tracking id was generated.');

  // Test generate again.
  Drupal.acquia_lift.generateTrackingId();
  var tracking_id_2 = drupalSettings.acquia_lift.pageContext.trackingId,
  is_valid_uuid = /^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$/.test(tracking_id_2);
  assert.ok(is_valid_uuid, 'Another valid tracking id was generated.');

  // Make sure the function produces unique ids.
  assert.notEqual(tracking_id_1, tracking_id_2, 'The two tracking id\'s are different.');
});
