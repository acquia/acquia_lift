(function ($) {

Drupal.acquia_lift_target = (function() {

  var agentRules = {}, initialized = false;

  function init() {
    var i, optionSet, agentName;
    var option_sets = Drupal.settings.personalize.option_sets;
    var agent_map = Drupal.settings.personalize.agent_map;
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

  function executeDecision(rule, decisions, choices, callback) {
    // The rule we matched could specify a single option, a combination of options,
    // or another option set with a different decision agent.
    if (rule.hasOwnProperty('option_id')) {
      for (var decision_name in decisions) {
        if (decisions.hasOwnProperty(decision_name) && choices.hasOwnProperty(decision_name) && choices[decision_name].indexOf(rule.option_id) != -1) {
          decisions[decision_name] = rule.option_id;
        }
      }
    }
    else if (rule.hasOwnProperty('osid')) {
      var osid = 'osid-' + rule.osid;
      if (Drupal.settings.acquia_lift_target.option_sets.hasOwnProperty(osid)) {
        var optionSet = Drupal.settings.acquia_lift_target.option_sets[osid];
        var agent_name = optionSet.agent,
            agent_plugin = Drupal.settings.acquia_lift_target.test_agent_plugin,
            nestedPoint = optionSet.decision_point,
            nestedDecision = optionSet.decision_name,
            subChoices = {},
            fallbacks = {};
        subChoices[nestedDecision] = optionSet.option_names;
        fallbacks[nestedDecision] = 0;

        if (Drupal.settings.acquia_lift_target.agent_map.hasOwnProperty(agent_name) && Drupal.personalize.agents.hasOwnProperty(agent_plugin)) {
          var subCallback = function(selection) {
            for (var decision_name in decisions) {
              if (decisions.hasOwnProperty(decision_name) && selection.hasOwnProperty(nestedDecision)) {
                decisions[decision_name] = selection[nestedDecision];
              }
            }
            callback(decisions);
          };
          Drupal.personalize.agents[agent_plugin].getDecisionsForPoint(agent_name, {}, subChoices, nestedPoint, fallbacks, subCallback);
          return;
        }
      }
    }
    callback(decisions);
  }

  return {
    'getDecision': function(agent_name, visitor_context, choices, decision_point, fallbacks, callback) {
      if (!initialized) {
        init();
      }

      if (!agentRules.hasOwnProperty(agent_name)) {
        // Just delegate to the testing agent.
        Drupal.personalize.agents.acquia_lift.getDecisionsForPoint(agent_name, visitor_context, choices, decision_point, fallbacks, callback);
        return;
      }
      var decisions = {},
          matched,
          i,
          j,
          ruleId,
          strategy,
          feature_strings = convertContextToFeatureStrings(visitor_context),
          fallbackIndex = fallbacks.hasOwnProperty(decision_name) ? fallbacks[decision_name] : 0,
          defaultTarget = Drupal.settings.acquia_lift_target.default_target;
      // Initialize each decision to the fallback option.
      for (var decision_name in choices) {
        if (choices.hasOwnProperty(decision_name)) {
          decisions[decision_name] = choices[decision_name][fallbackIndex];
        }
      }

      for (i in agentRules[agent_name]) {
        if (agentRules[agent_name].hasOwnProperty(i)) {
          ruleId = i;
          // If this is the "everyone else" target, then there are no features to be matched,
          // just execute the decision for this target.
          if (agentRules[agent_name][ruleId].name == defaultTarget) {
            executeDecision(agentRules[agent_name][ruleId], decisions, choices, callback);
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
                executeDecision(agentRules[agent_name][ruleId], decisions, choices, callback);
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
                executeDecision(agentRules[agent_name][ruleId], decisions, choices, callback);
                return;
              }
              break;
          }
        }
      }
      // If we got here there was no matched targeting rule so we just call the
      // callback with the fallback decisions.
      callback(decisions);
    },
    'sendGoal': function(agent_name, goal_name, value) {
      if (!initialized) {
        init();
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

})(jQuery);
