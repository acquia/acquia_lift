(function ($) {

Drupal.personalize = Drupal.personalize || {};
Drupal.personalize.agents = Drupal.personalize.agents || {};
Drupal.personalize.agents.acquia_lift = {
  'getDecisionsForPoint': function(agent_name, visitor_context, choices, decision_point, callback) {
    // Our decision point may have multiple decisions, if doing MVT.
    Drupal.acquiaLift.getDecision(agent_name, visitor_context, choices, decision_point, callback);
  },
  'sendGoalToAgent': function(agent_name, goal_name, goal_value, jsEvent) {
    Drupal.acquiaLift.sendGoal(agent_name, goal_name, goal_value, jsEvent);
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
    var prefix, val, feature_string;
    prefix = cleanString(name);
    val = cleanString(value);
    feature_string = prefix + '--' + val;
    // Make a string of the visitor context item in the form name--value
    while (feature_string.length > settings.featureStringMaxLength) {
      // Acquia Lift has a hard character limit for feature strings so
      // if our name--value string is too long, start by whittling down the
      // length of the name part and remove it if necessary.
      if (prefix.length > 1) {
        prefix = prefix.slice(0, prefix.length-1);
        feature_string = prefix + '--' + val;
      }
      else {
        feature_string = val.slice(0, settings.featureStringMaxLength);
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
        this.getDecision(decision.agent_name, decision.visitor_context, decision.choices, decision.point, decision.callback);
      }
    },
    // Processes all decisions for a given decision point.
    'getDecision': function(agent_name, visitor_context, choices, point, callback) {
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
          'callback' : callback
        });
        return;
      }

      // Prepare the options for our decision.
      var options = {
        // Acquia Lift won't accept a string with slashes in it as a
        // point code. Convert to double underscore.
        point: cleanString(point),
        choices: choices
      };
      if (sessionID) {
        options.session = sessionID;
      }
      // Process visitor_context
      var data = [], i, feature_string;
      for (i in visitor_context) {
        if (visitor_context.hasOwnProperty(i)) {
          feature_string = convertContextToFeatureString(i, visitor_context[i]);
          data.push(feature_string);
        }

      }
      if (data.length > 0) {
        options.features = data.join(',');
      }

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
