/**
 * Prevents parent element from scrolling when hovering over debugger elements.
 * code inspired by http://stackoverflow.com/questions/5802467/prevent-scrolling-of-parent-element
 */

function preventScrollDefault(e) {
    // converts event for all browsers
    e = e || window.event; 
    if (e.preventDefault) e.preventDefault(); 
    e.returnValue = false;
}
var scrollElement = function(e,elementID){
    var scroll;
    if ( e.wheelDelta ) { // IE and Opera and Chrome
        scroll = e.wheelDelta; //IE
        if(e.wheelDeltaY){
            scroll = e.wheelDeltaY; //Opera and Chrome
        }
    }
    else if (e.detail) { // Mozilla FireFox
        scroll= -e.detail;
    }
    //sets scroll amount if element is a class
    if(document.getElementsByClassName(elementID)[0]){ 
        document.getElementsByClassName(elementID)[0].scrollTop -= scroll;
    //sets scroll amount if element is ID
    }else if (document.getElementById(elementID)){ 
        document.getElementById(elementID).scrollTop -= scroll;
    }
    //prevents parent elements from scrolling.
    preventScrollDefault(e);
    e.stopPropagation();
}

//checks if user is scrolling in the debugger tool
document.getElementById('debugger').onmousewheel = function (e) {
    //scroll only the debugger content if mouse is hovering in the debugger section
    scrollElement(e,'debugger__content'); 
    if(document.getElementById('debugger__autocomplete__dropdown')){ 
        //scroll only the autocomplete dropdown if mouse is hover in that space.
        document.getElementById('debugger__autocomplete__dropdown').onmousewheel = function(e){
            scrollElement(e,'debugger__autocomplete__dropdown')
        } 
    }
}
/**
 * Angular Code for Lift Inspector starts here.
 */
var app = angular.module("debuggerModule", ['isteven-multi-select', 'autocomplete','angularResizable']);
app.value('debugPrefix', 'acquiaLift::debug');
app.constant('previewStatus', "Site Preview")
app.factory('debuggerFactory', function($http){
    var factory = {};

    factory.ajax = function(){
        $http({
            url: "",
            method: "POST",
            data: ""
        }).success(function(data, status){
            console.log(data);
        }).error(function(data, status){
            console.log(data);
        });
    };
    return factory;
})
    /**
     *Setting the UI for the Lift Site Preview Tool depending on the the state.
     *Renders the UI.
     */
    .factory('liftDebugger', function(){
        var Lift = Lift || {};

        (function () {

            'use strict';

            Lift.debugger = function (element, options) {

                this.element =
                    this.options =
                        this.triggerMaximize =
                            this.triggerClose =
                                this.triggerDestroy =
                                    this.isMaximized =
                                        this.isClosed = null;

                this.init(element, options);

            };

            Lift.debugger.DEFAULTS = {};

            Lift.debugger.prototype.init = function (element, options) {

                this.element = element;
                this.options = options;
                this.triggerMaximize = this.getTrigger('maximize');
                this.triggerClose = this.getTrigger('close');
                this.triggerDestroy = this.getTrigger('destroy');
                //checks if debugger is aleady open, will keep open
                if(window.sessionStorage.getItem('acquiaLift::debug::debugIsOpen')==="true"){
                    this.isMaximized = true;
                    this.isClosed = false;
                }else{
                    //if debugger is not already open. keeps the debugger minimized
                    this.isMaximized = false;
                    this.isClosed = true;
                }

                this.render();

            };

            Lift.debugger.prototype.getTrigger = function (type) {

                var trigger = this.element.getElementsByClassName('debugger__action__' + type)[0],
                    functionId = 'click' + type.charAt().toUpperCase() + type.slice(1);
                    
                trigger.addEventListener('click', this[functionId].bind(this));

                return trigger;

            }

            Lift.debugger.prototype.clickMaximize = function (event) {
                this.maximize();

            }

            Lift.debugger.prototype.clickClose = function (event) {
                this.close();

            }

            Lift.debugger.prototype.clickDestroy = function (event) {
                //tear down debugger
                Drupal.acquiaLiftProfilesDebug.turnOffDebugMode();
                Drupal.acquiaLiftProfilesDebug.clearStorage();
                

            }

            Lift.debugger.prototype.setMaximized = function () {
                this.isOpen = false;
                this.isClosed = false;
                this.isMaximized = true;

            };

            Lift.debugger.prototype.setClosed = function () {
                this.isOpen = false;
                this.isMaximized = false;
                this.isClosed = true;

            };
            Lift.debugger.prototype.maximize = function () {
                window.sessionStorage.setItem('acquiaLift::debug::debugIsOpen', true);
                this.setMaximized();
                this.render();
            };

            Lift.debugger.prototype.close = function () {
                window.sessionStorage.setItem('acquiaLift::debug::debugIsOpen', false);
                this.setClosed();
                this.render();
            };

            Lift.debugger.prototype.render = function () {
                //render only if debug mode is on.
                if(Drupal.acquiaLiftProfilesDebug){
                    if (!this.element.classList.contains('debugger-processed')) {
                    this.element.classList.add('debugger');
                    this.element.classList.add('debugger-processed');
                    }

                    if (this.isClosed) {
                        this.element.classList.remove('is-maximized');
                        this.element.classList.add('is-closed');
                        this.triggerMaximize.setAttribute('aria-hidden', 'false');
                        this.triggerDestroy.setAttribute('aria-hidden', 'false');
                        this.triggerClose.setAttribute('aria-hidden', 'true');
                    }

                    if (this.isMaximized) {
                        this.element.classList.add('is-maximized');

                        //checks if a debugger height was set in a previous page. Else sets the height to 80% of max height
                        if(window.sessionStorage.getItem('acquiaLift::debug::debugWindowHeight')){
                            var height = Math.min(window.sessionStorage.getItem('acquiaLift::debug::debugWindowHeight'), document.documentElement.clientHeight * 0.8);
                            document.getElementsByClassName('debugger__content')[0].style.height = height + "px";
                        }
                        this.element.classList.remove('is-closed');
                        this.triggerClose.setAttribute('aria-hidden', 'false');
                        this.triggerDestroy.setAttribute('aria-hidden', 'false');
                        this.triggerMaximize.setAttribute('aria-hidden', 'true');
                    }
                }
            };
        })(Lift);
        return Lift;
    })

    .factory('$sessionStorage', ['$window', function($window) {
        return {
            set: function(key, value) {
                $window.sessionStorage[key] = value;
            },
            get: function(key, defaultValue) {
                return $window.sessionStorage[key] || defaultValue;
            },
            setObject: function(key, value) {
                if(value == undefined){
                    return;
                }
                var cache = [];
                try {
                    $window.sessionStorage[key] = JSON.stringify(value, function (a, b) {
                        if (typeof b === 'object' && b !== null) {
                            if (cache.indexOf(b) !== -1) {
                                return;
                                // Circular reference found, discard key
                            }
                            // Store value in our collection
                            cache.push(b);
                        }
                        return b;
                    });
                } catch (err){
                    console.log(err);
                    window.sessionStorage.clear();
                }
                cache = null;
            },
            getObject: function(key) {
                try{
                    return JSON.parse($window.sessionStorage[key] || '{}');
                }catch (e){
                    return;
                }
            },
            removeItem: function(key){
                $window.sessionStorage.removeItem(key);
            },
            clear: function(){
                window.sessionStorage.clear();
            }
        }
    }])

app.controller("DebuggerController", function($scope, $timeout, debuggerFactory, $sessionStorage, $window, liftDebugger, debugPrefix, $document, previewStatus){
    //check for existence of acqiuaLiftProfilesDebug, if no do not proceed.
    if(!Drupal.acquiaLiftProfilesDebug){
        return;
    } 
    if(!Drupal.personalizeStorage.supportsLocalStorage()){
        var deb = document.getElementById("debugger");
        if(deb){
            deb.parentNode.removeChild(deb);
        }
        return;
    }
    //starting variables
    $scope.items = []; //event log
    $scope.tab = 'log'; //starting tab
    $scope.profile = {}; //no segments in profile

    //loads keys from sessionStorage into event log.
    for (var key in $window.sessionStorage){
        if(key.indexOf(debugPrefix) >= 0){
            $timeout(function(index){
                return function(){
                    $scope.items.push($sessionStorage.getObject(index));
                }
            }(key));
        }
    }
    //renders the debugger
    var debugConsole = new liftDebugger.debugger(document.getElementById('debugger'));

    //registers for any new debug events.
    $document.on('acquiaLiftDebugEvent', function(event, key){
        //makes sure that the debug event has the corect debug prefix
        if(key.indexOf(debugPrefix) >= 0){
            $timeout(function(index){
                return function(){
                    //make sure that debug mode is enabled
                    if(Drupal.acquiaLiftProfilesDebug){
                        //push the new log into the event log tab
                        $scope.items.push($sessionStorage.getObject(index));
                        if($sessionStorage.getObject(index).message.indexOf("Segments Returned") >= 0){
                            //if the returned log is regarding segments, update our current saved segments.
                            $scope.profile.curSegments = Drupal.acquiaLiftProfilesDebug.getCurrentSegments().slice();
                            //if override segments does not exist, set possible override segments to current segments. Update autocomplete list.
                            if(!$scope.profile.overrideSegments){
                                $scope.profile.overrideSegments = Drupal.acquiaLiftProfilesDebug.getCurrentSegments().slice();
                                $scope.allSegments =  $scope.profile.allSegments.filter(function(i) {return $scope.profile.overrideSegments .indexOf(i) < 0;});
                            };
                        }
                    }
                    return;
                }
            }(key));
        }
    });
    //set the tab to previously selected
    if(window.sessionStorage.getItem("acquiaLift::debug::state")){
        $scope.tab = window.sessionStorage.getItem("acquiaLift::debug::state");
    }

    //copy the values from our profile
    $scope.profile.personId = Drupal.acquiaLiftProfilesDebug.getPersonId();
    $scope.profile.touchId = Drupal.acquiaLiftProfilesDebug.getTouchId();
    $scope.profile.allSegments = Drupal.acquiaLiftProfilesDebug.getAllSegments().slice();
    $scope.profile.curSegments = Drupal.acquiaLiftProfilesDebug.getCurrentSegments().slice();
    $scope.profile.identities = Drupal.acquiaLiftProfilesDebug.getAdditionalIdentities().slice();
    $scope.profile.overrideSegments = Drupal.acquiaLiftProfilesDebug.getOverrideSegments();
 
    //ensures that allSegments doesnt have duplicates from override segments. Fills out the segments for the dropDown list in the site preview tab
    if($scope.profile.overrideSegments){
        $scope.allSegments =  $scope.profile.allSegments.filter(function(i) {return $scope.profile.overrideSegments.indexOf(i) < 0;});
    }else{
        $scope.allSegments = $scope.profile.allSegments;
    };

    //additional filters and functions
    $scope.severityFilter = [
        {  name: "Error",ticked: true },
        { name: "Warning", ticked: true  },
        {  name:"Info",ticked: true },
    ];
    $scope.typeFilter = [

        { name: "Drupal",ticked: true },
        { name: "Lift Web", ticked: true  },
        { name: "Developer", ticked: false  },
    ];  

    /**
     * Determines if an event log row should be displayed depending on filters
     *
     * @return boolean
     */
    $scope.search = function (row) {
        var returnValue = false;
        if(row) {
            if (row.severity !== undefined && row.type !== undefined) {
                for (var i = 0; i < $scope.severity.length; i++) {
                    if (typeof($scope.severity[i].name) === "string") {
                        if ($scope.severity[i].name.toUpperCase() === row.severity.toUpperCase()) {
                            returnValue = true;
                        }
                    }
                }
                if(returnValue){
                    returnValue = false;
                    for (var i = 0; i < $scope.type.length; i++) {
                        if (typeof($scope.type[i].name) === "string") {
                            if ($scope.type[i].name.toUpperCase() === row.type.toUpperCase()) {
                                returnValue =  true;
                            }
                        }
                    }       
                }
            }
        }
        return returnValue;
    };

    /**
     * Adds an item to the list of segments to preview. 
     * Removes that item from the list in autocomplete box.
     */
    $scope.addPreviewItem = function (item) {
        var value;
        if (item==null){
            value = $scope.overrideSegmentChoice;
        }else{
            value = item;
        }
        var indexAll = $scope.allSegments.indexOf(value);
        if(indexAll > -1){
            $scope.allSegments.splice(indexAll, 1);
            $scope.profile.overrideSegments.push(value);
        }
        $scope.overrideSegmentChoice = undefined;
    }

    /**
     * Removes an item to the list of segments to preview. 
     * Adds that item from the list in autocomplete box.
     */
    $scope.deletePreviewItem = function(item){
        var index = $scope.profile.overrideSegments.indexOf(item);
        if(index > -1){
            $scope.profile.overrideSegments.splice(index, 1);
        }
        $scope.allSegments.push(item);
    }

    /**
     * Sets override segments for preview
     * Tells user to refresh page page to see changes
     */
    $scope.startPreview = function(){
        Drupal.acquiaLiftProfilesDebug.setOverrideSegments($scope.profile.overrideSegments);    
        document.getElementsByClassName('debugger__preview__notification')[0].innerHTML = 'Please refresh page to see your changes. You may have to clear your cache as well.';
    }

    /**
     * Changes override segments back to previous versions
     * clears saved override segments.
     */
    $scope.stopPreview = function(){
        $scope.profile.overrideSegments = $sessionStorage.getObject(debugPrefix + "::overrideSegments");
        $scope.profile.curSegments = $sessionStorage.getObject(debugPrefix + "::originalSegments");
        if ($scope.profile.overrideSegments){
            Drupal.acquiaLiftProfilesDebug.setOverrideSegments(null);
            $sessionStorage.removeItem(debugPrefix + "::overrideSegments");
        }
    }

    /**
     * Sets button CSS depending on preview status.
     * Sets Site Preview Tab label.
     * Sets messages in the Lift Profile tab.
     */
    $scope.isPreview = function(){
        if (window.sessionStorage.getItem("acquiaLift::debug::overrideSegments")){
            $scope.previewActive = "preview__button__inactive";
            $scope.previewButtonStop = "preview__button__active";
            $scope.previewButtonStart = "preview__button__inactive";
            document.getElementById("sitePreviewTabLabel").innerHTML='Site Preview - Active';
            document.getElementById("sitePreviewSegmentsLabel").innerHTML='<b>Previewing site as these segments:</b>';
            document.getElementById("profileSegmentLabel").innerHTML='<b>Previewing site as these segments:</b>'

            return true;
        }else{
            $scope.previewActive = null;
            $scope.previewButtonStart = "preview__button__active";
            $scope.previewButtonStop="preview__button__inactive";
            document.getElementById("sitePreviewTabLabel").innerHTML='Site Preview';
            document.getElementById("sitePreviewSegmentsLabel").innerHTML='<b>Define your list of segments before starting to preview:</b>';
            document.getElementById("profileSegmentLabel").innerHTML='<b>Last Evaluated Segments:</b>'

            return false;
        }
    }
    /**
     * Navigate between tabs
     * Sets CSS of the active tab.
     * Remove CSS of previous active tab.
     */
    $scope.buttonClick = function(){
        $scope.profileButton={};
        $scope.logButton={};
        $scope.previewButton={};
        switch($scope.tab){
            case 'log': $scope.logButton="debugger__navigation__item__active";
            break;
            case 'profile': $scope.profileButton="debugger__navigation__item__active";
            break;
            case 'preview': $scope.previewButton="debugger__navigation__item__active";
            break;
        }
        window.sessionStorage.setItem('acquiaLift::debug::state', $scope.tab);
    }

    //first click
    $scope.buttonClick();

});


/**
 * CUSTOM ANGULAR FILTER: replaces severity with icons
 */
angular.module('debuggerModule')
    .filter('to_icon', ['$sce', function($sce){
        return function(text) {
            var icon;
            if(text){
                switch(text.toUpperCase()){
                    case "WARNING": icon="<div class='debugger-icon-warning'>&nbsp;</div>";
                        break;
                    case "ERROR" : icon="<div class='debugger-icon-error'>&nbsp;</div>";
                        break;
                    case "INFO" : icon="<div class='debugger-icon-info'>&nbsp;</div>";
                        break;
                }
                return $sce.trustAsHtml(icon);   
            }
            return;
        };
    }]);

/**
 * CUSTOM ANGULAR FILTER: truncates event logs to 100 characters or less. adds '...' for truncated strings
 */
angular.module('debuggerModule')
    .filter('cut', function () {
        return function (value, wordwise, max, tail, item) {
            if (!value) return '';
            max = parseInt(max, 10);
            if (!max) return value;
            if (value.length <= max) return value;
            if(item !== undefined){
                if (item.expand){
                    return value;
                }else{
                    value = value.substr(0, max);
                    if (wordwise) {
                        var lastspace = value.lastIndexOf(' ');
                        if (lastspace != -1) {
                            value = value.substr(0, lastspace);
                        }
                    }
                    return value + (tail || ' …');
                }
            }
            return value;
        };
    });
