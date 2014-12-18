(function ($) {

Drupal.acquia_lift_target = (function() {

  var agentRules = {}, initialized = false;

  function init() {
    var i, j, optionSet, agentName, optionId;
    var option_sets = Drupal.settings.personalize.option_sets;
    var agent_map = Drupal.settings.personalize.agent_map;
    for (i in option_sets) {
      if (option_sets.hasOwnProperty(i) && agent_map.hasOwnProperty(option_sets[i].agent) && agent_map[option_sets[i].agent]['type'] == 'acquia_lift_target') {
        optionSet = option_sets[i];
        agentName = optionSet.agent;
        if (agentRules.hasOwnProperty(agentName)) {
          continue;
        }
        agentRules[agentName] = {};
        for (j in optionSet.options) {
          if (optionSet.options.hasOwnProperty(j) && optionSet.options[j].hasOwnProperty('fixed_targeting')) {
            optionId = optionSet.options[j].option_id;
            agentRules[agentName][optionId] = {
              'strategy': optionSet.options[j].fixed_targeting_strategy,
              'features': optionSet.options[j].fixed_targeting
            };
          }
        }
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

  return {
    'getDecision': function(agent_name, visitor_context, choices, decision_name, fallbacks, callback) {
      if (!initialized) {
        init();
      }
      var decisions = {},
        matched,
        i,
        j,
        optionId,
        strategy,
        feature_strings = convertContextToFeatureStrings(visitor_context),
        // Initialize the decision to the fallback option.
        fallbackIndex = fallbacks.hasOwnProperty(decision_name) ? fallbacks[decision_name] : 0;

      decisions[decision_name] = choices[fallbackIndex];
      if (agentRules.hasOwnProperty(agent_name)) {
        for (i in agentRules[agent_name]) {
          if (agentRules[agent_name].hasOwnProperty(i)) {
            optionId = i;
            if (agentRules[agent_name][optionId].features.length == 0) {
              continue;
            }
            strategy = agentRules[agent_name][optionId].strategy;
            switch (strategy) {
              case 'AND':
                // If all features are present, call the callback with this option
                // as the chosen option.
                matched = true;
                // Set matched to false if any feature is missing.
                for (j in agentRules[agent_name][optionId].features) {
                  if (agentRules[agent_name][optionId].features.hasOwnProperty(j)) {
                    if (feature_strings.indexOf(agentRules[agent_name][optionId].features[j]) === -1) {
                      matched = false;
                      break;
                    }
                  }
                }
                if (matched) {
                  decisions[decision_name] = optionId;
                  callback(decisions);
                  return;
                }
                break;
              default:
                // If any of the features are present, call the callback with this option
                // as the chosen option.
                matched = false;
                // Set matched to true if *any* feature is present.
                for (j in agentRules[agent_name][optionId].features) {
                  if (agentRules[agent_name][optionId].features.hasOwnProperty(j)) {
                    if (feature_strings.indexOf(agentRules[agent_name][optionId].features[j]) !== -1) {
                      matched = true;
                      break;
                    }
                  }
                }
                if (matched) {
                  decisions[decision_name] = optionId;
                  callback(decisions);
                  return;
                }
                break;
            }
          }
        }
      }
      callback(decisions);
    }
  }
})();

Drupal.personalize = Drupal.personalize || {};
Drupal.personalize.agents = Drupal.personalize.agents || {};
Drupal.personalize.agents.acquia_lift_target = {
  'getDecisionsForPoint': function(agent_name, visitor_context, choices, decision_point, fallbacks, callback) {
    // This is the only decision point for this agent and there can only be one
    // decision (i.e. this cannot be an MVT), so we can simplify to one set of
    // options for the agent.
    var choice_array, decisionName;
    for (var i in choices) {
      if (choices.hasOwnProperty(i)) {
        decisionName = i;
        choice_array = choices[decisionName];
        break;
      }
    }
    Drupal.acquia_lift_target.getDecision(agent_name, visitor_context, choice_array, decisionName, fallbacks, callback);
  },
  'sendGoalToAgent': function(agent_name, goal_name, value) {
    // @todo: Introduce the concept of goals in fixed targeting :)
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
