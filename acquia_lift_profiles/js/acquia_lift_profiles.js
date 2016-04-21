var _tcaq = _tcaq || [];
var _tcwq = _tcwq || [];

(function ($) {
  function generateTrackingId(){
    var d = new Date().getTime();
    var uuid = 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replace(/[xy]/g,
      function(c) {
        var r = (d + Math.random()*16)%16 | 0;
        d = Math.floor(d/16);
        return (c=='x' ? r : (r&0x7|0x8)).toString(16);
      });
    return uuid;
  }
  var trackingId = generateTrackingId();

  Drupal.behaviors.acquia_lift_profiles = {
    'attach': function (context, settings) {
      settings.acquia_lift_profiles = settings.acquia_lift_profiles || {};
      Drupal.acquia_lift_profiles.init(settings);
      Drupal.acquia_lift_profiles.addActionListener(settings);
      Drupal.acquia_lift_profiles.processServerSideActions(settings);
    }
  };

  /**
   * Handles storage and retrieval of segments in the cache.
   */
  var segmentCache = (function() {
    var visitorSegments = null;
    return {
      'store': function(segments) {
        visitorSegments = {};
        for (var i = 0; i < segments.length; i++){
          visitorSegments[segments[i]] = 1;
        }
        // Go through all available segments and add an entry in visitorSegments for
        // each one that the visitor does *not* have.
        if (Drupal.settings.acquia_lift_profiles.available_segments) {
          for (var j in Drupal.settings.acquia_lift_profiles.available_segments) {
            if (Drupal.settings.acquia_lift_profiles.available_segments.hasOwnProperty(j)) {
              var segmentName = Drupal.settings.acquia_lift_profiles.available_segments[j];
              if (!visitorSegments.hasOwnProperty(segmentName)) {
                visitorSegments[segmentName] = 0;
              }
            }
          }
        }
        return visitorSegments;
      },
      'retrieve': function() {
        return visitorSegments;
      },
      'reset': function() {
        visitorSegments = null;
      }
    }
  })();

  Drupal.personalize = Drupal.personalize || {};
  Drupal.personalize.visitor_context = Drupal.personalize.visitor_context || {};
  Drupal.personalize.visitor_context.acquia_lift_profiles_context = {
    'getContext': function(enabled) {
      if ($.cookie('tc_dnt') === "true") {
        return {};
      }

      var i, j, context_values = {};
      // First check to see if we have the acquia_lift_profiles segments stored
      // already.
      var cached = segmentCache.retrieve();
      if (cached) {
        for (i in enabled) {
          if (enabled.hasOwnProperty(i) && cached.hasOwnProperty(i)) {
            context_values[i] = cached[i];
          }
        }
        return context_values;
      }

      // If not, use a Promise to wait for segments to be stored.
      return new Promise(function(resolve, reject){
        var milliseconds_limit = Drupal.personalize.contextTimeout || 5000,
          milliseconds_step = 50,
          milliseconds_count = 0,
          // Create the interval callback function that's called periodically.
          resolveWhenSegmentsAreCached = function() {
            // Time out, if the time limit has been reached.
            if (milliseconds_count > milliseconds_limit) {
              clearInterval(segments_interval);
              reject(new Error('Could not retrieve segments in time.'));
            }
            milliseconds_count += milliseconds_step;

            cached = segmentCache.retrieve();
            // If Segments are not stored yet, abort the current attempt.
            if (!cached) {
              return;
            }
            // Otherwise, segments are stored, now resolve using the segments.
            for (i in enabled) {
              if (enabled.hasOwnProperty(i) && cached.hasOwnProperty(i)) {
                context_values[i] = cached[i];
              }
            }
            clearInterval(segments_interval);
            resolve(context_values);
          },
          // Kick off the interval callbacks.
          segments_interval = setInterval(resolveWhenSegmentsAreCached, milliseconds_step);
      });
    }
  };

  // Keeps track of whether we've captured identity or not.
  var identityCaptured = false;

  /**
   * Centralized functionality for acquia_lift_profiles behavior.
   */
  Drupal.acquia_lift_profiles = (function(){

    var processedListeners = {}, initialized = false, initializing = false, pageFieldValues = {};

    return {
      'init': function(settings) {
        if (initialized || initializing || !settings.acquia_lift.hasOwnProperty('account_name')) {
          return;
        }
        initializing = true;

        var mappings = settings.acquia_lift_profiles.mappings, context_separator = settings.acquia_lift_profiles.mappingContextSeparator, plugins = {}, reverseMapping = {};
        for(var type in mappings) {
          if (mappings.hasOwnProperty(type)) {
            for (var udf in mappings[type]) {
              if (mappings[type].hasOwnProperty(udf)) {
                // We maintain a reverse mapping of all the UDFs that use each
                // context, so we can easily assign values once the contexts have
                // been retrieved.
                if (!reverseMapping.hasOwnProperty(mappings[type][udf])) {
                  reverseMapping[mappings[type][udf]] = [];
                }
                reverseMapping[mappings[type][udf]].push(udf);
                var context = mappings[type][udf].split(context_separator);
                var pluginName = context[0];
                var context_name = context[1];
                if (!plugins.hasOwnProperty(pluginName)) {
                  plugins[pluginName] = {};
                }
                plugins[pluginName][context_name] = context_name;
              }
            }
          }
        }

        var callback = function(contextValues) {
          if (initialized) {
            return;
          }
          for (var pluginName in contextValues) {
            if (contextValues.hasOwnProperty(pluginName)) {
              for (var contextName in contextValues[pluginName]) {
                if (contextValues[pluginName].hasOwnProperty(contextName)) {
                  var fullContextName = pluginName + context_separator + contextName;
                  if (reverseMapping.hasOwnProperty(fullContextName)) {
                    // Set this is as the value for all UDFs that use this context.
                    for (var i in reverseMapping[fullContextName]) {
                      if (reverseMapping[fullContextName].hasOwnProperty(i)) {
                        pageFieldValues[reverseMapping[fullContextName][i]] = contextValues[pluginName][contextName];
                      }
                    }
                  }
                }
              }
            }
          }

          // Create a segment callback function.
          var segmentCallback = function (segmentIds, captureInfo) {
            if (captureInfo.x['trackingId'] == trackingId) {
              segmentCache.store(segmentIds);
            }
          },
          // Ensure sensible defaults for our capture data.
          pageInfo = $.extend({
            'content_title': 'Untitled',
            'content_type': 'page',
            'page_type': 'content page',
            'content_section': '',
            'content_keywords': '',
            'post_id': '',
            'published_date': '',
            'thumbnail_url': '',
            'persona': '',
            'engagement_score':'1',
            'author':'',
            'evalSegments': true,
            'trackingId': trackingId
          }, settings.acquia_lift_profiles.pageContext, pageFieldValues);
          _tcwq.push(['onLoad', segmentCallback]);

          // Capture view.
          var captureViewEvent = ['captureView', 'Content View', pageInfo];

          // Capture identity in addition, if applicable.
          if(!identityCaptured && settings.acquia_lift_profiles.hasOwnProperty('identity')) {
            var identity = {};
            identity[settings.acquia_lift_profiles.identity] = settings.acquia_lift_profiles.identityType;
            captureViewEvent.push({'identity': identity});
            identityCaptured = true;
          }

          _tcaq.push(captureViewEvent);

          initialized = true;
        };

        //checks if debug mode is on. sets the debug mode for tcwidget
        if(Drupal.settings.acquia_lift.isDebugMode){
          _tcwq.push( ["setDebug", true]);
        }else{
          _tcwq.push(["setDebug", false]);
        }

        Drupal.personalize.getVisitorContexts(plugins, callback);
      },
      'getTrackingID': function () {
        return trackingId;
      },
      'clearSegmentMemoryCache': function() {
        segmentCache.reset();
      },
      /**
       * Sends an event to TC.
       *
       * @param eventName
       */
      'processEvent': function(eventName, settings, context) {
        var engagement_scores = settings.acquia_lift_profiles.engagement_scores,
          global_values = settings.acquia_lift_profiles.global_values,
          engagement_score = engagement_scores.hasOwnProperty(eventName) ? engagement_scores[eventName] : 1,
          global_value = global_values.hasOwnProperty(eventName) ? global_values[eventName] : 1,
          extra = {
            engagement_score: engagement_score,
            targetgoalvalue: global_value,
            evalSegments: true
          };
        // Add the field and UDF values to the extra info we're sending about the event. The assumption
        // here is that this event is being processed *after* the initial page view capture has
        // already retrieved all the visitor context values. Since this happens asynchronously
        // it is not guaranteed that this is the case.
        $.extend(extra, pageFieldValues);

        // Capture view.
        var captureEvent = ['capture', eventName, extra];

        // Capture identity in addition, if applicable.
        if (settings.acquia_lift_profiles.captureIdentity &&
          !identityCaptured &&
          (eventName === 'user_login' || eventName === 'user_register') &&
          context['mail']) {
          var identity = {};
          identity[context['mail']] = 'email';
          captureEvent.push({'identity': identity});
          identityCaptured = true;
        }

        _tcaq.push(captureEvent);
      },

      /**
       * Add an action listener for client-side goal events.
       */
      'addActionListener': function (settings) {
        if (Drupal.hasOwnProperty('visitorActions')) {
          var events = {}, new_events = 0;
          for (var i in settings.acquia_lift_profiles.tracked_actions) {
            if (settings.acquia_lift_profiles.tracked_actions.hasOwnProperty(i) && !processedListeners.hasOwnProperty(settings.acquia_lift_profiles.tracked_actions[i])) {
              var eventName = settings.acquia_lift_profiles.tracked_actions[i];
              processedListeners[eventName] = 1;
              events[eventName] = 1;
              new_events++;
            }
          }
          if (new_events > 0) {
            var self = this;
            var callback = function(eventName, jsEvent, context) {
              if (events.hasOwnProperty(eventName)) {
                self.processEvent(eventName, settings, {});
              }
            };
            Drupal.visitorActions.publisher.subscribe(callback);
          }
        }
      },

      /**
       * Goes through the server-side actions and calls the appropriate function for
       * each one, passing in the event context.
       *
       * @param settings
       */
      'processServerSideActions': function (settings) {
        if (settings.acquia_lift_profiles.serverSideActions) {
          for (var actionName in settings.acquia_lift_profiles.serverSideActions) {
            if (settings.acquia_lift_profiles.serverSideActions.hasOwnProperty(actionName)) {
              for (var i in settings.acquia_lift_profiles.serverSideActions[actionName]) {
                if (settings.acquia_lift_profiles.serverSideActions[actionName].hasOwnProperty(i) && !settings.acquia_lift_profiles.serverSideActions[actionName][i].processed) {
                  // Process the event.
                  this.processEvent(actionName, settings, settings.acquia_lift_profiles.serverSideActions[actionName][i]);
                  // Mark this event has having been processed so that it doesn't get sent again.
                  settings.acquia_lift_profiles.serverSideActions[actionName][i].processed = 1;
                }
              }
            }
          }
        }
      },

      /**
       * Helper function to reset variables during tests.
       */
      'resetAll' : function() {
        processedListeners = {};
        initialized = false;
        initializing = false;
        identityCaptured = false;
        pageFieldValues = {};
      }
    }
  })();

})(jQuery);
