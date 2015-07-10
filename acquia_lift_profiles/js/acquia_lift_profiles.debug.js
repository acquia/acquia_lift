/**
 * Defines JavaScript functions needed by the Acquia Lift debugger in
 * order to provide customer profile information.
 */

(function ($, Drupal) {

  Drupal.acquiaLiftProfilesDebug = (function() {

    function getFromCookie ( query )
    {
      var cookie = document.cookie;
      var regex = new RegExp('(?:^|;)\\s?' + query + '=(.*?)(?:;|$)','i');
      var match = cookie.match(regex);
      return match && window.decodeURIComponent(match[1]);
    };

    //_tcwq.push( ["setDebug", true] );

    var curSegmentCapture = {};
    var curSegments = [];
    var curSegmentsOverride=[];
    var curIdentities = [];

    $(document).on("segmentsUpdated", function( event, data, capture ) {
      curSegmentCapture = capture;
      curSegments = data["segments"];
      if ( curSegmentsOverride ) {
        //data["segments"].length = 0;
        //$(curSegmentsOverride).each( function(index,overrideSegment) { segments.push(overrideSegment); } );
      }
      var message = "Segments Returned: " + curSegments;
      Drupal.personalizeDebug.log( message , 1000);
    });
    $(document).bind("identitiesAdded", function( event, identities ) {
      // TODO: Check if the person id changed because TC_CONF.userIdentitySourceInTrackingId is true
      $(identities).each( function (index, identity) { curIdentities.push( identity ); } );
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

      //'getAllSegments' : function( callback ) {
      //  jQuery.ajax( {
      //    url : Drupal.settings.acquia_lift_profiles.apiUrl + '/dashboard/rest/'+
      //    Drupal.settings.acquia_lift_profiles.account_name + '/segments',
      //    cache : false,
      //    dataType : 'json',
      //    success : function( data, text, request ) {
      //      callback( data );
      //    }
      //  });
      //},

      'evaluateSegment' : function( segmentName, callback ) {
        var personId = this.getPersonId();
        var touchId = this.getTouchId();
        _tcwq.push( ["getApiUrl", function( apiUrl ) {
          jQuery.ajax( {
            url : apiUrl + "?tcla="+window.encodeURIComponent(Drupal.settings.acquia_lift_profiles.account_name)+
            "&tcptid="+window.encodeURIComponent(personId)+
            "&tcttid="+window.encodeURIComponent(touchId)+
            "&evalSegment=TRUE",//+window.encodeURIComponent(segmentName),
            cache : false,
            dataType : "jsonp",
            success : function( data, text, request ) {
              callback( data );
            }
          });
        }]);
      }
    };
  })();
})(Drupal.jQuery, Drupal);
