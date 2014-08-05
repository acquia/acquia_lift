var _tcaq = _tcaq || [];
function AcquiaLiftProfilesQUnitSetup() {
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
  Drupal.personalize = Drupal.personalize || {};
  Drupal.personalize.visitor_context = Drupal.personalize.visitor_context || {};
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
}
QUnit.asyncTest( "init test", function( assert ) {
  expect(5);
  AcquiaLiftProfilesQUnitSetup();
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
  Drupal.acquia_lift_profiles.init(settings);
});
