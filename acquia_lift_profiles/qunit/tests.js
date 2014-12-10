var _tcaq = _tcaq || [];
var _tcwq = _tcwq || [];

QUnit.module("Acquia Lift Profiles", {
  setup: function() {
    Drupal.settings.personalize = Drupal.settings.personalize || {};
    Drupal.settings = Drupal.settings || {};
    Drupal.settings.acquia_lift_profiles = Drupal.settings.acquia_lift_profiles || {};
    Drupal.settings.acquia_lift_profiles.udfMappings = {};
    Drupal.settings.acquia_lift_profiles.udfMappingContextSeparator = '__';

    Drupal.personalize = Drupal.personalize || {};
    Drupal.personalize.visitor_context = Drupal.personalize.visitor_context || {};
    // We need to mock the getVisitorContexts() method as this is called by the
    // init method.
    Drupal.personalize.getVisitorContexts = function(plugins, callback) {
      callback.call(null, {});
    };
    function assignDummyValues(contexts) {
      var values = {
        'some-context': 'some-value',
        'some-other-context': 'some-other-value',
        'ohai': 42,
        'kthxbai': 0
      };
      var myValues = {};
      for (var i in contexts) {
        if (contexts.hasOwnProperty(i) && values.hasOwnProperty(i)) {
          myValues[i] = values[i];
        }
      }
      return myValues;
    }

    Drupal.personalize.visitor_context.my_first_plugin = {
      'getContext': function(contexts) {
        return assignDummyValues(contexts);
      }
    };
    Drupal.personalize.visitor_context.my_promise_plugin = {
      'getContext': function(contexts) {
        return new Promise(function (resolve, reject) {
          setTimeout(function(){
            var values = assignDummyValues(contexts);
            resolve(values);
          }, 1000)
        });
      }
    };

    var myCache = {};
    Drupal.personalize.visitor_context_write = function(segmentName, plugin, value) {
      myCache[plugin] = myCache[plugin] || {};
      myCache[plugin][segmentName] = value;
    };
    Drupal.personalize.visitor_context_read = function (segmentName, plugin) {
      if (!myCache.hasOwnProperty(plugin)) {
        return null;
      }
      if (!myCache[plugin].hasOwnProperty(segmentName)) {
        return null;
      }
      return myCache[plugin][segmentName];
    };

    Drupal.settings.acquia_lift_profiles.available_segments = ['segment1', 'segment2'];
    var callbackCalled = 0;
    _tcwq = {
      'push':function(stf) {
        callbackCalled++;
        var values;
        setTimeout(function(){
          QUnit.start();
          if (callbackCalled === 1) {
            // The first time we'll just send back one segment.
            values = ['segment1'];
          }
          else {
            // On subsequent calls we'll send back another segment
            values = ['segment1', 'segment2'];
          }
          var captureInfo = {
            x: {
              trackingId: Drupal.acquia_lift_profiles.getTrackingID()
            }
          };

          stf[1].call(null, values, captureInfo);
        }, 1000);
      }
    };
  }
});

QUnit.asyncTest( "init test", function( assert ) {
  expect(5);
  Drupal.acquia_lift_profiles.resetAll();
  _tcaq = {
    'push':function(stf) {
      console.log(stf);
      assert.equal( stf[0], 'captureView',  'capture view received');
      assert.equal( stf[1], 'Content View',  'capture view is of type content view');
      assert.equal( stf[2].person_udf1, "some-value", 'value correctly assigned from context' );
      assert.equal( stf[2].person_udf2, "some-other-value", 'value correctly assigned from promise based context' );
      assert.equal( stf[2].person_udf3, "some-other-value", 'same  value correctly assigned to a second UDF' );
      QUnit.start();
    }
  };
  var settings = {
    acquia_lift_profiles: {
      udfMappings: {
        person: {
          person_udf1: "my_first_plugin__some-context",
          person_udf2: "my_promise_plugin__some-other-context",
          // Test that another UDF can be mapped to the same context.
          person_udf3: "my_promise_plugin__some-other-context"
        }
      },
      udfMappingContextSeparator: '__'
    }
  };

  // We need to mock the getVisitorContexts() method as this is called by the
  // init method, which we're testing here. We just need to make it call the
  // callback that will be passed into it with the values for the contexts
  // specified.
  Drupal.personalize.getVisitorContexts = function(plugins, callback) {
    var values = {
      'my_first_plugin': {
        'some-context': 'some-value'
      },
      'my_promise_plugin': {
        'some-other-context': 'some-other-value'
      }
    };
    callback.call(null, values);
  };
  Drupal.acquia_lift_profiles.init(settings);
});


QUnit.asyncTest("Get context values no cache", function( assert ) {
  expect(7);
  var contextResult = Drupal.personalize.visitor_context.acquia_lift_profiles_context.getContext({'segment1':'segment1', 'segment2':'segment2'});
  assert.ok(contextResult instanceof Promise);
  var cached = Drupal.personalize.visitor_context_read('segment1', 'acquia_lift_profiles_context');
  assert.ok(cached === null);
  QUnit.stop();
  Promise.all([contextResult]).then(function (loadedContexts) {
    QUnit.start();
    console.log(loadedContexts);
    assert.ok(loadedContexts[0].hasOwnProperty('segment1'), 'segment1 returned');
    assert.equal(loadedContexts[0]['segment1'], 1, 'segment1 has value 1');
    assert.ok(loadedContexts[0].hasOwnProperty('segment2'), 'segment2 returned');
    assert.equal(loadedContexts[0]['segment2'], 0, 'segment2 has value 0');
    cached = Drupal.personalize.visitor_context_read('segment1', 'acquia_lift_profiles_context');
    assert.equal(cached, 1, 'Segment1 value has been cached');
  });

});

QUnit.test("get context values with cache", function( assert ) {
  expect(3);
  Drupal.acquia_lift_profiles.clearSegmentMemoryCache();
  Drupal.personalize.visitor_context_write('segment1', 'acquia_lift_profiles_context', 1);
  Drupal.personalize.visitor_context_write('segment2', 'acquia_lift_profiles_context', 1);
  var contextResult = Drupal.personalize.visitor_context.acquia_lift_profiles_context.getContext({'segment1':'segment1', 'segment2':'segment2'});

  assert.ok(!(contextResult instanceof Promise), 'Got the segments from the cache');
  assert.equal(contextResult['segment1'], 1, 'Segment1 has value 1');
  assert.equal(contextResult['segment2'], 1, 'Segment2 has value 1');
});

QUnit.asyncTest( "personalize decision event", function( assert ) {
  expect(5);
  Drupal.acquia_lift_profiles.resetAll();
  _tcaq = {
    'push':function(args) {
      if ( args[1] == 'Campaign Action' ) {
        assert.equal( args[0], 'capture',  'capture view received');
        assert.equal( args[1], 'Campaign Action',  'capture view is of type campaign action');
        assert.equal( args[2].targetcampaignid, "my-agent", 'value correctly assigned from event' );
        assert.equal( args[2].targetcampaignname, "Test Agent", 'value correctly assigned from event' );
        assert.equal( args[2].targetactionname, "test_decision", 'value correctly assigned from event' );
        QUnit.start();
      }
    }
  };
  var settings = jQuery.extend({}, Drupal.settings);
  settings.personalize.agent_map = {
    'my-agent': {
      'active': 1,
      'cache_decisions': false,
      'enabled_contexts': [],
      'type': 'test_agent',
      'label' : 'Test Agent'
    }
  };

  Drupal.acquia_lift_profiles.init(settings);
  $(document).trigger("personalizeDecision", [{}, "test_decision", "test_osid", "my-agent" ]);
});

QUnit.asyncTest( "sent goal to agent event", function( assert ) {
  expect(4);
  Drupal.acquia_lift_profiles.resetAll();
  _tcaq = {
    'push':function(args) {
      if ( args[1] == 'goal-event' ) {
        assert.equal( args[0], 'capture',  'capture received');
        assert.equal( args[1], 'goal-event',  'capture view is of type goal-event');
        assert.equal( args[2].targetcampaignid, "my-agent", 'value correctly assigned from event' );
        assert.equal( args[2].targetcampaignname, "Test Agent", 'value correctly assigned from event' );
        QUnit.start();
      }
    }
  };
  var settings = jQuery.extend({}, Drupal.settings);
  settings.personalize.agent_map = {
    'my-agent': {
      'active': 1,
      'cache_decisions': false,
      'enabled_contexts': [],
      'type': 'test_agent',
      'label' : 'Test Agent'
    }
  };

  Drupal.acquia_lift_profiles.init(settings);
  $(document).trigger("sentGoalToAgent", ["my-agent", "goal-event", "goal-value"]);
});

QUnit.test( "Server-side events no email", function( assert ) {
  expect(4);
  Drupal.acquia_lift_profiles.resetAll();
  // Add a couple of server-side actions to the settings. Don't include the 'mail'
  // property in the user_login action's context.
  var settings = jQuery.extend({}, Drupal.settings);
  settings.acquia_lift_profiles.serverSideActions = {
    'someAction' : {
      'someContextKey': 'someContextValue'
    },
    'user_login': [
      { 'username': 'someUser' }
    ]
  };

  // Our _tcaq.push should be called with a 'capture' event for each server-side
  // action in our settings when we call processServerSideActions. THere should not
  // be a captureIdentity push because there's no email address in our user_login
  // context.
  _tcaq = {
    'push':function(args) {
      if ( args[1] == 'someAction' ) {
        assert.equal( args[0], 'capture',  'capture received');
        assert.equal( args[1], 'someAction',  'custom event captured');
      }
      else if (args[1] == 'user_login' ) {
        assert.equal( args[0], 'capture',  'capture received');
        assert.equal( args[1], 'user_login',  'custom event captured');
      }
      else if (args[0] == 'captureIdentity') {
        // Add an assertion here which simply serves to make the test fail if we
        // ever get here because the number of assertions will not match what's
        // expected.
        assert.ok('This should never run');
      }
    }
  };

  Drupal.acquia_lift_profiles.processServerSideActions(settings);
});


QUnit.test( "Server-side events with email", function( assert ) {
  expect(7);
  Drupal.acquia_lift_profiles.resetAll();

  // Add a couple of server-side actions to the settings. This time we do include
  // the 'mail' property in the user_login action's context.
  var settings = jQuery.extend({}, Drupal.settings);
  settings.acquia_lift_profiles.captureIdentity = 1;
  settings.acquia_lift_profiles.serverSideActions = {
    'someAction' : {
      'someContextKey': 'someContextValue'
    },
    'user_login': [
      { 'mail': 'test@example.com' }
    ]
  };

  // Our _tcaq.push should be called with a 'capture' event for each server-side
  // action in our settings when we call processServerSideActions. It should also
  // be called with a captureIdentity event because of the user_login with email.
  _tcaq = {
    'push':function(args) {
      if ( args[1] == 'someAction' ) {
        assert.equal( args[0], 'capture',  'capture received');
        assert.equal( args[1], 'someAction',  'custom event captured');
      }
      else if (args[1] == 'user_login' ) {
        assert.equal( args[0], 'capture',  'capture received');
        assert.equal( args[1], 'user_login',  'custom event captured');
      }
      else if (args[0] == 'captureIdentity') {
        assert.equal( args[0], 'captureIdentity',  'capture received');
        assert.equal( args[1], 'test@example.com',  'identity captured');
        assert.equal( args[2], 'email',  'identity type captured');
      }
    }
  };
  Drupal.acquia_lift_profiles.processServerSideActions(settings);
});

QUnit.test( "Capture identity test", function( assert ) {
  expect(3);
  Drupal.acquia_lift_profiles.resetAll();

  var settings = jQuery.extend({}, Drupal.settings);
  // Add an identity and identityType to the settings.
  settings.acquia_lift_profiles.identity = 'someTestUser';
  settings.acquia_lift_profiles.identityType = 'socialTastic';
  // Our _tcaq.push should be called wiht a captureIdentity event
  // when we call the init function with our settings.
  _tcaq = {
    'push':function(args) {
      if ( args[0] == 'captureIdentity' ) {
        assert.equal( args[0], 'captureIdentity',  'capture received');
        assert.equal( args[1], 'someTestUser',  'identity captured');
        assert.equal( args[2], "socialTastic", 'identity type received' );
      }
    }
  };

  Drupal.acquia_lift_profiles.init(settings);

  // There should be no subsequent captureIdentity call when we process a
  // user login action.
  settings.acquia_lift_profiles.captureIdentity = 1;
  settings.acquia_lift_profiles.serverSideActions = {
    'user_login': [
      { 'mail': 'test@example.com' }
    ]
  };
  Drupal.acquia_lift_profiles.processServerSideActions(settings);
});
