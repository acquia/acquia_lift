var _tcaq = _tcaq || [];
var _tcwq = _tcwq || [];
var TC = TC || {};

(function ($, TC) {

Drupal.acquia_lift_target = (function() {

  var agentRules = {}, initialized = false, processedDecisions = {}, agentLabels = {};

  /**
   * Returns the agent label for the given agent name.
   * If there is no label then the agent name is returned.
   *
   * @param agent_name
   */
  function getAgentLabel(agent_name) {
    var agent_label = agent_name;
    if (agentLabels[agent_name]) {
      agent_label = agentLabels[agent_name];
    }
    return agent_label;
  }


  function contextToFeatureString(key, value) {
    return key + '::' + value;
  }

  function convertContextToFeatureStrings(visitor_context) {
    var i, j, feature_strings = [];
    for (i in visitor_context) {
      if (visitor_context.hasOwnProperty(i)) {
        for (j in visitor_context[i]) {
          if (visitor_context[i].hasOwnProperty(j)) {
            feature_strings.push(contextToFeatureString(i, visitor_context[i][j]));
          }
        }
      }
    }
    return feature_strings;
  }

  function readDecisionsFromStorage(agent_name) {
    var bucket = Drupal.personalize.storage.utilities.getBucket('lift');
    return bucket.read(agent_name);
  }

  function writeDecisionsToStorage(agent_name, decisions, policy, goals, audience) {
    var bucket = Drupal.personalize.storage.utilities.getBucket('lift');
    var value = {
      'policy': policy,
      'decisions' : decisions
    };
    if (goals != undefined) {
      value.goals = goals;
    }
    if (audience != undefined) {
      value.audience = audience;
    }
    bucket.write(agent_name, value);
  }

  function executeDecision(agent_name, rule, decisions, choices, callback) {
    // The rule we matched could specify a single option, a combination of options,
    // or another option set with a different decision agent.
    if (rule.hasOwnProperty('option_id')) {
      for (var decision_name in decisions) {
        if (decisions.hasOwnProperty(decision_name) && choices.hasOwnProperty(decision_name) && choices[decision_name].indexOf(rule.option_id) != -1) {
          decisions[decision_name] = rule.option_id;
        }
      }
      callback(decisions, 'targeting', rule.name);
      return;
    }
    else if (rule.hasOwnProperty('osid')) {
      var osid = 'osid-' + rule.osid;
      if (Drupal.settings.acquia_lift_target.option_sets.hasOwnProperty(osid)) {
        var optionSet = Drupal.settings.acquia_lift_target.option_sets[osid];
        var test_agent_name = optionSet.agent,
            agent_plugin = Drupal.settings.acquia_lift_target.test_agent_plugin,
            nestedPoint = optionSet.decision_point,
            nestedDecision = optionSet.decision_name,
            subChoices = {},
            fallbacks = {};
        subChoices[nestedDecision] = optionSet.option_names;
        fallbacks[nestedDecision] = 0;

        if (Drupal.settings.acquia_lift_target.agent_map.hasOwnProperty(test_agent_name) && Drupal.personalize.agents.hasOwnProperty(agent_plugin)) {
          if (Drupal.settings.acquia_lift_target.agent_map[test_agent_name].type !== agent_plugin) {
            return;
          }
          var subCallback = function(selection, policy) {
            for (var decision_name in decisions) {
              if (decisions.hasOwnProperty(decision_name) && selection.hasOwnProperty(nestedDecision)) {
                decisions[decision_name] = selection[nestedDecision];
              }
            }
            callback(decisions, policy, rule.name);
          };
          Drupal.personalize.agents[agent_plugin].getDecisionsForPoint(test_agent_name, {}, subChoices, nestedPoint, fallbacks, subCallback);
          return;
        }
      }
    }
    callback(decisions, 'fallback', rule.name);
  }

  return {
    'init': function(settings) {
      if (initialized) {
        return;
      }
      var i, optionSet, agentName;
      var option_sets = settings.personalize.option_sets;
      var agent_map = settings.personalize.agent_map;
      for (i in option_sets) {
        if (option_sets.hasOwnProperty(i) && agent_map.hasOwnProperty(option_sets[i].agent) && agent_map[option_sets[i].agent]['type'] == 'acquia_lift_target') {
          optionSet = option_sets[i];
          agentName = optionSet.agent;
          if (agentRules.hasOwnProperty(agentName) || !optionSet.hasOwnProperty('targeting')) {
            continue;
          }
          agentRules[agentName] = optionSet.targeting;
        }
      }
      for (var agent_name in agent_map) {
        if (agent_map.hasOwnProperty(agent_name)) {
          if (agent_map[agent_name].label) {
            agentLabels[agent_name] = agent_map[agent_name].label;
          }
        }
      }
      initialized = true;
    },
    'reset': function() {
      agentRules = {};
      initialized = false;
      processedDecisions = {};
      agentLabels = {};
    },
    'getDecision': function(agent_name, visitor_context, choices, decision_point, fallbacks, callback) {
      if (!initialized) {
        this.init(Drupal.settings);
      }

      var callback_wrapper = function(decisions, policy, audience) {
        if (policy != "repeat") {
          writeDecisionsToStorage(agent_name, decisions, policy, null, audience);
          // In theory there could be mulitple desicions here (if it's an MVT), in
          // which case we'll concatenate the decision names and the choice names,
          // separated by ','.
          var decision_str = '', choice_str = '', index = 0;
          for (var decision_name in decisions) {
            if (decisions.hasOwnProperty(decision_name)) {
              if (index > 0) {
                decision_str += ',';
                choice_str += ',';
              }
              decision_str += decision_name;
              choice_str += decisions[decision_name];
              index++;
            }
          }
          // Send this to Lift Web if it has never been sent or the decision has changed due to
          // new targeting conditions being met.
          if (processedDecisions.hasOwnProperty(agent_name) && processedDecisions[agent_name] == choice_str) {
            return;
          }
          processedDecisions[agent_name] = choice_str;
          _tcaq.push(['capture', 'Decision', {'personalization_name': getAgentLabel(agent_name), 'personalization_machine_name':agent_name, 'personalization_audience_name': audience, 'personalization_chosen_variation': choice_str, 'personalization_decision_policy': policy }]);
        }
        callback(decisions);
      };
      if (!agentRules.hasOwnProperty(agent_name)) {
        // Just delegate to the testing agent.
        var agent_plugin = Drupal.settings.acquia_lift_target.test_agent_plugin;
        Drupal.personalize.agents[agent_plugin].getDecisionsForPoint(agent_name, visitor_context, choices, decision_point, fallbacks, callback_wrapper);
        return;
      }
      var decisions = {},
          matched,
          i,
          j,
          ruleId,
          strategy,
          feature_strings = convertContextToFeatureStrings(visitor_context),
          defaultTarget = Drupal.settings.acquia_lift_target.default_target;
      // Initialize each decision to the fallback option.
      for (var decision_name in choices) {
        if (choices.hasOwnProperty(decision_name)) {
          decisions[decision_name] = choices[decision_name][0];
        }
      }

      for (i in agentRules[agent_name]) {
        if (agentRules[agent_name].hasOwnProperty(i)) {
          ruleId = i;
          // Make sure the audience name is a string
          agentRules[agent_name][ruleId].name = '' + agentRules[agent_name][ruleId].name;
          // If this is the "everyone else" target, then there are no features to be matched,
          // just execute the decision for this target.
          if (agentRules[agent_name][ruleId].name.indexOf(defaultTarget) == 0) {
            executeDecision(agent_name, agentRules[agent_name][ruleId], decisions, choices, callback_wrapper);
            return;
          }
          if (!agentRules[agent_name][ruleId].hasOwnProperty('targeting_features') || agentRules[agent_name][ruleId].targeting_features.length == 0) {
            continue;
          }
          strategy = agentRules[agent_name][ruleId].targeting_strategy;
          switch (strategy) {
            case 'AND':
              // If all features are present, call the callback with this option
              // as the chosen option.
              matched = true;
              // Set matched to false if any feature is missing.
              for (j in agentRules[agent_name][ruleId].targeting_features) {
                if (agentRules[agent_name][ruleId].targeting_features.hasOwnProperty(j)) {
                  if (feature_strings.indexOf(agentRules[agent_name][ruleId].targeting_features[j]) === -1) {
                    matched = false;
                    break;
                  }
                }
              }
              if (matched) {
                executeDecision(agent_name, agentRules[agent_name][ruleId], decisions, choices, callback_wrapper);
                return;
              }
              break;
            default:
              // If any of the features are present, call the callback with this option
              // as the chosen option.
              matched = false;
              // Set matched to true if *any* feature is present.
              for (j in agentRules[agent_name][ruleId].targeting_features) {
                if (agentRules[agent_name][ruleId].targeting_features.hasOwnProperty(j)) {
                  if (feature_strings.indexOf(agentRules[agent_name][ruleId].targeting_features[j]) !== -1) {
                    matched = true;
                    break;
                  }
                }
              }
              if (matched) {
                executeDecision(agent_name, agentRules[agent_name][ruleId], decisions, choices, callback_wrapper);
                return;
              }
              break;
          }
        }
      }
      // If we got here there was no matched targeting rule so we just call the
      // callback with the fallback decisions.
      callback_wrapper(decisions, 'fallback');
    },
    'sendGoal': function(agent_name, goal_name, value) {
      if (!initialized) {
        this.init(Drupal.settings);
      }

      var stored = readDecisionsFromStorage(agent_name);
      if (stored && stored.hasOwnProperty('decisions')) {
        // First see if this goal was already attained.
        var goals = stored.hasOwnProperty('goals') ? stored.goals : [];
        if (goals.indexOf(goal_name) !== -1) {
          return;
        }

        goals.push(goal_name);
        writeDecisionsToStorage(agent_name, stored.decisions, stored.policy, goals, stored.audience);
        // In theory there could be mulitple desicions here (if it's an MVT), in
        // which case we'll concatenate the decision names and the choice names,
        // separated by ','.
        var decision_str = '', choice_str = '', index = 0;
        for (var decision_name in stored.decisions) {
          if (stored.decisions.hasOwnProperty(decision_name)) {
            if (index > 0) {
              decision_str += ',';
              choice_str += ',';
            }
            decision_str += decision_name;
            choice_str += stored.decisions[decision_name];
            index++;
          }
        }
        _tcaq.push(['capture', 'Goal', {'personalization_name': getAgentLabel(agent_name), 'personalization_machine_name':agent_name, 'personalization_audience_name': stored.audience, 'personalization_chosen_variation': choice_str, 'personalization_decision_policy': stored.policy, 'personalization_goal_name': goal_name, 'personalization_goal_value': value }]);
      }

      if (!stored || (stored.hasOwnProperty("policy") && stored.policy == 'targeting')) {
        return;
      }

      // Find any nested tests this goal needs to be sent to.
      var nested = Drupal.settings.acquia_lift_target.nested_tests;
      if (nested.hasOwnProperty(agent_name)) {
        var agent_plugin = Drupal.settings.acquia_lift_target.test_agent_plugin;
        for (var i in nested[agent_name]) {
          if (nested[agent_name].hasOwnProperty(i) && Drupal.personalize.agents.hasOwnProperty(agent_plugin)) {
            Drupal.personalize.agents[agent_plugin].sendGoalToAgent(nested[agent_name][i], goal_name, value);
          }
        }
      }
    }
  }
})();

Drupal.personalize = Drupal.personalize || {};
Drupal.personalize.agents = Drupal.personalize.agents || {};
Drupal.personalize.agents.acquia_lift_target = {
  'getDecisionsForPoint': function(agent_name, visitor_context, choices, decision_point, fallbacks, callback) {
    Drupal.acquia_lift_target.getDecision(agent_name, visitor_context, choices, decision_point, fallbacks, callback);
  },
  'sendGoalToAgent': function(agent_name, goal_name, value) {
    Drupal.acquia_lift_target.sendGoal(agent_name, goal_name, value);
  },
  'featureToContext': function(featureString) {
    var contextArray = featureString.split('::');
    return {
      'key': contextArray[0],
      'value': contextArray[1]
    }
  }
};

Drupal.personalize.agents.acquia_lift_learn = {
  'getDecisionsForPoint': function(agent_name, visitor_context, choices, decision_point, fallbacks, callback) {
    // Our decision point may have multiple decisions, if doing MVT.
    Drupal.acquiaLiftLearn.getDecision(agent_name, choices, decision_point, fallbacks, callback);
  },
  'sendGoalToAgent': function(agent_name, goal_name, goal_value, jsEvent) {
    Drupal.acquiaLiftLearn.sendGoal(agent_name, goal_name, goal_value, jsEvent);
  }
};
Drupal.acquiaLiftLearn = (function() {

  var settings, api, initialized = false, sessionID = null, site_name_prefixes = {};

  function initializeSession() {
    if (sessionID == null && TC.hasOwnProperty('getSessionID')) {
      sessionID = TC.getSessionID();
      Drupal.personalize.saveSessionID(sessionID);
    }
  }
  function init() {
    initializeSession();
    settings = Drupal.settings.acquia_lift_learn;
    api = Drupal.acquiaLiftAPI.getInstance();
    if (Drupal.settings.hasOwnProperty("acquia_lift_target") && Drupal.settings.acquia_lift_target.hasOwnProperty("agent_map")) {
      for (var agent_name in Drupal.settings.acquia_lift_target.agent_map) {
        if (Drupal.settings.acquia_lift_target.agent_map.hasOwnProperty(agent_name)) {
          site_name_prefixes[agent_name] = Drupal.settings.acquia_lift_target.agent_map[agent_name].site_name_prefix;
        }
      }
    }
    initialized = true;
  }

  return {
    'getDecision': function(agent_name, choices, decision_point, fallbacks, callback) {
      if (!initialized) {
        init();
      }
      var options = {};
      var fb = [];
      for (var key in fallbacks) {
        if (fallbacks.hasOwnProperty(key) && choices.hasOwnProperty(key)) {
          fb.push({
            'decision_set_id': key,
            'external_id': choices[key][fallbacks[key]]
          });
        }
      }
      options.fallback = fb;
      api.decision(agent_name, options, function(outcome, policy) {
        // We need to send back an object with decision names as keys
        // and the chosen option for each one as the value.
        var decisions = {};
        for (var i in outcome) {
          if (outcome.hasOwnProperty(i) && outcome[i].hasOwnProperty('decision_set_id') &&
              outcome[i].hasOwnProperty('external_id')) {
            var decision_name = outcome[i].decision_set_id;
            if (site_name_prefixes.hasOwnProperty(agent_name)) {
              decision_name = decision_name.replace(site_name_prefixes[agent_name], '')
            }
            decisions[decision_name] = outcome[i].external_id;
          }
        }
        callback(decisions, policy);
      });
    },
    'sendGoal': function(agent_name, goal_name, value) {
      if (!initialized) {
        init();
      }

      if (site_name_prefixes.hasOwnProperty(agent_name)) {
        goal_name = site_name_prefixes[agent_name] + goal_name;
      }
      var options = {
        reward: value,
        goal: goal_name
      };
      Drupal.acquiaLiftUtility.GoalQueue.addGoal(agent_name, options);
    },
    // @todo Remove this once we can just pass the session id in the api's
    //   getInstance call (i.e. once V1's js has gone away.
    'getSessionID': function() {
      if (sessionID == null) {
        initializeSession();
      }
      return sessionID;
    }
  }
})();

})(jQuery, TC);
