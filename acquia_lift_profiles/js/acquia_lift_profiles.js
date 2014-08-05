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
  var trackingId = generateTrackingId(), plugin = 'acquia_lift_profiles_context';


  Drupal.behaviors.acquia_lift_profiles = {
    'attach': function (context, settings) {
      Drupal.acquia_lift_profiles.init(settings);
      Drupal.acquia_lift_profiles.addActionListener(settings);
      processServerSideActions(settings);
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
          // Store this in localStorage so it can be retrieved for use as Lift
          // visitor context.
          Drupal.personalize.visitor_context_write(segments[i], plugin, 1);
        }
        // Go through all available segments and add an entry in localStorage for
        // each one that the visitor does *not* have.
        if (Drupal.settings.acquia_lift_profiles.available_segments) {
          for (var j in Drupal.settings.acquia_lift_profiles.available_segments) {
            if (Drupal.settings.acquia_lift_profiles.available_segments.hasOwnProperty(j)) {
              var segmentName = Drupal.settings.acquia_lift_profiles.available_segments[j];
              if (!visitorSegments.hasOwnProperty(segmentName)) {
                visitorSegments[segmentName] = 0;
                Drupal.personalize.visitor_context_write(segmentName, plugin, 0);
              }
            }
          }
        }
        return visitorSegments;
      },
      'retrieve': function(settings) {
        var i, val, segmentName, segments = settings.available_segments;
        for (i in segments) {
          if (segments.hasOwnProperty(i)) {
            segmentName = segments[i];
            if (visitorSegments === null || !visitorSegments.hasOwnProperty(segmentName)) {
              val = Drupal.personalize.visitor_context_read(segmentName, plugin);
              if (val !== null) {
                visitorSegments = visitorSegments || {};
                visitorSegments[segmentName] = val;
              }
            }
          }
        }
        return visitorSegments;
      }
    }
  })();

  Drupal.personalize = Drupal.personalize || {};
  Drupal.personalize.visitor_context = Drupal.personalize.visitor_context || {};
  Drupal.personalize.visitor_context.acquia_lift_profiles_context = {
    'getContext': function(enabled) {
      var i, j, context_values = {};
      // First check to see if we have the acquia_lift_profiles segments already stored
      // locally, either in our closure variable or in localStorage.
      var cached = segmentCache.retrieve(Drupal.settings.acquia_lift_profiles);
      if (cached) {
        for (i in enabled) {
          if (enabled.hasOwnProperty(i) && cached.hasOwnProperty(i)) {
            context_values[i] = cached[i];
          }
        }
        return context_values;
      }

      return new Promise(function(resolve, reject){
        // Define a callback function to receive information about the segments
        // for the current visitor and add them to the visitorSegments object.
        var segmentsCallback = function (segmentIds, captureInfo) {
          if (captureInfo.x['trackingId'] == trackingId) {
            var allSegments = segmentCache.store(segmentIds);
            for (j in enabled) {
              if (enabled.hasOwnProperty(j) && allSegments.hasOwnProperty(j)) {
                context_values[j] = allSegments[j];
              }
            }
            resolve(context_values);
          }
        };

        // Register our callback for receiving segments.
        _tcwq.push(["onLoad", segmentsCallback]);
      });
    }
  };

  // Keeps track of whether we've captured identity or not.
  var identityCaptured = false;

  /**
   * Sends a captureIdentity event to TC using the email address from the
   * passed in context.
   *
   * @param DrupalSettings
   *   An object containing the Acquia Lift Profiles settings from the server side.
   * @param context
   *   An object that must at least have a 'mail' property.
   */
  var pushCaptureEmail = function(DrupalSettings, context) {
    // Do nothing if identity has already been captured or should not be captured or
    // if we don't have an email address in the context.
    if (identityCaptured || !(DrupalSettings.captureIdentity && context['mail'])) {
      return;
    }
    _tcaq.push( [ 'captureIdentity', context['mail'], 'email' ] );
    identityCaptured = true;
  };

  /**
   * Centralized functionality for acquia_lift_profiles behavior.
   */
  Drupal.acquia_lift_profiles = (function(){

    // Keeps track of processed listeners so we don't subscribe them more than once.
    var processedListeners = {}, initialized = false, initializing = false;

    return {
      'init': function(settings) {
        if (initialized || initializing) {
          return;
        }
        initializing = true;
        var mappings = settings.acquia_lift_profiles.udfMappings, context_separator = settings.acquia_lift_profiles.udfMappingContextSeparator, plugins = {}, udfValues = {}, reverseMapping = {};
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
          for (var pluginName in contextValues) {
            if (contextValues.hasOwnProperty(pluginName)) {
              for (var contextName in contextValues[pluginName]) {
                if (contextValues[pluginName].hasOwnProperty(contextName)) {
                  var fullContextName = pluginName + context_separator + contextName;
                  if (reverseMapping.hasOwnProperty(fullContextName)) {
                    // Set this is as the value for all UDFs that use this context.
                    for (var i in reverseMapping[fullContextName]) {
                      if (reverseMapping[fullContextName].hasOwnProperty(i)) {
                        udfValues[reverseMapping[fullContextName][i]] = contextValues[pluginName][contextName];
                      }
                    }
                  }
                }
              }
            }
          }

          // Ensure sensible defaults for our capture data.
          var pageInfo = $.extend({
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
          }, settings.acquia_lift_profiles.pageContext, udfValues);
          _tcaq.push( [ 'captureView', 'Content View', pageInfo ] );

          initialized = true;
        };
        Drupal.personalize.getVisitorContexts(plugins, callback);
      },
      /**
       * Sends an event to TC.
       *
       * @param eventName
       */
      'processEvent': function(eventName, settings, context) {
        // Send to acquia_lift_profiles.
        _tcaq.push(['capture', eventName]);
        // If it's a special event with some other callback associated with it, call that
        // callback as well.
        if (typeof this.specialEvents[eventName] == 'function') {
          this.specialEvents[eventName].call(this, settings.acquia_lift_profiles, context);
        }
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

      // Holds the functions that should be called for particular events.
      'specialEvents': {
        'user_login': pushCaptureEmail,
        'user_register': pushCaptureEmail
      }
    }
  })();

  /**
   * Goes through the server-side actions and calls the appropriate function for
   * each one, passing in the event context.
   *
   * @param settings
   */
  function processServerSideActions(settings) {
    if (settings.acquia_lift_profiles.serverSideActions) {
      for (var actionName in settings.acquia_lift_profiles.serverSideActions) {
        if (settings.acquia_lift_profiles.serverSideActions.hasOwnProperty(actionName)) {
          for (var i in settings.acquia_lift_profiles.serverSideActions[actionName]) {
            if (settings.acquia_lift_profiles.serverSideActions[actionName].hasOwnProperty(i) && !settings.acquia_lift_profiles.serverSideActions[actionName][i].processed) {
              // Process the event.
              Drupal.acquia_lift_profiles.processEvent(actionName, settings, settings.acquia_lift_profiles.serverSideActions[actionName][i]);
              // Mark this event has having been processed so that it doesn't get sent again.
              settings.acquia_lift_profiles.serverSideActions[actionName][i].processed = 1;
            }
          }
        }
      }
    }
  }


})(jQuery);
