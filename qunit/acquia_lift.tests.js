/**
 * @file acquia_lift.tests.js
 */

QUnit.module("Acquia Lift Data Collection tests", {
  setup: function() {
    drupalSettings.acquia_lift.credential.account_name = 'an_account_name';
    drupalSettings.acquia_lift.credential.customer_site = 'a_customer_site';
    drupalSettings.acquia_lift.pageContext.post_id = 90210;
    drupalSettings.acquia_lift.pageContext.content_keywords = 'foo,bar';
  },
  teardown: function() {
    _tcaq = [];
  }
});

QUnit.test("Test generate tracking id", function(assert) {
  expect(3);
  var is_valid_uuid;

  // Test generate.
  Drupal.acquia_lift.generateTrackingId();
  var tracking_id_1 = drupalSettings.acquia_lift.pageContext.trackingId;
  is_valid_uuid = /^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$/.test(tracking_id_1);
  assert.ok(is_valid_uuid, 'A valid tracking id was generated.');

  // Test generate again.
  Drupal.acquia_lift.generateTrackingId();
  var tracking_id_2 = drupalSettings.acquia_lift.pageContext.trackingId;
  is_valid_uuid = /^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$/.test(tracking_id_2);
  assert.ok(is_valid_uuid, 'Another valid tracking id was generated.');

  // Make sure the function produces unique ids.
  assert.notEqual(tracking_id_1, tracking_id_2, 'The two tracking id\'s are different.');
});

QUnit.test("Test populate _tcaq queue", function(assert) {
  expect(3);

  Drupal.acquia_lift.populateTcaqQueue();
  var expected_config_object = {
    'post_id': 90210,
    'content_keywords': 'foo,bar',
    'trackingId': drupalSettings.acquia_lift.pageContext.trackingId,
  }
  assert.deepEqual(['setAccount', 'an_account_name', 'a_customer_site'], _tcaq[0], 'Pushed "setAccount" to _tcaq.');
  assert.deepEqual(['captureView', 'Content View', expected_config_object], _tcaq[1], 'Pushed "captureView" to _tcaq.');
  assert.equal(2, _tcaq.length, 'There are only 2 events in the _tcaq.');
});

QUnit.test("Test populate _tcaq queue, with identity", function(assert) {
  expect(4);

  // Populate the tcaq queue with identity.
  drupalSettings.acquia_lift.identity = {};
  drupalSettings.acquia_lift.identity.identity = 'my_identity';
  drupalSettings.acquia_lift.identity.identityType = 'my_identity_type';

  Drupal.acquia_lift.populateTcaqQueue();
  var expected_config_object = {
    'post_id': 90210,
    'content_keywords': 'foo,bar',
    'trackingId': drupalSettings.acquia_lift.pageContext.trackingId,
  }
  assert.deepEqual(['setAccount', 'an_account_name', 'a_customer_site'], _tcaq[0], 'Pushed "setAccount" to _tcaq.');
  assert.deepEqual(['captureView', 'Content View', expected_config_object], _tcaq[1], 'Pushed "captureView" to _tcaq.');
  assert.deepEqual(['captureIdentity', 'my_identity', 'my_identity_type'], _tcaq[2], 'Pushed "captureIdentity" to _tcaq.');
  assert.equal(3, _tcaq.length, 'There are only 3 events in the _tcaq.');
});
