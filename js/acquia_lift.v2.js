/**
 * Contains the definition of the Acquia Lift API.
 *
 * The abstraction of the API wrapper into its own singleton allows different
 * parts of the system to initialize and use the API.
 */

(function ($, Drupal) {
  var instance;
  Drupal.acquiaLiftV2API = (function () {

    /**
     * The Singleton API instance.  All callers will be accessing the functions
     * defined here.
     *
     */
    function SingletonAPI(session_id)  {
      var settings = Drupal.settings.acquia_lift_learn;
      this.options = {
        'server': settings.baseUrl,
        'user_hash': session_id,
        'application_hash': settings.applicationHash,
        'client_id': settings.clientId
      };

    }


    /**
     * Instance of the API that is handed out.
     */
    SingletonAPI.prototype = {
      constructor: SingletonAPI,

      decision: function (agent_name, options, callback) {
        var params = {
          client_id: this.options.client_id,
          user_hash: this.options.user_hash,
          campaign_id: agent_name,
          application_hash: this.options.application_hash
        };
        var fb, path = 'play';
        fb = null;
        if (options.fallback != null) {
          fb = options.fallback;
        }
        return this.send(path, params, null, (function() {
          return function(res) {
            var selection = fb;
            if (res && res.hasOwnProperty('outcome')) {
              selection = res.outcome;
            }
            return callback(selection);
          };
        })(this));
      },
      goal: function(agent_name, options, callback) {
        var path = 'feedback';
        var params = {
          client_id: this.options.client_id
        };
        var body = {
          'user_hash': this.options.user_hash,
          'application_hash': this.options.application_hash,
          'campaign_id': agent_name,
          'goal_id': options.goal,
          'score': +options.reward
        };
        return this.send(path, params, body, (function(_this) {
          return function(res) {
            var success, nodecision, accepted, retryable;
            if (callback == null) {
              return;
            }
            success = res != null && res.hasOwnProperty('feedback_id');
            nodecision = res != null && res.hasOwnProperty('error') && res.error.indexOf("The request has been accepted for processing") === 0;
            accepted = success || nodecision;
            retryable = (res != null ? res.submitted : void 0) == null;
            return callback(accepted, _this.options.user_hash, retryable);
          };
        })(this));
      },
      reset: function () {
        instance = undefined;
      },
      send: function(path, data, body, cb) {
        var postBody = body ? JSON.stringify(body) : "";
        data._t = new Date().getTime();

        // Encode querystring variables.
        var querystring = '', key, value, i = 0;
        for (key in data) {
          value = data[key];
          querystring += i == 0 ? "" : "&";
          querystring += key + "=" + (encodeURI(value));
          i++;
        }
        var url = this.options.server + "/" + path + "?" + querystring;
        return microAjax(url, function(text) {
          var response;
          try {
            response = JSON.parse(text);
            return cb(response);
          } catch (e) {
            return cb(null);
          }
        }, postBody);
      }
    };

    // Return the static instance initializer.
    return  {
      name:  "AcquiaLiftV2API",
      getInstance:  function() {
        // @todo Remove this once we can just pass the session id in the
        //   getInstance call (i.e. once V1's js has gone away.
        var session_id = Drupal.acquiaLiftLearn.getSessionID();
        if (instance  ===  undefined) {
          instance = new SingletonAPI(session_id);
        }
        return instance;
      }
    };

  })();

  // Copyright (c) 2008 Stefan Lange-Hegermann
  // Adapted from the MicroAjax library here:
  // https://code.google.com/p/microajax and modified for
  // CORS support based on this post:
  // http://www.nczonline.net/blog/2010/05/25/cross-domain-ajax-with-cross-origin-resource-sharing/
  function microAjax(url, callbackFunction) {
    this.getRequest = function(method, url) {
      var xhr = new XMLHttpRequest();
      if ("withCredentials" in xhr){
        xhr.open(method, url, true);
      } else if (typeof XDomainRequest != "undefined"){
        xhr = new XDomainRequest();
        xhr.open(method, url);
      } else {
        xhr = null;
      }
      return xhr;
    };

    this.postBody = (arguments[2] || "");
    var method = this.postBody !== "" ? 'POST' : 'GET';
    this.request = this.getRequest(method, url);

    if(this.request) {
      var req = this.request;
      req.timeout = 5000;
      req.onload = function() {
        return callbackFunction(req.responseText);
      };
      req.onerror = req.ontimeout = function() {
        return callbackFunction(null);
      };

      if (method=='POST') {
        req.setRequestHeader('Content-Type', 'application/json');
      }

      req.send(this.postBody);
    }
    else {
      return callbackFunction(null);
    }
  }
})(Drupal.jQuery, Drupal);
