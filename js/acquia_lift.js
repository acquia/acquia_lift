(function ($, Drupal) {

  Drupal.personalize = Drupal.personalize || {};
  Drupal.personalize.agents = Drupal.personalize.agents || {};
  Drupal.personalize.agents.acquia_lift = {
    'getDecisionsForPoint': function(agent_name, visitor_context, choices, decision_point, fallbacks, callback) {
      // Our decision point may have multiple decisions, if doing MVT.
      Drupal.acquiaLift.getDecision(agent_name, choices, decision_point, fallbacks, callback);
    },
    'sendGoalToAgent': function(agent_name, goal_name, goal_value, jsEvent) {
      Drupal.acquiaLift.sendGoal(agent_name, goal_name, goal_value, jsEvent);
    }
  };

  /**
   * Adapter for the Acquia Lift API.
   */
  Drupal.acquiaLift = (function() {

    var settings, api, initialized = false, waitingDecisions = [];

    /**
     * Initialize the API and page level processing.
     *
     * @param initializeSession
     *   If true, also trigger the setting of the session.  This should
     *   only be done by the decisions function and not by the goals function
     *   as the decision functionality allows for setting of the session ID upon
     *   the api result, whereas the goals functionality will not.
     */
    function init(initializeSession) {
      settings = Drupal.settings.acquia_lift;

      api = Drupal.acquiaLiftAPI.getInstance();

      if (initializeSession) {
        api.initializeSessionID();
      }

      $(document).bind('personalizeDecisionsEnd', function(e) {
        if (settings.batchMode) {
          api.batchSend();
        }
      });
      initialized = true;
    }

    function cleanString(str) {
      var regex = new RegExp(settings.stringReplacePattern, "g");
      return str.replace(regex, '-').replace(/\-{2,}/g, '-');
    }

    return {
      // Processes any decisions that have been queued up while the session was
      // initializing.
      'processWaitingDecisions': function() {
        while (waitingDecisions.length > 0) {
          var decision = waitingDecisions.shift();
          this.getDecision(decision.agent_name, decision.choices, decision.point, decision.fallbacks, decision.callback);
        }
      },
      // Processes all decisions for a given decision point.
      'getDecision': function(agent_name, choices, point, fallbacks, callback) {
        var self = this;
        if (!initialized) {
          init(true);
        } else if (api.initializingSession && !api.isManualBatch()) {
          // Add this decision to the queue of waiting decisions.
          waitingDecisions.push({
            'agent_name' : agent_name,
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
          if (!api.getSessionID() && session) {
            api.setSessionID(session);
            Drupal.personalize.saveSessionID(session);
          }
          if (!session) {
            // This means the call to Lift was unsuccessful and we are showing
            // the fallback option. Log this as an error.
            Drupal.personalize.debug('Could not get decision from Lift for ' + agent_name + ', showing fallback option', 5100);
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
          api.initializingSession = false;
          // Process any decisions that have been waiting in the queue.
          self.processWaitingDecisions();
        });
      },

      // Sends a goal to an agent.
      'sendGoal': function(agent_name, goal_name, value, jsEvent) {
        if (!initialized) {
          init(false);
        }
        var options = {
          reward: value,
          goal: goal_name
        };
        Drupal.acquiaLiftUtility.GoalQueue.addGoal(agent_name, options);
      },
      'reset': function() {
        // Reset the initialization of the shared instance.
        if (api) {
          api.reset();
        }
        // Reset the reference.
        api = null;
        initialized = false;
        waitingDecisions = [];
      }
    }
  })();

})(Drupal.jQuery, Drupal);
