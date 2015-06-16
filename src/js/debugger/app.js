//var $ = JQuery;
var app = angular.module("debuggerModule", []);
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
            console.log($window.localStorage[key]);
            return JSON.parse($window.localStorage[key] || '{}');
        },
        clear: function(){
            window.localStorage.clear();
        }
    }
}])

app.controller("DebuggerController", function($scope, $timeout, liftwebURl, debuggerFactory, $localstorage, $window, liftDebugger, debugPrefix){
    $scope.url = liftwebURl;
    $scope.items = [];
    $scope.tab = 'log';
    $scope.profile = {};

    for (var key in $window.localStorage){
        console.log(key);
        if(key.indexOf(debugPrefix) >= 0){
            console.log($localstorage.getObject(key));
            $timeout(function(index){
                return function(){
                    $scope.items.push($localstorage.getObject(index));
                }
            }(key));
        }
    }

    var debugConsole = new liftDebugger.debugger(document.getElementById('debugger'));

    $(document).bind("acquiaLiftDebugEvent", function(e, value){
//        alert(value.key);
        console.log(value);
        if(key.indexOf(debugPrefix) >= 0) {
            $timeout(function (index) {
                return function () {
                    $scope.items.push($localstorage.getObject(index));
                }
            }(value.key));
        }
    });
    $scope.profile.personId = Drupal.acquiaLiftProfilesDebug.getPersonId();
    $scope.profile.touchId = Drupal.acquiaLiftProfilesDebug.getTouchId();
    $scope.profile.allSegments = Drupal.acquiaLiftProfilesDebug.getAllSegments();
    $scope.profile.curSegments = Drupal.acquiaLiftProfilesDebug.getCurrentSegments();
    $scope.profile.identities = Drupal.acquiaLiftProfilesDebug.getAdditionalIdentities();
    $scope.profile.overrideSegments = Drupal.acquiaLiftProfilesDebug.getOverrideSegments();


});

