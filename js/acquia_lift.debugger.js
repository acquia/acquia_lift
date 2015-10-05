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


/* --- Made by justgoscha and licensed under MIT license --- */

var app = angular.module('autocomplete', []);

app.directive('autocomplete', function() {
    var index = -1;

    return {
        restrict: 'E',
        scope: {
            searchParam: '=ngModel',
            suggestions: '=data',
            onType: '=onType',
            onSelect: '=onSelect',
            autocompleteRequired: '='
        },
        controller: ['$scope', function($scope){
            // the index of the suggestions that's currently selected
            $scope.selectedIndex = -1;

            $scope.initLock = true;

            // set new index
            $scope.setIndex = function(i){
                $scope.selectedIndex = parseInt(i);
            };

            this.setIndex = function(i){
                $scope.setIndex(i);
                $scope.$apply();
            };

            $scope.getIndex = function(i){
                return $scope.selectedIndex;
            };

            // watches if the parameter filter should be changed
            var watching = false;

            // autocompleting drop down on/off
            $scope.completing = false;
            // starts autocompleting on typing in something
            $scope.$watch('searchParam', function(newValue, oldValue){
                if (oldValue === newValue || newValue === undefined) {
                    return;
                }

                if(watching && typeof $scope.searchParam !== 'undefined' && $scope.searchParam !== null) {
                    $scope.completing = true;
                    $scope.searchFilter = $scope.searchParam;
                    $scope.selectedIndex = -1;
                }
                // function thats passed to on-type attribute gets executed
                if($scope.onType)
                    $scope.onType($scope.searchParam);
            });

            $scope.showAll = function(){
                $scope.completing = true;
                watching = true;
                if(typeof $scope.searchParam!=='undefined'){
                    $scope.searchFilter = $scope.searchParam;
                }else{
                    $scope.searchFilter = '';
                }
            }

            // for hovering over suggestions
            this.preSelect = function(suggestion){

                watching = false;

                // this line determines if it is shown

                // in the input field before it's selected:
                //$scope.searchParam = suggestion;

                $scope.$apply();
                watching = true;

            };

            $scope.preSelect = this.preSelect;

            this.preSelectOff = function(){
                watching = true;
            };

            $scope.preSelectOff = this.preSelectOff;

            // selecting a suggestion with RIGHT ARROW or ENTER
            $scope.select = function(suggestion){
                if(suggestion){
                    $scope.searchParam = suggestion;
                    $scope.searchFilter = suggestion;
                    if($scope.onSelect)
                        $scope.onSelect(suggestion);

                }
                watching = false;
                $scope.completing = false;
                setTimeout(function(){watching = true;},1000);
                $scope.setIndex(-1);
            };


        }],
        link: function(scope, element, attrs){

            setTimeout(function() {
                scope.initLock = false;
                scope.$apply();
            }, 250);

            var attr = '';

            // Default atts
            scope.attrs = {
                "placeholder": "Enter a segment...",
                "class": "",
                "id": "autocomplete_input",
                "inputclass": "",
                "inputid": ""
            };

            for (var a in attrs) {
                attr = a.replace('attr', '').toLowerCase();
                // add attribute overriding defaults
                // and preventing duplication
                if (a.indexOf('attr') === 0) {
                    scope.attrs[attr] = attrs[a];
                }
            }

            if (attrs.clickActivation) {
                element[0].onclick = function(e){
                    if(!scope.searchParam){
                        setTimeout(function() {
                            scope.completing = true;
                            scope.$apply();
                        }, 200);
                    }
                };
            }

            var key = {left: 37, up: 38, right: 39, down: 40 , enter: 13, esc: 27, tab: 9};

            document.addEventListener("keydown", function(e){
                var keycode = e.keyCode || e.which;
                switch (keycode){
                    case key.esc:
                        // disable suggestions on escape
                        scope.select();
                        scope.setIndex(-1);
                        scope.$apply();
                        // e.preventDefault();
                }
            }, true);

            document.addEventListener("focus", function(e){
                if(scope.$parent.tab !== "preview" || e.target.tagName !== "INPUT"){
                    return;
                }
                var dropdown = document.getElementById('debugger__autocomplete__dropdown');
                var addbox = document.getElementsByClassName('addBox')[0].offsetTop;
                var debuggerHeight = window.sessionStorage.getItem('acquiaLift::debug::debugWindowHeight')
                var position = Math.min(debuggerHeight? debuggerHeight-addbox: 100000, window.innerHeight*0.80-addbox);
                position -= 50;
                position = (position < 0)? 15: position;
                scope.showAll();
                scope.$apply();
                dropdown.style.maxHeight = position.toString() + "px";
            }, true);
            document.addEventListener("blur", function(e){
                if(scope.$parent.tab !== "preview" || e.target.tagName !== "INPUT"){
                    return;
                }
                // disable suggestions on blur
                // we do a timeout to prevent hiding it before a click event is registered
                setTimeout(function() {
                    scope.select();
                    scope.setIndex(-1);
                    scope.$apply();
                }, 150);
            }, true);

            element[0].addEventListener("keydown",function (e){
                var keycode = e.keyCode || e.which;

                var l = angular.element(this).find('li').length;

                // this allows submitting forms by pressing Enter in the autocompleted field
                if(!scope.completing || l == 0) return;

                // implementation of the up and down movement in the list of suggestions
                switch (keycode){
                    case key.up:
                        index = scope.getIndex()-1;
                        if(index<-1){
                            index = l-1;
                        } else if (index >= l ){
                            index = -1;
                            scope.setIndex(index);
                            scope.preSelectOff();
                            break;
                        }
                        scope.setIndex(index);

                        if(index!==-1)
                            scope.preSelect(angular.element(angular.element(this).find('li')[index]).text());

                        scope.$apply();

                        break;
                    case key.down:
                        index = scope.getIndex()+1;
                        if(index<-1){
                            index = l-1;
                        } else if (index >= l ){
                            index = -1;
                            scope.setIndex(index);
                            scope.preSelectOff();
                            scope.$apply();
                            break;
                        }
                        scope.setIndex(index);

                        if(index!==-1)
                            scope.preSelect(angular.element(angular.element(this).find('li')[index]).text());

                        break;
                    case key.left:
                        break;
                    case key.right:
                    case key.enter:
                    case key.tab:

                        index = scope.getIndex();
                        // scope.preSelectOff();
                        if(index !== -1) {
                            scope.select(angular.element(angular.element(this).find('li')[index]).text());
                            if(keycode == key.enter) {
                                e.preventDefault();
                            }
                        } else {
                            if(keycode == key.enter) {
                                scope.select();
                            }
                        }
                        scope.setIndex(-1);
                        scope.$apply();

                        break;
                    case key.esc:
                        // disable suggestions on escape
                        scope.select();
                        scope.setIndex(-1);
                        scope.$apply();
                        e.preventDefault();
                        break;
                    default:
                        return;
                }

            });
        },
        template: '\
        <div class="autocomplete {{ attrs.class }}" id="{{ attrs.id }}">\
          <input\
            type="text"\
            ng-model="searchParam"\
            placeholder="{{ attrs.placeholder }}"\
            class="{{ attrs.inputclass }}"\
            id="{{ attrs.inputid }}"\
            ng-required="{{ autocompleteRequired }}" />\
          <ul id="debugger__autocomplete__dropdown" ng-show="completing && (suggestions | filter:searchFilter).length > 0">\
            <li\
              suggestion\
              ng-repeat="suggestion in suggestions | filter:searchFilter | orderBy:\'toString()\' track by $index"\
              index="{{ $index }}"\
              val="{{ suggestion }}"\
              ng-class="{ active: ($index === selectedIndex) }"\
              ng-click="select(suggestion)"\
              ng-bind-html="suggestion | highlight:searchParam"></li>\
          </ul>\
        </div>'
    };
});

app.filter('highlight', ['$sce', function ($sce) {
    return function (input, searchParam) {
        if (typeof input === 'function') return '';
        if (searchParam) {
            var words = '(' +
                    searchParam.split(/\ /).join(' |') + '|' +
                    searchParam.split(/\ /).join('|') +
                    ')',
                exp = new RegExp(words, 'gi');
            if (words.length) {
                input = input.replace(exp, "<span class=\"highlight\">$1</span>");
            }
        }
        return $sce.trustAsHtml(input);
    };
}]);

app.directive('suggestion', function(){
    return {
        restrict: 'A',
        require: '^autocomplete', // ^look for controller on parents element
        link: function(scope, element, attrs, autoCtrl){
            element.bind('mouseenter', function() {
                autoCtrl.preSelect(attrs.val);
                autoCtrl.setIndex(attrs.index);
            });

            element.bind('mouseleave', function() {
                autoCtrl.preSelectOff();
            });
        }
    };
});


/*
 * Angular JS Multi Select
 * Creates a dropdown-like button with checkboxes.
 *
 * Project started on: Tue, 14 Jan 2014 - 5:18:02 PM
 * Current version: 4.0.0
 *
 * Released under the MIT License
 * License: https://github.com/isteven/angular-multi-select/blob/master/LICENSE.txt
 */

'use strict'

angular.module( 'isteven-multi-select', ['ng'] ).directive( 'istevenMultiSelect' , [ '$sce', '$timeout', '$templateCache', function ( $sce, $timeout, $templateCache ) {
    return {
        restrict:
            'AE',

        scope:
        {
            // models
            inputModel      : '=',
            outputModel     : '=',

            // settings based on attribute
            isDisabled      : '=',

            // callbacks
            onClear         : '&',
            onClose         : '&',
            onSearchChange  : '&',
            onItemClick     : '&',
            onOpen          : '&',
            onReset         : '&',

            // i18n
            translation     : '='
        },

        /*
         * The rest are attributes. They don't need to be parsed / binded, so we can safely access them by value.
         * - buttonLabel, directiveId, helperElements, itemLabel, maxLabels, orientation, selectionMode, minSearchLength,
         *   tickProperty, disableProperty, groupProperty, searchProperty, maxHeight, outputProperties
         */

        templateUrl:
            'isteven-multi-select.htm',

        link: function ( $scope, element, attrs ) {

            $scope.backUp           = [];
            $scope.varButtonLabel   = '';
            $scope.spacingProperty  = '';
            $scope.indexProperty    = '';
            $scope.orientationH     = false;
            $scope.orientationV     = true;
            $scope.filteredModel    = [];
            $scope.inputLabel       = { labelFilter: '' };
            $scope.tabIndex         = 0;
            $scope.lang             = {};
            $scope.helperStatus     = {
                filter  : true
            };

            var
                prevTabIndex        = 0,
                helperItems         = [],
                helperItemsLength   = 0,
                checkBoxLayer       = '',
                scrolled            = false,
                selectedItems       = [],
                formElements        = [],
                vMinSearchLength    = 0,
                clickedItem         = null

            // v3.0.0
            // clear button clicked
            $scope.clearClicked = function( e ) {
                $scope.inputLabel.labelFilter = '';
                $scope.updateFilter();
                $scope.select( 'clear', e );
            }

            // A little hack so that AngularJS ng-repeat can loop using start and end index like a normal loop
            // http://stackoverflow.com/questions/16824853/way-to-ng-repeat-defined-number-of-times-instead-of-repeating-over-array
            $scope.numberToArray = function( num ) {
                return new Array( num );
            }

            // Call this function when user type on the filter field
            $scope.searchChanged = function() {
                if ( $scope.inputLabel.labelFilter.length < vMinSearchLength && $scope.inputLabel.labelFilter.length > 0 ) {
                    return false;
                }
                $scope.updateFilter();
            }

            $scope.updateFilter = function()
            {
                // we check by looping from end of input-model
                $scope.filteredModel = [];
                var i = 0;

                if ( typeof $scope.inputModel === 'undefined' ) {
                    return false;
                }

                for( i = $scope.inputModel.length - 1; i >= 0; i-- ) {

                    // if it's group end, we push it to filteredModel[];
                    if ( typeof $scope.inputModel[ i ][ attrs.groupProperty ] !== 'undefined' && $scope.inputModel[ i ][ attrs.groupProperty ] === false ) {
                        $scope.filteredModel.push( $scope.inputModel[ i ] );
                    }

                    // if it's data
                    var gotData = false;
                    if ( typeof $scope.inputModel[ i ][ attrs.groupProperty ] === 'undefined' ) {

                        // If we set the search-key attribute, we use this loop.
                        if ( typeof attrs.searchProperty !== 'undefined' && attrs.searchProperty !== '' ) {

                            for (var key in $scope.inputModel[ i ]  ) {
                                if (
                                    typeof $scope.inputModel[ i ][ key ] !== 'boolean'
                                    && String( $scope.inputModel[ i ][ key ] ).toUpperCase().indexOf( $scope.inputLabel.labelFilter.toUpperCase() ) >= 0
                                    && attrs.searchProperty.indexOf( key ) > -1
                                ) {
                                    gotData = true;
                                    break;
                                }
                            }
                        }
                        // if there's no search-key attribute, we use this one. Much better on performance.
                        else {
                            for ( var key in $scope.inputModel[ i ]  ) {
                                if (
                                    typeof $scope.inputModel[ i ][ key ] !== 'boolean'
                                    && String( $scope.inputModel[ i ][ key ] ).toUpperCase().indexOf( $scope.inputLabel.labelFilter.toUpperCase() ) >= 0
                                ) {
                                    gotData = true;
                                    break;
                                }
                            }
                        }

                        if ( gotData === true ) {
                            // push
                            $scope.filteredModel.push( $scope.inputModel[ i ] );
                        }
                    }
                }
                $scope.filteredModel.reverse();
                $timeout( function() {
                    $scope.getFormElements();

                    // Callback: on filter change
                    if ( $scope.inputLabel.labelFilter.length > vMinSearchLength ) {

                        var filterObj = [];

                        angular.forEach( $scope.filteredModel, function( value, key ) {
                            if ( typeof value !== 'undefined' ) {
                                if ( typeof value[ attrs.groupProperty ] === 'undefined' ) {
                                    var tempObj = angular.copy( value );
                                    var index = filterObj.push( tempObj );
                                    delete filterObj[ index - 1 ][ $scope.indexProperty ];
                                    delete filterObj[ index - 1 ][ $scope.spacingProperty ];
                                }
                            }
                        });

                        $scope.onSearchChange({
                            data:
                            {
                                keyword: $scope.inputLabel.labelFilter,
                                result: filterObj
                            }
                        });
                    }
                },0);
            };

            // List all the input elements. We need this for our keyboard navigation.
            // This function will be called everytime the filter is updated.
            // Depending on the size of filtered mode, might not good for performance, but oh well..
            $scope.getFormElements = function() {
                formElements = [];

                var
                    selectButtons   = [],
                    inputField      = [],
                    checkboxes      = [],
                    clearButton     = [];

                // If available, then get select all, select none, and reset buttons
                if ( $scope.helperStatus.all || $scope.helperStatus.none || $scope.helperStatus.reset ) {
                    selectButtons = element.children().children().next().children().children()[ 0 ].getElementsByTagName( 'button' );
                    // If available, then get the search box and the clear button
                    if ( $scope.helperStatus.filter ) {
                        // Get helper - search and clear button.
                        inputField =    element.children().children().next().children().children().next()[ 0 ].getElementsByTagName( 'input' );
                        clearButton =   element.children().children().next().children().children().next()[ 0 ].getElementsByTagName( 'button' );
                    }
                }
                else {
                    if ( $scope.helperStatus.filter ) {
                        // Get helper - search and clear button.
                        inputField =    element.children().children().next().children().children()[ 0 ].getElementsByTagName( 'input' );
                        clearButton =   element.children().children().next().children().children()[ 0 ].getElementsByTagName( 'button' );
                    }
                }

                // Get checkboxes
                if ( !$scope.helperStatus.all && !$scope.helperStatus.none && !$scope.helperStatus.reset && !$scope.helperStatus.filter ) {
                    checkboxes = element.children().children().next()[ 0 ].getElementsByTagName( 'input' );
                }
                else {
                    checkboxes = element.children().children().next().children().next()[ 0 ].getElementsByTagName( 'input' );
                }

                // Push them into global array formElements[]
                for ( var i = 0; i < selectButtons.length ; i++ )   { formElements.push( selectButtons[ i ] );  }
                for ( var i = 0; i < inputField.length ; i++ )      { formElements.push( inputField[ i ] );     }
                for ( var i = 0; i < clearButton.length ; i++ )     { formElements.push( clearButton[ i ] );    }
                for ( var i = 0; i < checkboxes.length ; i++ )      { formElements.push( checkboxes[ i ] );     }
            }

            // check if an item has attrs.groupProperty (be it true or false)
            $scope.isGroupMarker = function( item , type ) {
                if ( typeof item[ attrs.groupProperty ] !== 'undefined' && item[ attrs.groupProperty ] === type ) return true;
                return false;
            }

            $scope.removeGroupEndMarker = function( item ) {
                if ( typeof item[ attrs.groupProperty ] !== 'undefined' && item[ attrs.groupProperty ] === false ) return false;
                return true;
            }

            // call this function when an item is clicked
            $scope.syncItems = function( item, e, ng_repeat_index ) {

                e.preventDefault();
                e.stopPropagation();

                // if the directive is globaly disabled, do nothing
                if ( typeof attrs.disableProperty !== 'undefined' && item[ attrs.disableProperty ] === true ) {
                    return false;
                }

                // if item is disabled, do nothing
                if ( typeof attrs.isDisabled !== 'undefined' && $scope.isDisabled === true ) {
                    return false;
                }

                // if end group marker is clicked, do nothing
                if ( typeof item[ attrs.groupProperty ] !== 'undefined' && item[ attrs.groupProperty ] === false ) {
                    return false;
                }

                var index = $scope.filteredModel.indexOf( item );

                // if the start of group marker is clicked ( only for multiple selection! )
                // how it works:
                // - if, in a group, there are items which are not selected, then they all will be selected
                // - if, in a group, all items are selected, then they all will be de-selected
                if ( typeof item[ attrs.groupProperty ] !== 'undefined' && item[ attrs.groupProperty ] === true ) {

                    // this is only for multiple selection, so if selection mode is single, do nothing
                    if ( typeof attrs.selectionMode !== 'undefined' && attrs.selectionMode.toUpperCase() === 'SINGLE' ) {
                        return false;
                    }

                    var i,j,k;
                    var startIndex = 0;
                    var endIndex = $scope.filteredModel.length - 1;
                    var tempArr = [];

                    // nest level is to mark the depth of the group.
                    // when you get into a group (start group marker), nestLevel++
                    // when you exit a group (end group marker), nextLevel--
                    var nestLevel = 0;

                    // we loop throughout the filtered model (not whole model)
                    for( i = index ; i < $scope.filteredModel.length ; i++) {

                        // this break will be executed when we're done processing each group
                        if ( nestLevel === 0 && i > index )
                        {
                            break;
                        }

                        if ( typeof $scope.filteredModel[ i ][ attrs.groupProperty ] !== 'undefined' && $scope.filteredModel[ i ][ attrs.groupProperty ] === true ) {

                            // To cater multi level grouping
                            if ( tempArr.length === 0 ) {
                                startIndex = i + 1;
                            }
                            nestLevel = nestLevel + 1;
                        }

                        // if group end
                        else if ( typeof $scope.filteredModel[ i ][ attrs.groupProperty ] !== 'undefined' && $scope.filteredModel[ i ][ attrs.groupProperty ] === false ) {
                            nestLevel = nestLevel - 1;
                        }

                        // if data
                        else {
                            tempArr.push( $scope.filteredModel[ i ] );
                        }
                    }
                }

                // if an item (not group marker) is clicked
                else {

                    // If it's single selection mode
                    if ( typeof attrs.selectionMode !== 'undefined' && attrs.selectionMode.toUpperCase() === 'SINGLE' ) {

                        // first, set everything to false
                        for( i=0 ; i < $scope.filteredModel.length ; i++) {
                            $scope.filteredModel[ i ][ $scope.tickProperty ] = false;
                        }
                        for( i=0 ; i < $scope.inputModel.length ; i++) {
                            $scope.inputModel[ i ][ $scope.tickProperty ] = false;
                        }

                        // then set the clicked item to true
                        $scope.filteredModel[ index ][ $scope.tickProperty ] = true;
                    }

                    // Multiple
                    else {
                        $scope.filteredModel[ index ][ $scope.tickProperty ]   = !$scope.filteredModel[ index ][ $scope.tickProperty ];
                    }

                    // we refresh input model as well
                    var inputModelIndex = $scope.filteredModel[ index ][ $scope.indexProperty ];
                    $scope.inputModel[ inputModelIndex ][ $scope.tickProperty ] = $scope.filteredModel[ index ][ $scope.tickProperty ];
                }

                // we execute the callback function here
                clickedItem = angular.copy( item );
                if ( clickedItem !== null ) {
                    $timeout( function() {
                        delete clickedItem[ $scope.indexProperty ];
                        delete clickedItem[ $scope.spacingProperty ];
                        $scope.onItemClick( { data: clickedItem } );
                        clickedItem = null;
                    }, 0 );
                }

                $scope.refreshOutputModel();
                $scope.refreshButton();

                // We update the index here
                prevTabIndex = $scope.tabIndex;
                $scope.tabIndex = ng_repeat_index + helperItemsLength;

                // Set focus on the hidden checkbox
                e.target.focus();

                // set & remove CSS style
                $scope.removeFocusStyle( prevTabIndex );
                $scope.setFocusStyle( $scope.tabIndex );

                if ( typeof attrs.selectionMode !== 'undefined' && attrs.selectionMode.toUpperCase() === 'SINGLE' ) {
                    // on single selection mode, we then hide the checkbox layer
                    $scope.toggleCheckboxes( e );
                }
            }

            // update $scope.outputModel
            $scope.refreshOutputModel = function() {

                $scope.outputModel  = [];
                var
                    outputProps     = [],
                    tempObj         = {};

                // v4.0.0
                if ( typeof attrs.outputProperties !== 'undefined' ) {
                    outputProps = attrs.outputProperties.split(' ');
                    angular.forEach( $scope.inputModel, function( value, key ) {
                        if (
                            typeof value !== 'undefined'
                            && typeof value[ attrs.groupProperty ] === 'undefined'
                            && value[ $scope.tickProperty ] === true
                        ) {
                            tempObj         = {};
                            angular.forEach( value, function( value1, key1 ) {
                                if ( outputProps.indexOf( key1 ) > -1 ) {
                                    tempObj[ key1 ] = value1;
                                }
                            });
                            var index = $scope.outputModel.push( tempObj );
                            delete $scope.outputModel[ index - 1 ][ $scope.indexProperty ];
                            delete $scope.outputModel[ index - 1 ][ $scope.spacingProperty ];
                        }
                    });
                }
                else {
                    angular.forEach( $scope.inputModel, function( value, key ) {
                        if (
                            typeof value !== 'undefined'
                            && typeof value[ attrs.groupProperty ] === 'undefined'
                            && value[ $scope.tickProperty ] === true
                        ) {
                            var temp = angular.copy( value );
                            var index = $scope.outputModel.push( temp );
                            delete $scope.outputModel[ index - 1 ][ $scope.indexProperty ];
                            delete $scope.outputModel[ index - 1 ][ $scope.spacingProperty ];
                        }
                    });
                }
            }

            // refresh button label
            $scope.refreshButton = function() {
                $scope.varButtonLabel   = attrs.outputModelName;
                $scope.varButtonLabel = $sce.trustAsHtml( $scope.varButtonLabel );
            }

            // Check if a checkbox is disabled or enabled. It will check the granular control (disableProperty) and global control (isDisabled)
            // Take note that the granular control has higher priority.
            $scope.itemIsDisabled = function( item ) {

                if ( typeof attrs.disableProperty !== 'undefined' && item[ attrs.disableProperty ] === true ) {
                    return true;
                }
                else {
                    if ( $scope.isDisabled === true ) {
                        return true;
                    }
                    else {
                        return false;
                    }
                }

            }

            // A simple function to parse the item label settings. Used on the buttons and checkbox labels.
            $scope.writeLabel = function( item, type ) {

                // type is either 'itemLabel' or 'buttonLabel'
                var temp    = attrs[ type ].split( ' ' );
                var label   = '';

                angular.forEach( temp, function( value, key ) {
                    item[ value ] && ( label += '&nbsp;' + value.split( '.' ).reduce( function( prev, current ) {
                            return prev[ current ];
                        }, item ));
                });

                if ( type.toUpperCase() === 'BUTTONLABEL' ) {
                    return label;
                }
                return $sce.trustAsHtml( label );
            }

            // UI operations to show/hide checkboxes based on click event..
            $scope.toggleCheckboxes = function( e ) {

                // We grab the button
                var clickedEl = element.children()[0];

                // Just to make sure.. had a bug where key events were recorded twice
                angular.element( document ).off( 'click', $scope.externalClickListener );
                angular.element( document ).off( 'keydown', $scope.keyboardListener );

                // The idea below was taken from another multi-select directive - https://github.com/amitava82/angular-multiselect
                // His version is awesome if you need a more simple multi-select approach.

                // close
                if ( angular.element( checkBoxLayer ).hasClass( 'show' )) {

                    angular.element( checkBoxLayer ).removeClass( 'show' );
                    angular.element( clickedEl ).removeClass( 'buttonClicked' );
                    angular.element( document ).off( 'click', $scope.externalClickListener );
                    angular.element( document ).off( 'keydown', $scope.keyboardListener );

                    // clear the focused element;
                    $scope.removeFocusStyle( $scope.tabIndex );
                    if ( typeof formElements[ $scope.tabIndex ] !== 'undefined' ) {
                        formElements[ $scope.tabIndex ].blur();
                    }

                    // close callback
                    $timeout( function() {
                        $scope.onClose();
                    }, 0 );

                    // set focus on button again
                    element.children().children()[ 0 ].focus();
                }
                // open
                else
                {
                    // clear filter
                    $scope.inputLabel.labelFilter = '';
                    $scope.updateFilter();

                    helperItems = [];
                    helperItemsLength = 0;

                    angular.element( checkBoxLayer ).addClass( 'show' );
                    angular.element( clickedEl ).addClass( 'buttonClicked' );

                    // Attach change event listener on the input filter.
                    // We need this because ng-change is apparently not an event listener.
                    angular.element( document ).on( 'click', $scope.externalClickListener );
                    angular.element( document ).on( 'keydown', $scope.keyboardListener );

                    // to get the initial tab index, depending on how many helper elements we have.
                    // priority is to always focus it on the input filter
                    $scope.getFormElements();
                    $scope.tabIndex = 0;

                    var helperContainer = angular.element( element[ 0 ].querySelector( '.helperContainer' ) )[0];

                    if ( typeof helperContainer !== 'undefined' ) {
                        for ( var i = 0; i < helperContainer.getElementsByTagName( 'BUTTON' ).length ; i++ ) {
                            helperItems[ i ] = helperContainer.getElementsByTagName( 'BUTTON' )[ i ];
                        }
                        helperItemsLength = helperItems.length + helperContainer.getElementsByTagName( 'INPUT' ).length;
                    }

                    // focus on the filter element on open.
                    if ( element[ 0 ].querySelector( '.inputFilter' ) ) {
                        element[ 0 ].querySelector( '.inputFilter' ).focus();
                        $scope.tabIndex = $scope.tabIndex + helperItemsLength - 2;
                        // blur button in vain
                        angular.element( element ).children()[ 0 ].blur();
                    }
                    // if there's no filter then just focus on the first checkbox item
                    else {
                        if ( !$scope.isDisabled ) {
                            $scope.tabIndex = $scope.tabIndex + helperItemsLength;
                            if ( $scope.inputModel.length > 0 ) {
                                formElements[ $scope.tabIndex ].focus();
                                $scope.setFocusStyle( $scope.tabIndex );
                                // blur button in vain
                                angular.element( element ).children()[ 0 ].blur();
                            }
                        }
                    }

                    // open callback
                    $scope.onOpen();
                }
            }

            // handle clicks outside the button / multi select layer
            $scope.externalClickListener = function( e ) {

                var targetsArr = element.find( e.target.tagName );
                for (var i = 0; i < targetsArr.length; i++) {
                    if ( e.target == targetsArr[i] ) {
                        return;
                    }
                }

                angular.element( checkBoxLayer.previousSibling ).removeClass( 'buttonClicked' );
                angular.element( checkBoxLayer ).removeClass( 'show' );
                angular.element( document ).off( 'click', $scope.externalClickListener );
                angular.element( document ).off( 'keydown', $scope.keyboardListener );

                // close callback
                $timeout( function() {
                    $scope.onClose();
                }, 0 );

                // set focus on button again
                element.children().children()[ 0 ].focus();
            }

            // just to create a random variable name
            function genRandomString( length ) {
                var possible    = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz';
                var temp        = '';
                for( var i=0; i < length; i++ ) {
                    temp += possible.charAt( Math.floor( Math.random() * possible.length ));
                }
                return temp;
            }

            // count leading spaces
            $scope.prepareGrouping = function() {
                var spacing     = 0;
                angular.forEach( $scope.filteredModel, function( value, key ) {
                    value[ $scope.spacingProperty ] = spacing;
                    if ( value[ attrs.groupProperty ] === true ) {
                        spacing+=2;
                    }
                    else if ( value[ attrs.groupProperty ] === false ) {
                        spacing-=2;
                    }
                });
            }

            // prepare original index
            $scope.prepareIndex = function() {
                var ctr = 0;
                angular.forEach( $scope.filteredModel, function( value, key ) {
                    value[ $scope.indexProperty ] = ctr;
                    ctr++;
                });
            }

            // navigate using up and down arrow
            $scope.keyboardListener = function( e ) {

                var key = e.keyCode ? e.keyCode : e.which;
                var isNavigationKey = false;

                // ESC key (close)
                if ( key === 27 ) {
                    e.preventDefault();
                    e.stopPropagation();
                    $scope.toggleCheckboxes( e );
                }


                // next element ( tab, down & right key )
                else if ( key === 40 || key === 39 || ( !e.shiftKey && key == 9 ) ) {

                    isNavigationKey = true;
                    prevTabIndex = $scope.tabIndex;
                    $scope.tabIndex++;
                    if ( $scope.tabIndex > formElements.length - 1 ) {
                        $scope.tabIndex = 0;
                        prevTabIndex = formElements.length - 1;
                    }
                    while ( formElements[ $scope.tabIndex ].disabled === true ) {
                        $scope.tabIndex++;
                        if ( $scope.tabIndex > formElements.length - 1 ) {
                            $scope.tabIndex = 0;
                        }
                        if ( $scope.tabIndex === prevTabIndex ) {
                            break;
                        }
                    }
                }

                // prev element ( shift+tab, up & left key )
                else if ( key === 38 || key === 37 || ( e.shiftKey && key == 9 ) ) {
                    isNavigationKey = true;
                    prevTabIndex = $scope.tabIndex;
                    $scope.tabIndex--;
                    if ( $scope.tabIndex < 0 ) {
                        $scope.tabIndex = formElements.length - 1;
                        prevTabIndex = 0;
                    }
                    while ( formElements[ $scope.tabIndex ].disabled === true ) {
                        $scope.tabIndex--;
                        if ( $scope.tabIndex === prevTabIndex ) {
                            break;
                        }
                        if ( $scope.tabIndex < 0 ) {
                            $scope.tabIndex = formElements.length - 1;
                        }
                    }
                }

                if ( isNavigationKey === true ) {

                    e.preventDefault();

                    // set focus on the checkbox
                    formElements[ $scope.tabIndex ].focus();
                    var actEl = document.activeElement;

                    if ( actEl.type.toUpperCase() === 'CHECKBOX' ) {
                        $scope.setFocusStyle( $scope.tabIndex );
                        $scope.removeFocusStyle( prevTabIndex );
                    }
                    else {
                        $scope.removeFocusStyle( prevTabIndex );
                        $scope.removeFocusStyle( helperItemsLength );
                        $scope.removeFocusStyle( formElements.length - 1 );
                    }
                }

                isNavigationKey = false;
            }

            // set (add) CSS style on selected row
            $scope.setFocusStyle = function( tabIndex ) {
                angular.element( formElements[ tabIndex ] ).parent().parent().parent().addClass( 'multiSelectFocus' );
            }

            // remove CSS style on selected row
            $scope.removeFocusStyle = function( tabIndex ) {
                angular.element( formElements[ tabIndex ] ).parent().parent().parent().removeClass( 'multiSelectFocus' );
            }

            /*********************
             *********************
             *
             * 1) Initializations
             *
             *********************
             *********************/

                // attrs to $scope - attrs-$scope - attrs - $scope
                // Copy some properties that will be used on the template. They need to be in the $scope.
            $scope.groupProperty    = attrs.groupProperty;
            $scope.tickProperty     = attrs.tickProperty;
            $scope.directiveId      = attrs.directiveId;

            // Unfortunately I need to add these grouping properties into the input model
            var tempStr = genRandomString( 5 );
            $scope.indexProperty = 'idx_' + tempStr;
            $scope.spacingProperty = 'spc_' + tempStr;

            // set orientation css
            if ( typeof attrs.orientation !== 'undefined' ) {

                if ( attrs.orientation.toUpperCase() === 'HORIZONTAL' ) {
                    $scope.orientationH = true;
                    $scope.orientationV = false;
                }
                else
                {
                    $scope.orientationH = false;
                    $scope.orientationV = true;
                }
            }

            // get elements required for DOM operation
            checkBoxLayer = element.children().children().next()[0];

            // set max-height property if provided
            if ( typeof attrs.maxHeight !== 'undefined' ) {
                var layer = element.children().children().children()[0];
                angular.element( layer ).attr( "style", "height:" + attrs.maxHeight + "; overflow-y:scroll;" );
            }

            // some flags for easier checking
            for ( var property in $scope.helperStatus ) {
                if ( $scope.helperStatus.hasOwnProperty( property )) {
                    if (
                        typeof attrs.helperElements !== 'undefined'
                        && attrs.helperElements.toUpperCase().indexOf( property.toUpperCase() ) === -1
                    ) {
                        $scope.helperStatus[ property ] = false;
                    }
                }
            }
            if ( typeof attrs.selectionMode !== 'undefined' && attrs.selectionMode.toUpperCase() === 'SINGLE' )  {
                $scope.helperStatus[ 'all' ] = false;
                $scope.helperStatus[ 'none' ] = false;
            }

            // helper button icons.. I guess you can use html tag here if you want to.
            $scope.icon        = {};
            $scope.icon.selectAll  = '&#10003;';    // a tick icon
            $scope.icon.selectNone = '&times;';     // x icon
            $scope.icon.reset      = '&#8630;';     // undo icon
            // this one is for the selected items
            $scope.icon.tickMark   = '&#10003;';    // a tick icon

            // configurable button labels
            if ( typeof attrs.translation !== 'undefined' ) {
                $scope.lang.selectAll       = $sce.trustAsHtml( $scope.icon.selectAll  + '&nbsp;&nbsp;' + $scope.translation.selectAll );
                $scope.lang.selectNone      = $sce.trustAsHtml( $scope.icon.selectNone + '&nbsp;&nbsp;' + $scope.translation.selectNone );
                $scope.lang.reset           = $sce.trustAsHtml( $scope.icon.reset      + '&nbsp;&nbsp;' + $scope.translation.reset );
                $scope.lang.search          = $scope.translation.search;
                $scope.lang.nothingSelected = $sce.trustAsHtml( $scope.translation.nothingSelected );
            }
            else {
                $scope.lang.selectAll       = $sce.trustAsHtml( $scope.icon.selectAll  + '&nbsp;&nbsp;Select All' );
                $scope.lang.selectNone      = $sce.trustAsHtml( $scope.icon.selectNone + '&nbsp;&nbsp;Select None' );
                $scope.lang.reset           = $sce.trustAsHtml( $scope.icon.reset      + '&nbsp;&nbsp;Reset' );
                $scope.lang.search          = 'Search...';
                $scope.lang.nothingSelected = 'None Selected';
            }
            $scope.icon.tickMark = $sce.trustAsHtml( $scope.icon.tickMark );

            // min length of keyword to trigger the filter function
            if ( typeof attrs.MinSearchLength !== 'undefined' && parseInt( attrs.MinSearchLength ) > 0 ) {
                vMinSearchLength = Math.floor( parseInt( attrs.MinSearchLength ) );
            }

            /*******************************************************
             *******************************************************
             *
             * 2) Logic starts here, initiated by watch 1 & watch 2
             *
             *******************************************************
             *******************************************************/

                // watch1, for changes in input model property
                // updates multi-select when user select/deselect a single checkbox programatically
                // https://github.com/isteven/angular-multi-select/issues/8
            $scope.$watch( 'inputModel' , function( newVal ) {
                if ( newVal ) {
                    $scope.refreshOutputModel();
                    $scope.refreshButton();
                }
            }, true );

            // watch2 for changes in input model as a whole
            // this on updates the multi-select when a user load a whole new input-model. We also update the $scope.backUp variable
            $scope.$watch( 'inputModel' , function( newVal ) {
                if ( newVal ) {
                    $scope.backUp = angular.copy( $scope.inputModel );
                    $scope.updateFilter();
                    $scope.prepareGrouping();
                    $scope.prepareIndex();
                    $scope.refreshOutputModel();
                    $scope.refreshButton();
                }
            });

            // watch for changes in directive state (disabled or enabled)
            $scope.$watch( 'isDisabled' , function( newVal ) {
                $scope.isDisabled = newVal;
            });

            // this is for touch enabled devices. We don't want to hide checkboxes on scroll.
            var onTouchStart = function( e ) {
                $scope.$apply( function() {
                    $scope.scrolled = false;
                });
            };
            angular.element( document ).bind( 'touchstart', onTouchStart);
            var onTouchMove = function( e ) {
                $scope.$apply( function() {
                    $scope.scrolled = true;
                });
            };
            angular.element( document ).bind( 'touchmove', onTouchMove);

            // unbind document events to prevent memory leaks
            $scope.$on( '$destroy', function () {
                angular.element( document ).unbind( 'touchstart', onTouchStart);
                angular.element( document ).unbind( 'touchmove', onTouchMove);
            });
        }
    }
}]).run( [ '$templateCache' , function( $templateCache ) {
    var template =
        '<span class="multiSelect float">' +
            // main button
        '<button id="{{directiveId}}" type="button"' +
        'ng-click="toggleCheckboxes( $event ); refreshSelectedItems(); refreshButton(); prepareGrouping; prepareIndex();"' +
        'ng-bind-html="varButtonLabel"' +
        'ng-disabled="disable-button"' +
        '>' +
        '</button>' +
            // overlay layer
        '<div class="checkboxLayer">' +
            // container of the helper elements
        '<div class="helperContainer" ng-if="helperStatus.filter || helperStatus.all || helperStatus.none || helperStatus.reset ">' +
            // the search box
        //'<div class="line" style="position:relative" ng-if="helperStatus.filter">'+
        //    // textfield
        '<input placeholder="{{lang.search}}" type="text"' +
        'ng-click="select( \'filter\', $event )" '+
        'ng-model="inputLabel.labelFilter" '+
        'ng-change="searchChanged()" class="inputFilter"'+
        '/">'+
            // clear button
            //'<button type="button" class="clearButton" ng-click="clearClicked( $event )" >×</button> '+
        //'</div> '+
        '</div> '+
            // selection items
        '<div class="checkBoxContainer">'+
        '<div '+
        'ng-repeat="item in filteredModel | filter:removeGroupEndMarker" class="multiSelectItem"'+
        'ng-class="{selected: item[ tickProperty ], horizontal: orientationH, vertical: orientationV, multiSelectGroup:item[ groupProperty ], disabled:itemIsDisabled( item )}"'+
        'ng-click="syncItems( item, $event, $index );" '+
        'ng-mouseleave="removeFocusStyle( tabIndex );"> '+
            // this is the spacing for grouped items
        '<div class="acol" ng-if="item[ spacingProperty ] > 0" ng-repeat="i in numberToArray( item[ spacingProperty ] ) track by $index">'+
        '</div>  '+
        '<div class="acol">'+
        '<label>'+
            // input, so that it can accept focus on keyboard click
        '<input class="checkbox focusable" type="checkbox" '+
        'ng-disabled="itemIsDisabled( item )" '+
        'ng-checked="item[ tickProperty ]" '+
        'ng-click="syncItems( item, $event, $index )" />'+
            // item label using ng-bind-hteml
        '<span class="debugger-font-size" '+
        'ng-class="{disabled:itemIsDisabled( item )}" '+
        'ng-bind-html="writeLabel( item, \'itemLabel\' )">'+
        '</span>'+
        '</label>'+
        '</div>'+
            // the tick/check mark
        '<span class="tickMark" ng-if="item[ groupProperty ] !== true && item[ tickProperty ] === true" ng-bind-html="icon.tickMark"></span>'+
        '</div>'+
        '</div>'+
        '</div>'+
        '</span>';
    $templateCache.put( 'isteven-multi-select.htm' , template );
}]);


angular.module('angularResizable', [])
    .directive('resizable', function() {
        var toCall;
        function throttle(fun) {
            if (toCall === undefined) {
                toCall = fun;
                setTimeout(function() {
                    toCall();
                    toCall = undefined;
                }, 100);
            } else {
                toCall = fun;
            }
        }
        return {
            restrict: 'AE',
            scope: {
                rDirections: "=",
                rCenteredX: "=",
                rCenteredY: "=",
                rWidth: "=",
                rHeight: "=",
                rFlex: "=",
                rGrabber: "@",
                rDisabled: "@"
            },
            link: function(scope, element, attr) {

                // register watchers on width and height attributes if they are set
                scope.$watch('rWidth', function(value){
                    element[0].style.width = scope.rWidth + 'px';
                });
                scope.$watch('rHeight', function(value){
                    element[0].style.height = scope.rHeight + 'px';
                });

                element.addClass('resizable');

                var style = window.getComputedStyle(element[0], null),
                    w,
                    h,
                    dir = scope.rDirections,
                    vx = scope.rCenteredX ? 2 : 1, // if centered double velocity
                    vy = scope.rCenteredY ? 2 : 1, // if centered double velocity
                    inner = scope.rGrabber ? scope.rGrabber : '<span></span>',
                    start,
                    dragDir,
                    axis,
                    info = {};
                var updateInfo = function(e) {
                    info.width = false; 
                    info.height = false;
                    if(axis == 'x')
                        info.width = scope.rFlex ? parseInt(element[0].style.flexBasis) : parseInt(element[0].style.width);
                    else
                        info.height = scope.rFlex ? parseInt(element[0].style.flexBasis) : parseInt(element[0].style.height);
                    info.id = element[0].id;
                    info.evt = e;
                }

                var dragging = function(e) {
                    var offset = axis == 'x' ? start - e.clientX : start - e.clientY;
                    switch(dragDir) {
                        case 'top':
                            if(scope.rFlex) { element[0].style.flexBasis = h + (offset * vy) + 'px'; }
                            else {            element[0].style.height = h + (offset * vy) + 'px'; }
                            break;
                    }
                    updateInfo(e);
                    throttle(function() { scope.$emit("angular-resizable.resizing", info);});
                };
                var dragEnd = function(e) {
                    updateInfo(e);
                    scope.$emit("angular-resizable.resizeEnd", info);
                    scope.$apply();
                    document.removeEventListener('mouseup', dragEnd, false);
                    document.removeEventListener('mousemove', dragging, false);
                    element.removeClass('no-transition');
                    window.sessionStorage.setItem('acquiaLift::debug::debugWindowHeight', info.height);
                };
                var dragStart = function(e, direction) {
                    dragDir = direction;
                    axis = dragDir == 'left' || dragDir == 'right' ? 'x' : 'y';
                    start = axis == 'x' ? e.clientX : e.clientY;
                    w = parseInt(style.getPropertyValue("width"));
                    h = parseInt(style.getPropertyValue("height"));

                    //prevent transition while dragging
                    element.addClass('no-transition');

                    document.addEventListener('mouseup', dragEnd, false);
                    document.addEventListener('mousemove', dragging, false);

                    // Disable highlighting while dragging
                    if(e.stopPropagation) e.stopPropagation();
                    if(e.preventDefault) e.preventDefault();
                    e.cancelBubble = true;
                    e.returnValue = false;

                    updateInfo(e);
                    scope.$emit("angular-resizable.resizeStart", info);
                    scope.$apply();
                };

                for(var i=0;i<dir.length;i++) {
                    (function () {
                        var grabber = document.createElement('div'),
                            direction = dir[i];

                        // add class for styling purposes
                        grabber.setAttribute('class', 'rg-' + dir[i]);
                        grabber.innerHTML = inner;
                        document.getElementById("debugger").appendChild(grabber);
                        grabber.ondragstart = function() { return false }
                        grabber.addEventListener('mousedown', function(e) {
                          var disabled = (scope.rDisabled == 'true');
                          if (!disabled && e.which == 1) {
                            // left mouse click
                            dragStart(e, direction);
                          }
                        }, false);
                    }())
                }

            }
        }
    });

//# sourceMappingURL=acquia_lift.debugger.js.map