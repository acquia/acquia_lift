QUnit.test("test explicit targeting logic", function( assert ) {
  expect(6);
  // Add settings for a targeting agent with some fixed targeting rules on its single
  // option set.
  var agentName = 'my-test-agent',
    decisionName = 'my-decision',
    enabledContexts = {
      'some_plugin': {
        'some-context': 'some-context',
        'other-context': 'other-context'
      }
    },
    options = {
      'first-option': {
        'option_id': 'first-option',
        'option_label': 'First Option'
      },
      'second-option': {
        'option_id': 'second-option',
        'option_label': 'Second Option',
        // Add fixed targeting rules such that this option should be shown if two
        // feature strings are present.
        'fixed_targeting': [
          "some-context::some-value",
          "other-context::ss-other"
        ],
        'fixed_targeting_strategy': 'AND'
      },
      'third-option': {
        'option_id': 'third-option',
        'option_label': 'Third Option',
        // Add fixed targeting rules such that this option should be shown if one of
        // two feature strings is present.
        'fixed_targeting': [
          "some-context::some-value",
          "other-context::ss-ohai"
        ],
        'fixed_targeting_strategy': 'OR'
      }
    };
  addLiftTargetToDrupalSettings(agentName, enabledContexts, decisionName, 'osid-1', options);

  // Now request decisions from that agent to test its behavior with different contexts.
  var evaluatedVisitorContexts = {},
    choices = {},
    fallbacks = {};
  choices[decisionName] = ['first-option', 'second-option', 'third-option'];
  fallbacks[decisionName] = 0;
  // Try first with no visitor contexts present, we should get the first (fallback) option.
  Drupal.personalize.agents.acquia_lift_target.getDecisionsForPoint(agentName, evaluatedVisitorContexts, choices, decisionName, fallbacks,     callback = function(decisions) {
    assert.ok(decisions.hasOwnProperty(decisionName));
    assert.equal(decisions[decisionName], 'first-option');
  });

  // Now try with contexts that should satisfy the rules for the second option.
  evaluatedVisitorContexts = {
    'some-context': [
      'some-value',
      'sc-some'
    ],
    'other-context': [
      'other-value',
      'ss-other'
    ]
  };
  Drupal.personalize.agents.acquia_lift_target.getDecisionsForPoint(agentName, evaluatedVisitorContexts, choices, decisionName, fallbacks,     callback = function(decisions) {
    assert.ok(decisions.hasOwnProperty(decisionName));
    assert.equal(decisions[decisionName], 'second-option');
  });

  // Now try with contexts that only partially satisfy the rules for the second option, but
  // fully satisfy the rules for the third option.
  evaluatedVisitorContexts = {
    'some-context': [
      'some-value',
      'sc-some'
    ],
    'other-context': [
      'my-other-value'
    ]
  };
  Drupal.personalize.agents.acquia_lift_target.getDecisionsForPoint(agentName, evaluatedVisitorContexts, choices, decisionName, fallbacks,     callback = function(decisions) {
    assert.ok(decisions.hasOwnProperty(decisionName));
    assert.equal(decisions[decisionName], 'third-option');
  });
});

/**
 * Adds settings for the required targeting agent set-up to Drupal.settings.
 */
function addLiftTargetToDrupalSettings(agent_name, enabled_contexts, decision_name, osid, options) {

  Drupal.settings.personalize.agent_map = Drupal.settings.personalize.agent_map || {};
  Drupal.settings.personalize.agent_map[agent_name] = {
    'active': 1,
    'cache_decisions': false,
    'enabled_contexts': enabled_contexts,
    'type': 'acquia_lift_target'
  };

  var option_names = [];
  var options_array = [];
  for (var i in options) {
    if (options.hasOwnProperty(i)) {
      option_names.push(i);
      options_array.push(options[i]);
    }
  }
  Drupal.settings.personalize.option_sets = Drupal.settings.personalize.option_sets || {};
  Drupal.settings.personalize.option_sets[osid] = {
    'agent': agent_name,
    'data': [],
    'decision_name': decision_name,
    'decision_point': decision_name,
    'executor': 'show',
    'label': 'My Lift Target',
    'mvt': null,
    'option_names': option_names,
    'options': options_array,
    'osid': osid,
    'plugin': 'my_os_plugin',
    'selector': '.some-class',
    'stateful': 0,
    'winner': null
  };
}
