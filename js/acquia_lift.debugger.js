//var $ = JQuery;
var app = angular.module("debuggerModule", ['isteven-multi-select', 'autocomplete', 'LocalStorageModule']);
app.constant('liftwebURl', "");
app.value('debugPrefix', 'acquiaLift::debug');
app.factory('debuggerFactory', function($http, liftwebURl){
    var factory = {};

    factory.ajax = function(){
        $http({
            url: liftwebURl,
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

    .factory('liftDebugger', function(){
        var Lift = Lift || {};

        (function () {

            'use strict';

            Lift.debugger = function (element, options) {

                this.element =
                    this.options =
                        this.triggerOpen =
                            this.triggerMaximize =
                                this.triggerClose =
                                    this.triggerDestroy =
                                        this.isOpen =
                                            this.isMaximized =
                                                this.isClosed = null;

                this.init(element, options);

            };

            Lift.debugger.DEFAULTS = {};

            Lift.debugger.prototype.init = function (element, options) {

                this.element = element;
                this.options = options;
                this.triggerOpen = this.getTrigger('open');
                this.triggerMaximize = this.getTrigger('maximize');
                this.triggerClose = this.getTrigger('close');
                this.triggerDestroy = this.getTrigger('destroy');
                this.isOpem = false;
                this.isMaximized = false;
                this.isClosed = true;

                this.render();

            };

            Lift.debugger.prototype.getTrigger = function (type) {

                var trigger = this.element.getElementsByClassName('debugger__action__' + type)[0],
                    functionId = 'click' + type.charAt().toUpperCase() + type.slice(1);

                trigger.addEventListener('click', this[functionId].bind(this));

                return trigger;

            }

            Lift.debugger.prototype.clickOpen = function (event) {
                this.open();

            }

            Lift.debugger.prototype.clickMaximize = function (event) {
                this.maximize();

            }

            Lift.debugger.prototype.clickClose = function (event) {
                this.close();

            }

            Lift.debugger.prototype.clickDestroy = function (event) {
                window.localStorage.clear();
                this.destroy();

            }

            Lift.debugger.prototype.setOpen = function () {

                this.isMaximized = false;
                this.isClosed = false;
                this.isOpen = true;

            };

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

            Lift.debugger.prototype.open = function () {
                this.setOpen();
                this.render();

            };

            Lift.debugger.prototype.maximize = function () {
                this.setMaximized();
                this.render();

            };

            Lift.debugger.prototype.close = function () {

                this.setClosed();
                this.render();
            };

            Lift.debugger.prototype.render = function () {

                if (!this.element.classList.contains('debugger-processed')) {
                    this.element.classList.add('debugger');
                    this.element.classList.add('debugger-processed');
                }

                if (this.isClosed) {
                    this.element.classList.remove('is-open');
                    this.element.classList.remove('is-maximized');
                    this.element.classList.add('is-closed');
                    this.triggerOpen.setAttribute('aria-hidden', 'false');
                    this.triggerMaximize.setAttribute('aria-hidden', 'false');
                    this.triggerDestroy.setAttribute('aria-hidden', 'false');
                    this.triggerClose.setAttribute('aria-hidden', 'true');
                }

                if (this.isOpen) {
                    this.element.classList.add('is-open');
                    this.element.classList.remove('is-maximized');
                    this.element.classList.remove('is-closed');
                    this.triggerClose.setAttribute('aria-hidden', 'false');
                    this.triggerMaximize.setAttribute('aria-hidden', 'false');
                    this.triggerDestroy.setAttribute('aria-hidden', 'false');
                    this.triggerOpen.setAttribute('aria-hidden', 'true');
                }

                if (this.isMaximized) {
                    this.element.classList.add('is-maximized');
                    this.element.classList.remove('is-open');
                    this.element.classList.remove('is-closed');
                    this.triggerOpen.setAttribute('aria-hidden', 'false');
                    this.triggerClose.setAttribute('aria-hidden', 'false');
                    this.triggerDestroy.setAttribute('aria-hidden', 'false');
                    this.triggerMaximize.setAttribute('aria-hidden', 'true');
                }

            };

            Lift.debugger.prototype.destroy = function () {

                this.element.classList.remove('is-open');
                this.element.classList.remove('is-maximized');
                this.element.classList.remove('is-closed');
                this.element.classList.remove('debugger');
                this.element.classList.remove('debugger-processed');

            };

        })(Lift);
        return Lift;
    })

    .factory('$localstorage', ['$window', function($window) {
        return {
            set: function(key, value) {
                $window.localStorage[key] = value;
            },
            get: function(key, defaultValue) {
                return $window.localStorage[key] || defaultValue;
            },
            setObject: function(key, value) {
                if(value == undefined){
                    return;
                }
                var cache = [];
                try {
                    $window.localStorage[key] = JSON.stringify(value, function (a, b) {
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
                    window.localStorage.clear();
                }
                cache = null;
            },
            getObject: function(key) {
                return JSON.parse($window.localStorage[key] || '{}');
            },
            clear: function(){
                window.localStorage.clear();
            }
        }
    }])

app.controller("DebuggerController", function($scope, $timeout, liftwebURl, debuggerFactory, $localstorage, $window, liftDebugger, debugPrefix, $document){
    $scope.url = liftwebURl;
    $scope.items = [];
    $scope.tab = 'log';
    $scope.profile = {};
    var originalData;
    //loadFakeData($scope);
    for (var key in $window.localStorage){
        if(key.indexOf(debugPrefix) >= 0){
            $timeout(function(index){
                return function(){
                    $scope.items.push($localstorage.getObject(index));
                }
            }(key));
        }
    }

    var debugConsole = new liftDebugger.debugger(document.getElementById('debugger'));

    $document.on('acquiaLiftDebugEvent', function(event, key){
        if(key.indexOf(debugPrefix) >= 0){
            $timeout(function(index){
                return function(){
                    $scope.items.push($localstorage.getObject(index));
                    if($localstorage.getObject(index).message.indexOf("Segments Returned") >= 0){
                        $scope.profile.curSegments = Drupal.acquiaLiftProfilesDebug.getCurrentSegments();
                    }
                }
            }(key));
        }
    });

    $scope.profile.personId = Drupal.acquiaLiftProfilesDebug.getPersonId();
    $scope.profile.touchId = Drupal.acquiaLiftProfilesDebug.getTouchId();
    $scope.profile.allSegments = Drupal.acquiaLiftProfilesDebug.getAllSegments();
    $scope.profile.curSegments = Drupal.acquiaLiftProfilesDebug.getCurrentSegments();
    $scope.profile.identities = Drupal.acquiaLiftProfilesDebug.getAdditionalIdentities();

    if(Drupal.acquiaLiftProfilesDebug.getOverrideSegments().length < 1){
        $scope.profile.overrideSegments = Drupal.acquiaLiftProfilesDebug.getCurrentSegments();
        Drupal.acquiaLiftProfilesDebug.setOverrideSegments($scope.profile.overrideSegments);
    };

    //$scope.allSegments = _.difference($scope.profile.allSegments, $scope.profile.overrideSegments);
    $scope.allSegments =  $scope.profile.allSegments.filter(function(i) {return $scope.profile.overrideSegments .indexOf(i) < 0;});

    $scope.severityFilter = [
        { icon: "<img src=icons/icon_close--circle.svg />", name: "Error",ticked: true },
        { icon: "<img src=icons/icon_warning.svg />", name: "Warning", ticked: true  },
        { icon: "<img src=icons/icon_success.svg />", name:"Info",ticked: true },
    ];
    $scope.typeFilter = [
        { icon: "<img src=icons/icon_close--circle.svg />", name: "Target",ticked: false },
        { icon: "<img src=icons/icon_warning.svg />", name: "Segment", ticked: false  },
    ];

    $scope.search = function (row) {
        if($scope.severity !== undefined){
            for(var i=0; i < $scope.severity.length; i++){
                if (typeof($scope.severity[i])==="object"){
                    if($scope.severity[i].name.toUpperCase() === row.severity.toUpperCase()){
                        return true;
                    }
                }
            }
        }
        if($scope.type !== undefined){
            for(var i=0; i < $scope.type.length; i++){
                if(typeof($scope.type[i]) === "object"){
                    if($scope.type[i].name.toUpperCase() === row.type.toUpperCase()){
                        return true;
                    }
                }
            }
        }
        return false;
    };
    $scope.addPreviewItem = function (item) {

        var value;
        if (item==null){
            value = $scope.yourchoice;
        }else{
            value = item;
        }

        var indexAll = $scope.allSegments.indexOf(value);
        if(indexAll > -1){
            $scope.allSegments.splice(indexAll, 1);
            $scope.profile.overrideSegments.push(value);
            Drupal.acquiaLiftProfilesDebug.setOverrideSegments($scope.profile.overrideSegments);
        }
    }

    $scope.deletePreviewItem = function(item){
        var index = $scope.profile.overrideSegments.indexOf(item);
        if(index > -1){
            $scope.profile.overrideSegments.splice(index, 1);
        }
        $scope.allSegments.push(item);
    }

    $scope.startPreview = function(){
        originalData =  Drupal.acquiaLiftProfilesDebug.getCurrentSegments();
        //originalData = $scope.profile.curSegments;
        Drupal.acquiaLiftProfilesDebug.setOverrideSegments($scope.profile.overrideSegments);
        $scope.profile.curSegments = Drupal.acquiaLiftProfilesDebug.getOverrideSegments();
        Drupal.acquiaLiftProfilesDebug.evaluateSegment('TRUE', function(variable){

        })
        //$scope.profile.curSegments = $scope.profile.overrideSegments;
    }

    $scope.stopPreview = function(){
        Drupal.acquiaLiftProfilesDebug.setOverrideSegments(originalData);
        $scope.profile.curSegments = originalData;
    }



});

//function loadFakeData($scope){
//    //faking some data here!
//    for (i=0;i < 5;i++){
//        var item={};
//        item.type="Segment";
//        item.timestamp="timestamp";
//        item.page=i;
//        item.message="here is a message";
//        item.severity = "WARNING";
//        //item.icon= "<img src=icons/icon_close--circle.svg />";
//        $scope.items.push(item);
//    }
//    for (i=0;i < 5;i++){
//        var item={};
//        item.type="Target";
//        item.timestamp="timestamp";
//        item.page=i;
//        item.message="here is a message";
//        item.severity = "ERROR";
//        $scope.items.push(item);
//    }
//    for (i=0;i < 5;i++){
//        var item={};
//        item.type="type";
//        item.timestamp="timestamp";
//        item.page=i;
//        item.message="here is a message";
//        item.severity = "INFO";
//        $scope.items.push(item);
//    }
//    for (i=0; i < $scope.items.length; i ++){
//        var item = $scope.items[i];
//        switch(item.severity.toUpperCase()){
//            case "WARNING": item.icon="<img src=icons/icon_warning.svg />";
//                break;
//            case "ERROR" : item.icon="<img src=icons/icon_close--circle.svg />";
//                break;
//            case "INFO" : item.icon="<img src=icons/icon_success.svg />";
//                break;
//        }
//    }
////stop faking data
//}

angular.module('debuggerModule')
    .filter('to_trusted', ['$sce', function($sce){
        return function(text) {
            return $sce.trustAsHtml(text);
        };
    }]);


/*
 * Angular JS Multi Select
 * Creates a dropdown-like button with checkboxes.
 *
 * Project started on: Tue, 14 Jan 2014 - 5:18:02 PM
 * Current version: 4.0.0
 *
 * Released under the MIT License
 * --------------------------------------------------------------------------------
 * The MIT License (MIT)
 *
 * Copyright (c) 2014 Ignatius Steven (https://github.com/isteven)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * --------------------------------------------------------------------------------
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
        '<div class="line" style="position:relative" ng-if="helperStatus.filter">'+
            // textfield
        '<input placeholder="{{lang.search}}" type="text"' +
        'ng-click="select( \'filter\', $event )" '+
        'ng-model="inputLabel.labelFilter" '+
        'ng-change="searchChanged()" class="inputFilter"'+
        '/>'+
            // clear button
            //'<button type="button" class="clearButton" ng-click="clearClicked( $event )" >×</button> '+
        '</div> '+
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
            var watching = true;

            // autocompleting drop down on/off
            $scope.completing = false;

            // starts autocompleting on typing in something
            $scope.$watch('searchParam', function(newValue, oldValue){
                if (oldValue === newValue) {
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
                "placeholder": "start typing...",
                "class": "",
                "id": "",
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
                console.log("open")
                element[0].onclick = function(e){
                    if(!scope.searchParam){
                        console.log("sdsds");
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
                        e.preventDefault();
                }
            }, true);

            document.addEventListener("blur", function(e){
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
          <ul ng-show="completing && (suggestions | filter:searchFilter).length > 0">\
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

/**
 * An Angular module that gives you access to the browsers local storage
 * @version v0.2.2 - 2015-05-29
 * @link https://github.com/grevory/angular-local-storage
 * @author grevory <greg@gregpike.ca>
 * @license MIT License, http://www.opensource.org/licenses/MIT
 */
(function ( window, angular, undefined ) {
    /*jshint globalstrict:true*/
    'use strict';

    var isDefined = angular.isDefined,
        isUndefined = angular.isUndefined,
        isNumber = angular.isNumber,
        isObject = angular.isObject,
        isArray = angular.isArray,
        extend = angular.extend,
        toJson = angular.toJson;
    var angularLocalStorage = angular.module('LocalStorageModule', []);

    angularLocalStorage.provider('localStorageService', function() {

        // You should set a prefix to avoid overwriting any local storage variables from the rest of your app
        // e.g. localStorageServiceProvider.setPrefix('yourAppName');
        // With provider you can use config as this:
        // myApp.config(function (localStorageServiceProvider) {
        //    localStorageServiceProvider.prefix = 'yourAppName';
        // });
        this.prefix = 'ls';

        // You could change web storage type localstorage or sessionStorage
        this.storageType = 'localStorage';

        // Cookie options (usually in case of fallback)
        // expiry = Number of days before cookies expire // 0 = Does not expire
        // path = The web path the cookie represents
        this.cookie = {
            expiry: 30,
            path: '/'
        };

        // Send signals for each of the following actions?
        this.notify = {
            setItem: true,
            removeItem: false
        };

        // Setter for the prefix
        this.setPrefix = function(prefix) {
            this.prefix = prefix;
            return this;
        };

        // Setter for the storageType
        this.setStorageType = function(storageType) {
            this.storageType = storageType;
            return this;
        };

        // Setter for cookie config
        this.setStorageCookie = function(exp, path) {
            this.cookie.expiry = exp;
            this.cookie.path = path;
            return this;
        };

        // Setter for cookie domain
        this.setStorageCookieDomain = function(domain) {
            this.cookie.domain = domain;
            return this;
        };

        // Setter for notification config
        // itemSet & itemRemove should be booleans
        this.setNotify = function(itemSet, itemRemove) {
            this.notify = {
                setItem: itemSet,
                removeItem: itemRemove
            };
            return this;
        };

        this.$get = ['$rootScope', '$window', '$document', '$parse', function($rootScope, $window, $document, $parse) {
            var self = this;
            var prefix = self.prefix;
            var cookie = self.cookie;
            var notify = self.notify;
            var storageType = self.storageType;
            var webStorage;

            // When Angular's $document is not available
            if (!$document) {
                $document = document;
            } else if ($document[0]) {
                $document = $document[0];
            }

            // If there is a prefix set in the config lets use that with an appended period for readability
            if (prefix.substr(-1) !== '.') {
                prefix = !!prefix ? prefix + '.' : '';
            }
            var deriveQualifiedKey = function(key) {
                return prefix + key;
            };
            // Checks the browser to see if local storage is supported
            var browserSupportsLocalStorage = (function () {
                try {
                    var supported = (storageType in $window && $window[storageType] !== null);

                    // When Safari (OS X or iOS) is in private browsing mode, it appears as though localStorage
                    // is available, but trying to call .setItem throws an exception.
                    //
                    // "QUOTA_EXCEEDED_ERR: DOM Exception 22: An attempt was made to add something to storage
                    // that exceeded the quota."
                    var key = deriveQualifiedKey('__' + Math.round(Math.random() * 1e7));
                    if (supported) {
                        webStorage = $window[storageType];
                        webStorage.setItem(key, '');
                        webStorage.removeItem(key);
                    }

                    return supported;
                } catch (e) {
                    storageType = 'cookie';
                    $rootScope.$broadcast('LocalStorageModule.notification.error', e.message);
                    return false;
                }
            }());

            // Directly adds a value to local storage
            // If local storage is not available in the browser use cookies
            // Example use: localStorageService.add('library','angular');
            var addToLocalStorage = function (key, value) {
                // Let's convert undefined values to null to get the value consistent
                if (isUndefined(value)) {
                    value = null;
                } else {
                    value = toJson(value);
                }

                // If this browser does not support local storage use cookies
                if (!browserSupportsLocalStorage || self.storageType === 'cookie') {
                    if (!browserSupportsLocalStorage) {
                        $rootScope.$broadcast('LocalStorageModule.notification.warning', 'LOCAL_STORAGE_NOT_SUPPORTED');
                    }

                    if (notify.setItem) {
                        $rootScope.$broadcast('LocalStorageModule.notification.setitem', {key: key, newvalue: value, storageType: 'cookie'});
                    }
                    return addToCookies(key, value);
                }

                try {
                    if (webStorage) {webStorage.setItem(deriveQualifiedKey(key), value)};
                    if (notify.setItem) {
                        $rootScope.$broadcast('LocalStorageModule.notification.setitem', {key: key, newvalue: value, storageType: self.storageType});
                    }
                } catch (e) {
                    $rootScope.$broadcast('LocalStorageModule.notification.error', e.message);
                    return addToCookies(key, value);
                }
                return true;
            };

            // Directly get a value from local storage
            // Example use: localStorageService.get('library'); // returns 'angular'
            var getFromLocalStorage = function (key) {

                if (!browserSupportsLocalStorage || self.storageType === 'cookie') {
                    if (!browserSupportsLocalStorage) {
                        $rootScope.$broadcast('LocalStorageModule.notification.warning','LOCAL_STORAGE_NOT_SUPPORTED');
                    }

                    return getFromCookies(key);
                }

                var item = webStorage ? webStorage.getItem(deriveQualifiedKey(key)) : null;
                // angular.toJson will convert null to 'null', so a proper conversion is needed
                // FIXME not a perfect solution, since a valid 'null' string can't be stored
                if (!item || item === 'null') {
                    return null;
                }

                try {
                    return JSON.parse(item);
                } catch (e) {
                    return item;
                }
            };

            // Remove an item from local storage
            // Example use: localStorageService.remove('library'); // removes the key/value pair of library='angular'
            var removeFromLocalStorage = function () {
                var i, key;
                for (i=0; i<arguments.length; i++) {
                    key = arguments[i];
                    if (!browserSupportsLocalStorage || self.storageType === 'cookie') {
                        if (!browserSupportsLocalStorage) {
                            $rootScope.$broadcast('LocalStorageModule.notification.warning', 'LOCAL_STORAGE_NOT_SUPPORTED');
                        }

                        if (notify.removeItem) {
                            $rootScope.$broadcast('LocalStorageModule.notification.removeitem', {key: key, storageType: 'cookie'});
                        }
                        removeFromCookies(key);
                    }
                    else {
                        try {
                            webStorage.removeItem(deriveQualifiedKey(key));
                            if (notify.removeItem) {
                                $rootScope.$broadcast('LocalStorageModule.notification.removeitem', {
                                    key: key,
                                    storageType: self.storageType
                                });
                            }
                        } catch (e) {
                            $rootScope.$broadcast('LocalStorageModule.notification.error', e.message);
                            removeFromCookies(key);
                        }
                    }
                }
            };

            // Return array of keys for local storage
            // Example use: var keys = localStorageService.keys()
            var getKeysForLocalStorage = function () {

                if (!browserSupportsLocalStorage) {
                    $rootScope.$broadcast('LocalStorageModule.notification.warning', 'LOCAL_STORAGE_NOT_SUPPORTED');
                    return false;
                }

                var prefixLength = prefix.length;
                var keys = [];
                for (var key in webStorage) {
                    // Only return keys that are for this app
                    if (key.substr(0,prefixLength) === prefix) {
                        try {
                            keys.push(key.substr(prefixLength));
                        } catch (e) {
                            $rootScope.$broadcast('LocalStorageModule.notification.error', e.Description);
                            return [];
                        }
                    }
                }
                return keys;
            };

            // Remove all data for this app from local storage
            // Also optionally takes a regular expression string and removes the matching key-value pairs
            // Example use: localStorageService.clearAll();
            // Should be used mostly for development purposes
            var clearAllFromLocalStorage = function (regularExpression) {

                // Setting both regular expressions independently
                // Empty strings result in catchall RegExp
                var prefixRegex = !!prefix ? new RegExp('^' + prefix) : new RegExp();
                var testRegex = !!regularExpression ? new RegExp(regularExpression) : new RegExp();

                if (!browserSupportsLocalStorage || self.storageType === 'cookie') {
                    if (!browserSupportsLocalStorage) {
                        $rootScope.$broadcast('LocalStorageModule.notification.warning', 'LOCAL_STORAGE_NOT_SUPPORTED');
                    }
                    return clearAllFromCookies();
                }

                var prefixLength = prefix.length;

                for (var key in webStorage) {
                    // Only remove items that are for this app and match the regular expression
                    if (prefixRegex.test(key) && testRegex.test(key.substr(prefixLength))) {
                        try {
                            removeFromLocalStorage(key.substr(prefixLength));
                        } catch (e) {
                            $rootScope.$broadcast('LocalStorageModule.notification.error',e.message);
                            return clearAllFromCookies();
                        }
                    }
                }
                return true;
            };

            // Checks the browser to see if cookies are supported
            var browserSupportsCookies = (function() {
                try {
                    return $window.navigator.cookieEnabled ||
                        ("cookie" in $document && ($document.cookie.length > 0 ||
                        ($document.cookie = "test").indexOf.call($document.cookie, "test") > -1));
                } catch (e) {
                    $rootScope.$broadcast('LocalStorageModule.notification.error', e.message);
                    return false;
                }
            }());

            // Directly adds a value to cookies
            // Typically used as a fallback is local storage is not available in the browser
            // Example use: localStorageService.cookie.add('library','angular');
            var addToCookies = function (key, value, daysToExpiry) {

                if (isUndefined(value)) {
                    return false;
                } else if(isArray(value) || isObject(value)) {
                    value = toJson(value);
                }

                if (!browserSupportsCookies) {
                    $rootScope.$broadcast('LocalStorageModule.notification.error', 'COOKIES_NOT_SUPPORTED');
                    return false;
                }

                try {
                    var expiry = '',
                        expiryDate = new Date(),
                        cookieDomain = '';

                    if (value === null) {
                        // Mark that the cookie has expired one day ago
                        expiryDate.setTime(expiryDate.getTime() + (-1 * 24 * 60 * 60 * 1000));
                        expiry = "; expires=" + expiryDate.toGMTString();
                        value = '';
                    } else if (isNumber(daysToExpiry) && daysToExpiry !== 0) {
                        expiryDate.setTime(expiryDate.getTime() + (daysToExpiry * 24 * 60 * 60 * 1000));
                        expiry = "; expires=" + expiryDate.toGMTString();
                    } else if (cookie.expiry !== 0) {
                        expiryDate.setTime(expiryDate.getTime() + (cookie.expiry * 24 * 60 * 60 * 1000));
                        expiry = "; expires=" + expiryDate.toGMTString();
                    }
                    if (!!key) {
                        var cookiePath = "; path=" + cookie.path;
                        if(cookie.domain){
                            cookieDomain = "; domain=" + cookie.domain;
                        }
                        $document.cookie = deriveQualifiedKey(key) + "=" + encodeURIComponent(value) + expiry + cookiePath + cookieDomain;
                    }
                } catch (e) {
                    $rootScope.$broadcast('LocalStorageModule.notification.error',e.message);
                    return false;
                }
                return true;
            };

            // Directly get a value from a cookie
            // Example use: localStorageService.cookie.get('library'); // returns 'angular'
            var getFromCookies = function (key) {
                if (!browserSupportsCookies) {
                    $rootScope.$broadcast('LocalStorageModule.notification.error', 'COOKIES_NOT_SUPPORTED');
                    return false;
                }

                var cookies = $document.cookie && $document.cookie.split(';') || [];
                for(var i=0; i < cookies.length; i++) {
                    var thisCookie = cookies[i];
                    while (thisCookie.charAt(0) === ' ') {
                        thisCookie = thisCookie.substring(1,thisCookie.length);
                    }
                    if (thisCookie.indexOf(deriveQualifiedKey(key) + '=') === 0) {
                        var storedValues = decodeURIComponent(thisCookie.substring(prefix.length + key.length + 1, thisCookie.length))
                        try {
                            return JSON.parse(storedValues);
                        } catch(e) {
                            return storedValues
                        }
                    }
                }
                return null;
            };

            var removeFromCookies = function (key) {
                addToCookies(key,null);
            };

            var clearAllFromCookies = function () {
                var thisCookie = null, thisKey = null;
                var prefixLength = prefix.length;
                var cookies = $document.cookie.split(';');
                for(var i = 0; i < cookies.length; i++) {
                    thisCookie = cookies[i];

                    while (thisCookie.charAt(0) === ' ') {
                        thisCookie = thisCookie.substring(1, thisCookie.length);
                    }

                    var key = thisCookie.substring(prefixLength, thisCookie.indexOf('='));
                    removeFromCookies(key);
                }
            };

            var getStorageType = function() {
                return storageType;
            };

            // Add a listener on scope variable to save its changes to local storage
            // Return a function which when called cancels binding
            var bindToScope = function(scope, key, def, lsKey) {
                lsKey = lsKey || key;
                var value = getFromLocalStorage(lsKey);

                if (value === null && isDefined(def)) {
                    value = def;
                } else if (isObject(value) && isObject(def)) {
                    value = extend(def, value);
                }

                $parse(key).assign(scope, value);

                return scope.$watch(key, function(newVal) {
                    addToLocalStorage(lsKey, newVal);
                }, isObject(scope[key]));
            };

            // Return localStorageService.length
            // ignore keys that not owned
            var lengthOfLocalStorage = function() {
                var count = 0;
                var storage = $window[storageType];
                for(var i = 0; i < storage.length; i++) {
                    if(storage.key(i).indexOf(prefix) === 0 ) {
                        count++;
                    }
                }
                return count;
            };

            return {
                isSupported: browserSupportsLocalStorage,
                getStorageType: getStorageType,
                set: addToLocalStorage,
                add: addToLocalStorage, //DEPRECATED
                get: getFromLocalStorage,
                keys: getKeysForLocalStorage,
                remove: removeFromLocalStorage,
                clearAll: clearAllFromLocalStorage,
                bind: bindToScope,
                deriveKey: deriveQualifiedKey,
                length: lengthOfLocalStorage,
                cookie: {
                    isSupported: browserSupportsCookies,
                    set: addToCookies,
                    add: addToCookies, //DEPRECATED
                    get: getFromCookies,
                    remove: removeFromCookies,
                    clearAll: clearAllFromCookies
                }
            };
        }];
    });
})( window, window.angular );
//# sourceMappingURL=acquia_lift.debugger.js.map