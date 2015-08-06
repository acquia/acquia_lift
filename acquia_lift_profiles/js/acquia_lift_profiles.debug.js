/**
 * Defines JavaScript functions needed by the Acquia Lift debugger in
 * order to provide customer profile information.
 */

(function ($, Drupal) {

      function getFromCookie ( query )
    {
      var cookie = document.cookie;
      var regex = new RegExp('(?:^|;)\\s?' + query + '=(.*?)(?:;|$)','i');
      var match = cookie.match(regex);
      return match && window.decodeURIComponent(match[1]);
    };

  Drupal.acquiaLiftProfilesDebug = (function() {

    //_tcwq.push( ["setDebug", true] );
    var debugPrefix = "acquiaLift::debug::"
    var curSegmentCapture = {};
    var curSegments = [];
    var curSegmentsOverride;
    var curIdentities = [];
    var message;
    var code;
    $(document).on("segmentsUpdated", function( event, data, capture ) {
      curSegmentCapture = capture;
      if(data){
        curSegments = data["segments"];
        curSegmentsOverride =  JSON.parse(window.sessionStorage.getItem(debugPrefix + "overrideSegments"));
        if ( curSegmentsOverride ) {
          if(data["segments"]){
            data["segments"].length = 0;
            $(curSegmentsOverride).each( function(index,overrideSegment) { data["segments"].push(overrideSegment); } );
          }
          curSegments = curSegmentsOverride.splice(0);

        }
        message = "Segments Returned: " + curSegments;
        code = 1000;
      }else{
        message = "No Data found"
        code = 3001;
      }
      Drupal.personalizeDebug.log( message , code, 'Lift Web');
    });
    $(document).bind("identitiesAdded", function( event, identities ) {
      // TODO: Check if the person id changed because TC_CONF.userIdentitySourceInTrackingId is true
      if(identities){
      $.each(identities, function (index, identity) { curIdentities.push( identity ); } );
        
      }
    });

    return {
      'getPersonId' : function() {
        return getFromCookie("tc_ptid");
      },

      'getTouchId' : function() {
        return getFromCookie("tc_ttid");
      },

      'isThirdPartyPersonId' : function() {
        return TC_CONF && TC_CONF.thirdPartyCookie == true;
      },

      'getCurrentSegments' : function() {
        return curSegments;
      },

      'setOverrideSegments' : function( segmentsOverride ) {
        if (segmentsOverride){
          window.sessionStorage.setItem(debugPrefix + "originalSegments",JSON.stringify(curSegments));
          window.sessionStorage.setItem(debugPrefix + "overrideSegments",JSON.stringify(segmentsOverride));
        }else{
          window.sessionStorage.removeItem(debugPrefix + "::overrideSegments");
        }
        curSegmentsOverride = segmentsOverride;
      },

      'getOverrideSegments' : function() {
        return curSegmentsOverride;
      },

      'getAdditionalIdentities' : function() {
        return curIdentities;
      },

      'openProfile' : function() {
        window.open( Drupal.settings.acquia_lift_profiles.apiUrl + '#person:'+this.getPersonId()+','+
            Drupal.settings.acquia_lift_profiles.accountName );
      },

      'getAllSegments' : function () {
        return Drupal.settings.acquia_lift_profiles.available_segments;
      },


      'evaluateSegment' : function( segmentName, callback ) {
        var personId = this.getPersonId();
        var touchId = this.getTouchId();
        _tcwq.push( ["getApiUrl", function( apiUrl ) {
          jQuery.ajax( {
            url : apiUrl + "?tcla="+window.encodeURIComponent(Drupal.settings.acquia_lift_profiles.account_name)+
            "&tcptid="+window.encodeURIComponent(personId)+
            "&tcttid="+window.encodeURIComponent(touchId)+
            "&evalSegment="+window.encodeURIComponent(segmentName),
            cache : false,
            dataType : "json",
            success : function( data, text, request ) {
              callback( data );
            }
          });
        }]);
      },

      'clearStorage' : function (data){
        $(document).trigger('personalizeDebugClear', debugPrefix);
      },

      'turnOffDebugMode' : function(){
          var href = window.location.href;
          href = href.replace(/((\?|\&)acquia_lift_debug_mode=[^]*)/g,"");
          if(href.indexOf('?') > 0){
            href+='&';
          }else{
            href+='?';
          }
          href += 'acquia_lift_debug_mode=0'
          window.location.href = href;
      }
    };
  })();
})(jQuery, Drupal);
