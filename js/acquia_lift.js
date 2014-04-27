(function ($) {

Drupal.personalize = Drupal.personalize || {};
Drupal.personalize.agents = Drupal.personalize.agents || {};
Drupal.personalize.agents.acquia_lift = {
  'getDecisionsForPoint': function(agent_name, visitor_context, choices, decision_point, fallbacks, callback) {
    // Our decision point may have multiple decisions, if doing MVT.
    Drupal.acquiaLift.getDecision(agent_name, visitor_context, choices, decision_point, fallbacks, callback);
  },
  'sendGoalToAgent': function(agent_name, goal_name, goal_value, jsEvent) {
    Drupal.acquiaLift.sendGoal(agent_name, goal_name, goal_value, jsEvent);
  },
  'featureToContext': function(featureString) {
    var contextArray = featureString.split(Drupal.settings.acquia_lift.featureStringSeparator);
    return {
      'key': contextArray[0],
      'value': contextArray[1]
    };
  }
};

/**
 * Adapter for the Acquia Lift API.
 */
Drupal.acquiaLift = (function() {

  var settings, api, initialized = false, initializingSession = false, sessionID = false, waitingDecisions = [];

  function init() {
    settings = Drupal.settings.acquia_lift;
    var options = {
        'cookies': null, // we provide our own cookie support
        'scodestore': false,
        'server': settings.baseUrl
    };
    if (!sessionID) {
      sessionID = Drupal.personalize.initializeSessionID();
    }

    // At this stage we still may not have a session ID.
    if (sessionID) {
      options.session = sessionID;
    }
    else {
      // This variable ensures subsequent requests for decisions will get
      // queued up until the first decision comes back from Acquia Lift
      // and the session ID gets set.
      initializingSession = true;
    }
    api = new AcquiaLiftJS(
      settings.owner,
      settings.apiKey,
      options
    );
    initialized = true;
  }

  function cleanString(str) {
    var regex = new RegExp(settings.featureStringReplacePattern, "g");
    return str.replace(regex, '-').replace(/\-{2,}/g, '-');
  }

  function convertContextToFeatureString(name, value) {
    var prefix, val, feature_string, separator = settings.featureStringSeparator;
    prefix = cleanString(name);
    val = cleanString(value);
    // Make a string of the visitor context item in the format Acquia Lift can
    // consume.
    feature_string = prefix + separator + val;

    var prefixMaxLength = Math.floor((settings.featureStringMaxLength - separator.length) / 2);
    while (feature_string.length > settings.featureStringMaxLength) {
      // Acquia Lift has a hard character limit for feature strings.
      if (prefix.length > prefixMaxLength) {
        // Start by truncating the prefix down to half the max length.
        prefix = prefix.slice(0, prefixMaxLength);
        feature_string = prefix + separator + val;
      }
      else {
        // Otherwise just truncate the whole thing down to the max length.
        feature_string = feature_string.slice(0, settings.featureStringMaxLength);
      }
    }
    return feature_string;
  }

  return {
    // Processes any decisions that have been queued up while the session was
    // initializing.
    'processWaitingDecisions': function() {
      while (waitingDecisions.length > 0) {
        var decision = waitingDecisions.shift();
        this.getDecision(decision.agent_name, decision.visitor_context, decision.choices, decision.point, decision.fallbacks, decision.callback);
      }
    },
    // Processes all decisions for a given decision point.
    'getDecision': function(agent_name, visitor_context, choices, point, fallbacks, callback) {
      var self = this;
      if (!initialized && !initializingSession) {
        init();
      }
      else if (initializingSession) {
        // Add this decision to the queue of waiting decisions.
        waitingDecisions.push({
          'agent_name' : agent_name,
          'visitor_context' : visitor_context,
          'choices' : choices,
          'point' : point,
          'fallbacks' : fallbacks,
          'callback' : callback
        });
        return;
      }

      // Prepare the options for our decision.
      var options = {
        point: cleanString(point),
        choices: choices
      };
      if (sessionID) {
        options.session = sessionID;
      }
      // Process visitor_context
      var data = [], i, j, feature_string;
      for (i in visitor_context) {
        if (visitor_context.hasOwnProperty(i)) {
          for (j in visitor_context[i]) {
            if (visitor_context[i].hasOwnProperty(j)) {
              feature_string = convertContextToFeatureString(i, visitor_context[i][j]);
              data.push(feature_string);
            }
          }
        }
      }
      if (data.length > 0) {
        options.features = data.join(',');
      }

      // Format the fallbacks object into the structure required by the Acquia Lift
      // client.
      var fb = {};
      for (var key in fallbacks) {
        if (fallbacks.hasOwnProperty(key) && choices.hasOwnProperty(key)) {
          fb[key] = {code: choices[key][fallbacks[key]]}
        }
      }
      options.fallback = fb;
      api.decision(agent_name, options, function(selection, session) {
        if (window.console) {
          console.log(selection);
        }
        if (!sessionID && session) {
          sessionID = session;
          Drupal.personalize.saveSessionID(session);
        }

        // We need to send back an object with decision names as keys
        // and the chosen option for each one as the value.
        var decisions = {};
        for (var key in selection) {
          if (selection.hasOwnProperty(key)) {
            decisions[key] = selection[key].code;
          }
        }
        callback(decisions);
        // Now unblock all future decision requests.
        initializingSession = false;
        // Process any decisions that have been waiting in the queue.
        self.processWaitingDecisions();
      });
    },

    // Sends a goal to an agent.
    'sendGoal': function(agent_name, goal_name, value, jsEvent) {
      var callback = Drupal.visitorActions.preventDefaultCallback(jsEvent);
      if (!initialized) {
        init();
      }
      options = {
        reward: value,
        goal: goal_name
      };
      api.goal(agent_name, options, function(response, textStatus, jqXHR) {
        if (typeof callback === 'function') {
          callback();
          callback = null;
        }
        if (window.console) {
          console.log(response);
        }
      });
    }
  }
})();

})(jQuery);
