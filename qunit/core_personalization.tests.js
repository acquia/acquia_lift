/**
 * @file libraries.js
 */

QUnit.module("Acquia Lift service calls", {
  'setup': function() {
    Drupal.acquiaLift.reset();
    Drupal.personalize.initializeSessionID = function() {
      return 'some-session-ID';
    };
    Drupal.settings.personalize = Drupal.settings.personalize || {};
    Drupal.settings.acquia_lift = Drupal.settings.acquia_lift || {};
    Drupal.settings.acquia_lift.baseUrl = 'http://api.example.com';
    Drupal.settings.acquia_lift.owner = 'someOwner';
    Drupal.settings.acquia_lift.apiKey = 'xyz123';
    Drupal.settings.acquia_lift.featureStringSeparator = '::';
    Drupal.settings.acquia_lift.featureStringMaxLength = 50;
    Drupal.settings.acquia_lift.featureStringReplacePattern = '[^A-Za-z0-9_-]';
  }
});

QUnit.test('Make decision', function(assert) {
  var xhr = sinon.useFakeXMLHttpRequest();
  var requests = sinon.requests = [];

  xhr.onCreate = function (request) {
    requests.push(request);
  };

  var callback = sinon.spy();
  Drupal.acquiaLift.getDecision('my-agent', {'some-feature': ['some-value', 'sc-some-value'],'other-feature': ['some,value'] }, {'first-decision': ['option-1', 'option-2']}, 'my-decision-point', {'first-decision': 0}, callback);

  equal(sinon.requests.length, 1);
  var parsed = parseUri(sinon.requests[0].url);

  equal(parsed.host, 'api.example.com');
  equal(parsed.path, "/someOwner/my-agent/decisions/first-decision:option-1,option-2");
  equal(parsed.queryKey.apikey, "xyz123");
  equal(parsed.queryKey.features, "some-feature%3A%3Asome-value%2Csome-feature%3A%3Asc-some-value%2Cother-feature%3A%3Asome-value");
  equal(parsed.queryKey.session, "some-session-ID");

  requests[0].respond(200, { "Content-Type": "application/json" }, '{"decisions": {"first-decision":"option-2"}, "session": "1234678"}');
  ok(callback.called);
});

QUnit.test('Make batched decisions', function(assert) {

  Drupal.settings.acquia_lift.batchMode = true;
  var xhr = sinon.useFakeXMLHttpRequest();
  var requests = sinon.requests = [];

  xhr.onCreate = function (request) {
    requests.push(request);
  };

  var callback = sinon.spy();
  // Fire off two requests for decisions - only one HTTP request should be made.
  Drupal.acquiaLift.getDecision('my-agent', {'some-feature': ['some-value', 'sc-some-value'],'other-feature': ['some,value'] }, {'first-decision': ['option-1', 'option-2']}, 'my-decision-point', {'first-decision': 0}, callback);
  Drupal.acquiaLift.getDecision('my-agent', {'some-feature': ['some-value', 'sc-some-value'],'other-feature': ['some,value'] }, {'second-decision': ['option-1', 'option-2']}, 'other-decision-point', {'second-decision': 0}, callback);
  // This event lets the Lift js know there are no more decisions to wait for.
  $(document).trigger('personalizeDecisionsEnd');

  // Mock a response from the server so that the request completes.
  requests[0].respond(200, { "Content-Type": "application/json" }, '[{"status": 200, "data": {"agent": "my-agent", "decisions": {"first-decision":"option-2"}, "session": "1234678"}}, {"status": 200, "data": {"agent": "my-agent", "decisions": {"second-decision":"option-2"}, "session": "1234678"}}}]');
  // Confirm a single request is made with the expected request body.
  assert.equal(sinon.requests.length, 1);
  var parsedUri = parseUri(sinon.requests[0].url);
  var requestBody = JSON.parse(sinon.requests[0].requestBody);
  var expectedRequestBody = [
    {
      'agent': 'my-agent',
      'choices': "first-decision:option-1,option-2",
      'query': {
        '_t': 0,
        'apikey': 'xyz123',
        'features': "some-feature::some-value,some-feature::sc-some-value,other-feature::some-value",
        'point': 'my-decision-point',
        'session': 'some-session-ID'
      },
      'type': 'decisions'
    },
    {
      'agent': 'my-agent',
      'choices': "second-decision:option-1,option-2",
      'query': {
        '_t': 0,
        'apikey': 'xyz123',
        'features': "some-feature::some-value,some-feature::sc-some-value,other-feature::some-value",
        'point': 'other-decision-point',
        'session': 'some-session-ID'
      },
      'type': 'decisions'
    }
  ];
  assert.deepEqual(expectedRequestBody, requestBody);
  assert.equal(parsedUri.host, 'api.example.com');
  assert.equal(parsedUri.path, "/someOwner/-/batch");
  assert.equal(parsedUri.queryKey.session, "some-session-ID");

  assert.ok(callback.called);

});

// Helper for parsing the ajax request URI

// parseUri 1.2.2
// (c) Steven Levithan <stevenlevithan.com>
// MIT License

function parseUri (str) {
  var	o   = parseUri.options,
    m   = o.parser[o.strictMode ? "strict" : "loose"].exec(str),
    uri = {},
    i   = 14;

  while (i--) uri[o.key[i]] = m[i] || "";

  uri[o.q.name] = {};
  uri[o.key[12]].replace(o.q.parser, function ($0, $1, $2) {
    if ($1) uri[o.q.name][$1] = $2;
  });

  return uri;
};

parseUri.options = {
  strictMode: false,
  key: ["source","protocol","authority","userInfo","user","password","host","port","relative","path","directory","file","query","anchor"],
  q:   {
    name:   "queryKey",
    parser: /(?:^|&)([^&=]*)=?([^&]*)/g
  },
  parser: {
    strict: /^(?:([^:\/?#]+):)?(?:\/\/((?:(([^:@]*)(?::([^:@]*))?)?@)?([^:\/?#]*)(?::(\d*))?))?((((?:[^?#\/]*\/)*)([^?#]*))(?:\?([^#]*))?(?:#(.*))?)/,
    loose:  /^(?:(?![^:@]+:[^:@\/]*@)([^:\/?#.]+):)?(?:\/\/)?((?:(([^:@]*)(?::([^:@]*))?)?@)?([^:\/?#]*)(?::(\d*))?)(((\/(?:[^?#](?![^?#\/]*\.[^?#\/.]+(?:[?#]|$)))*\/?)?([^?#\/]*))(?:\?([^#]*))?(?:#(.*))?)/
  }
};
