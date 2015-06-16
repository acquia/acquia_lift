/**
 * Defines JavaScript functions needed by the Acquia Lift debugger in
 * order to provide customer profile information.
 */

(function ($, Drupal) {

  Drupal.acquiaLiftProfilesDebug = (function() {

    function getFromCookie ( query )
    {
      var cookie = _cookie;
      var regex = new RegExp('(?:^|;)\\s?' + query + '=(.*?)(?:;|$)','i');
      var match = cookie.match(regex);
      return match && window.decodeURIComponent(match[1]);
    };

    _tcwq.push( ["setDebug", true] );

    var curSegmentCapture = {};
    var curSegments = [];
    var curSegmentsOverride;
    var curIdentities = [];

    $(document).bind("segmentsUpdated", function( event, segments, capture ) {
      curSegmentCapture = capture;
      curSegments = segments;
      if ( curSegmentsOverride ) {
        segments.length = 0;
        $(curSegmentsOverride).each( function(index,overrideSegment) { segments.push(overrideSegment); } );
      }
      Drupal.personalizeDebug.log( "Segments Returned", "INFO", curSegments);
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
        window.open( Drupal.Settings.lift_user_profiles.apiUrl + '#person:'+this.getPersonId()+','+
        Drupal.Settings.lift_user_profiles.accountName );
      },

      'getAllSegments' : function( callback ) {
        jQuery.ajax( {
          url : Drupal.Settings.lift_user_profiles.apiUrl + '/dashboard/rest/'+
          Drupal.Settings.lift_user_profiles.accountName + '/segments',
          cache : false,
          dataType : 'json',
          success : function( data, text, request ) {
            callback( data );
          }
        });
      },

      'evaluateSegment' : function( segmentName, callback ) {
        var personId = this.getPersonId();
        var touchId = this.getTouchId();
        _tcwq.push( ["getApiUrl", function( apiUrl ) {
          jQuery.ajax( {
            url : apiUrl + "?tcla="+window.encodeURIComponent(Drupal.Settings.lift_user_profiles.accountName)+
            "&tcptid="+window.encodeURIComponent(personId)+
            "&tcttid="+window.encodeURIComponent(touchId)+
            "&evalSegment="+window.encodeURIComponent(segmentName),
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
