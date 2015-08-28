/**
 * Defines JavaScript functions needed by the Acquia Lift debugger in
 * order to provide customer profile information.
 */
(function ($, Drupal, Storage) {

  Drupal.acquiaLiftProfilesDebug = (function() {
    var debugPrefix = "acquiaLift::debug::"
    var curSegmentCapture = {};
    var curSegments = [];
    var curSegmentsOverride;
    var curIdentities = [];
    var message;
    var code;
    console.log(Storage);
    //tc-widget.js triggers acquiaLiftSegmentsUpdated event when segments are returned. This function will save them
    /**
     *tc-widget.js triggers acquiaLiftSegmentUpdated event when segments are returned. 
     * String Event - the name of the event
     * Object data - the data returned back from tcwidget's ajax call. (tcoffer) 
     * Capture - the capture data. 
     */
    $(document).on("acquiaLiftSegmentsUpdated", function( event, data, capture ) {
      //saves the capture
      curSegmentCapture = capture;
      //if data exists
      if(data){
        curSegments = data["segments"];
        curSegmentsOverride =  Storage.read(debugPrefix + "overrideSegments");
        //if overriding segments
        if ( curSegmentsOverride ) {
          //replaces the segments stored in the data object with the ones specified by user
          if(data["segments"]){
            data["segments"].length = 0;
            $(curSegmentsOverride).each( function(index,overrideSegment) { data["segments"].push(overrideSegment); } );
          }
          //saves copy of override segments to current segments
          curSegments = curSegmentsOverride.splice(0);
        }
        message = "Segments Returned: " + curSegments;
        code = 1000;
      }else{
        message = "No Data found"
        code = 3001;
      }
      //adds the message and code to sessionStorage logs with type 'Lift Web' 
      Drupal.personalizeDebug.log( message , code, 'Lift Web');
    });
    //updates saved identities when triggered.
    $(document).bind("acquiaLiftIdentitiesAdded", function( event, identities ) {
      if(identities){
        $.each(identities, function (index, identity) { curIdentities.push( identity ); } );
      }
    });

    return {
      'getPersonId' : function() {
        return $.cookie("tc_ptid");
      },

      'getTouchId' : function() {
        return $.cookie("tc_ttid");
      },

      'getCurrentSegments' : function() {
        return curSegments;
      },

      'setOverrideSegments' : function( segmentsOverride ) {
        if (segmentsOverride){
          Storage.write(debugPrefix + "originalSegments", curSegments);
          Storage.write(debugPrefix + "overrideSegments", segmentsOverride)
        }else{
          Storage.write(debugPrefix + "overrideSegments");
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
          href = href.replace(/((\?|\&)acquia_lift_inspect_mode=[^]*)/g,"");
          if(href.indexOf('?') > 0){
            href+='&';
          }else{
            href+='?';
          }
          href += 'acquia_lift_inspect_mode=0'
          window.location.href = href;
      }
    };
  })();
})(jQuery, Drupal,  Drupal.personalizeStorage);
