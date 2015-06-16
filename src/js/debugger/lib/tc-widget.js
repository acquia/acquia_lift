// Trucentric Widget JS : 1.5.0-SNAPSHOT-272-4-g0c9ef71-dirty
// singleton and namespace
var TCWidget = TCWidget || (function() {
	"use strict";
	
	var ElapsedExpiry = 30 * 60 * 1000;
	
	var Log = {
		debug: function(msg) {
			var logEnabled = document.cookie.indexOf("tc_log") >= 0;
			if ('console' in self && 'log' in console && logEnabled) {
				console.log(msg);
			}
		}
	};
	
	//
	// Lang
	//
	var Lang = {
			
		isDefined: function(o) {
			return typeof o !== 'undefined';
		},

		isUndefined: function(o) {
			return !this.isDefined(o);
		},

		isNull: function(o) {
			return o === null;
		},
		
		isString: function(o) {
			return (typeof o === 'string' || o instanceof String);
		},

		"String": {
			EMPTY: ""
		},

		"Date": {
			now: function() {
				return (new Date()).getTime();
			}
		},
		
		"Object": {
			merge: function(obj1, obj2){
				if (!obj1) {
					obj1 = {};
				}
				if (obj2) {
					for ( var prop in obj2) {
						obj1[prop] = obj2[prop];
					}
				}
				return obj1;
			}
			
		}

	};
	
	//
	// CommandQueue
	//
	function CommandQueue(apiList) {
		this.api = apiList;
	};
	CommandQueue.prototype.processBacklog = function() {
		// first deal with backlog on the queue
		var backlog = Queue.getAll();
		for ( var backlogIndex = 0; backlogIndex < backlog.length; backlogIndex++ ) {
			var url = backlog[backlogIndex];
			Tracker._sendTrack(url);
		}
	}
    /**
	 * @param {Array} aq queue
	 */
	CommandQueue.prototype.processQueue = function( aq ) {
		// then with new events
		for ( var i = 0; i < aq.length; i++ ) {
			this.push( aq[i] );
		}
	};
	CommandQueue.prototype.push = function () {

		var i, method, parameterList;

		for (i = 0; i < arguments.length; i += 1) {
			parameterList = arguments[i];
			method = parameterList.shift();
			if ( Lang.isString( method ) ) {
				this.api[method].apply( this.api, parameterList ); // it's a method name
			}
			else {
				method.apply( this.api, parameterList ); // it's a function - TODO - why do we need this?
			}

		};
	};
	
	//
	// Cookie
	//
	function Cookie(name)
	{
		this.name = name;
		this.value = Lang.String.EMPTY;
		this.path = "/";
		
		// Find base domain from domain
		var domain = document.domain;
		var domainPathArray = domain.split( '.' );
		var length = domainPathArray.length;
		
		this.domain = "." + domainPathArray[length - 2] + "." + domainPathArray[length - 1];
		
		return this;
	};
	
	Cookie.prototype = {
		setValue: function(value) {
			this.value = value;
			return this;
		},
		getValue: function() {
			return this.value;
		},
		// milliseconds of cookie's life
		setExpires: function(expires) {
			this.expires = expires;
			return this;
		},
		setDomain: function(domain) {
			this.domain = domain;
			return this;
		},
		setPath: function(path) {
			this.path = path;
			return this;
		},
		toString: function() {
			return this.name + ";" + this.value + ";" + this.expires + ";" + this.path + ";" + this.domain;
		},
		size: function() {
			var expiryDate = 0;

			if (this.expires) {
				expiryDate = new Date();
				expiryDate.setTime(expiryDate.getTime() + this.expires);
			}

			var cookieString = this.name + '=' + window.encodeURIComponent(this.value) +
				(this.expires ? '; expires=' + expiryDate.toGMTString() : '') +
				'; path=' + this.path +
				'; domain=' + this.domain;
			
			return cookieString.length;
		},
		newSize: function(expires, newValue) {
			var expiryDate = expiryDate = new Date();
			expiryDate.setTime(expiryDate.getTime() + expires);

			var cookieString = this.name + '=' + window.encodeURIComponent(newValue) +
				(this.expires ? '; expires=' + expiryDate.toGMTString() : '') +
				'; path=' + this.path +
				'; domain=' + this.domain;
			
			return cookieString.length;
		},
		save: function() {
			var expiryDate = 0;

			if (this.expires) {
				expiryDate = new Date();
				expiryDate.setTime(expiryDate.getTime() + this.expires);
			}

			var cookieString = this.name + '=' + window.encodeURIComponent(this.value) +
				(this.expires ? '; expires=' + expiryDate.toGMTString() : '') +
				'; path=' + this.path +
				'; domain=' + this.domain;
			document.cookie = cookieString;
		},
		clear: function() {
			this.value = Lang.EMPTY;
			this.expires = -1;
			this.save();
		},
		exists: function() {
			var regex = new RegExp('(?:^|;)\\s?' + this.name + '=(.*?)(?:;|$)','i');
			var match = document.cookie.match(regex); // TODO - better is a document.cookie.test(regex) - does the pattern exist?
			return !Lang.isNull(match);
		},
		load: function() {
			var regex = new RegExp('(?:^|;)\\s?' + this.name + '=(.*?)(?:;|$)','i');
			var match = document.cookie.match(regex);
			this.value = match && window.decodeURIComponent(match[1]);
		}
		
	};
	
	function ScrollDetect(elemId, offset, loc, callbackFunc) {
		this.scrollToElemId = elemId;
		this.scrollToOffset = offset;
		this.scrolledPastTop = false;
		this.scrolledPastBottom = false;
		this.scrollPrevious = 0;
		this.callbackFunction = callbackFunc;
		this.relativeLocation = loc;
		this.onScroll = (loc == "bottom" ? this.onScrollToBottom : this.onScrollToTop );
	}
	ScrollDetect.getDocumentHeight = function () {
		return Math.max(
				Math.max(document.body.scrollHeight, document.documentElement.scrollHeight),
				Math.max(document.body.offsetHeight, document.documentElement.offsetHeight),
				Math.max(document.body.clientHeight, document.documentElement.clientHeight)
		);
	};
	ScrollDetect.findBestLocationOfHiddenElement = function( elem, loc ) {
		var nextElem = elem.next();
		while ( nextElem.length > 0 && nextElem.css('display') == 'none' ) {
			nextElem = nextElem.next();
		}
		if ( nextElem.length > 0 ) {
			return nextElem.offset().top;
		}

		var prevElem = elem.prev();
		while ( prevElem.length > 0 && prevElem.css('display') == 'none' ) {
			prevElem = prevElem.prev();
		}
		if ( prevElem.length > 0 ) {
			return prevElem.offset().top + prevElem.outerHeight(true);
		}
	
		var parentElem = elem.parent();
		if ( parentElem.length == 0 ) {
			if ( loc == 'bottom' ) {
				return ScrollDetect.getDocumentHeight();
			}
			else {
				return 0;
			}
		}
		if ( parentElem.css('display') == 'none' ) {
			return ScrollDetect.findBestLocationOfHiddenElement( parentElem, loc );
		}
		
		if ( loc == 'bottom' ) {
			return parentElem.offset().top + parentElem.outerHeight(true);
		}
		else {
			return parentElem.offset().top;
		}
	};
	
	ScrollDetect.prototype.isAtScrollPosition = function() {
		if ( this.relativeLocation == 'bottom' ) {
			var height = ScrollDetect.getDocumentHeight();
			if ( this.scrollToElemId && this.scrollToElemId != 'body' ) {
				var scrollToElem = jQuery( this.scrollToElemId );
				if ( scrollToElem.length == 0) {
					return false;
				}
				height = scrollToElem.offset().top + scrollToElem.outerHeight(true);
				if ( scrollToElem.length > 0 && scrollToElem.css('display') == 'none' ) {
					height = ScrollDetect.findBestLocationOfHiddenElement( scrollToElem, this.relativeLocation );
				}
			}
			var pastBottom = jQuery(window).height() + jQuery(window).scrollTop() >= (height - this.scrollToOffset);
			return pastBottom;		
		}
		else {
			var height = 0;
			if ( this.scrollToElemId && this.scrollToElemId != 'body' ) {
				var scrollToElem = jQuery( this.scrollToElemId );
				if ( scrollToElem.length == 0 ) {
					return false;
				}
				height = scrollToElem.offset().top;
				if ( scrollToElem.length > 0 && scrollToElem.css('display') == 'none' ) {
					height = ScrollDetect.findBestLocationOfHiddenElement( scrollToElem, this.relativeLocation );
				}
			}
			var pastTop = jQuery(window).scrollTop() <= height + this.scrollToOffset;
			return pastTop;
		}
	};
	
	ScrollDetect.prototype.onScrollToBottom = function() {
		var height = ScrollDetect.getDocumentHeight();
		if ( this.scrollToElemId && this.scrollToElemId != 'body' ) {
			var scrollToElem = jQuery( this.scrollToElemId );
			if ( scrollToElem.length == 0 ) {
				return;
			}
			height = scrollToElem.offset().top + scrollToElem.outerHeight(true);
			if ( scrollToElem && scrollToElem.css('display') == 'none' ) {
				height = ScrollDetect.findBestLocationOfHiddenElement( scrollToElem, this.relativeLocation );
			}
		}
		var pastBottom = jQuery(window).height() + jQuery(window).scrollTop() >= (height - this.scrollToOffset);
		if(!this.scrolledPastBottom && pastBottom) {
			this.callbackFunction(jQuery(window).height() + jQuery(window).scrollTop());
			this.scrolledPastBottom = true;
		} else {
			if(!pastBottom) this.scrolledPastBottom = false;
		}
		this.scrollPrevious = jQuery(window).scrollTop();
	};
	ScrollDetect.prototype.onScrollToTop = function () {
		var height = 0;
		if ( this.scrollToElemId && this.scrollToElemId != 'body' ) {
			var scrollToElem = jQuery( this.scrollToElemId );
			if ( scrollToElem.length == 0 ) {
				return;
			}
			height = scrollToElem.offset().top;
			if ( scrollToElem && scrollToElem.css('display') == 'none' ) {
				height = ScrollDetect.findBestLocationOfHiddenElement( scrollToElem, this.relativeLocation );
			}
		}
		var pastTop = jQuery(window).scrollTop() <= height + this.scrollToOffset;
		if(!this.scrolledPastTop && pastTop) {
			this.callbackFunction(jQuery(window).scrollTop());
			this.scrolledPastTop = true;
		} else {
			if(!pastTop) this.scrolledPastTop = false;
		}
		this.scrollPrevious = jQuery(window).scrollTop();
	};
	
	var TCWidgetAjaxQueue = jQuery({});
	jQuery.TCWidgetAjaxQueue = function( ajaxOpts ) {

		var jqXHR, dfd;
		var promise = null;
		if ( jQuery.Deferred ) {
			dfd = jQuery.Deferred();
			promise = dfd.promise();
		}
		TCWidgetAjaxQueue.queue( doRequest );
		
		if ( jQuery.Deferred ) {
			promise.abort = function( statusText ) {
				if ( jqXHR ) {
					return jqXHR.abort( statusText );
				}
				
				var queue = TCWidgetAjaxQueue.queue(),
					index = jQuery.inArray( doRequest, queue );
				if ( index > -1 ) {
					queue.splice( index, 1 );
				}
				dfd.rejectWith( ajaxOpts.context || ajaxOpts,
					[ promise, statusText, "" ] );
				return promise;
			};
		}
		
		function doRequest( next ) {
			if ( dfd ) {
				jqXHR = jQuery.ajax( ajaxOpts )
					.done( dfd.resolve )
					.fail( dfd.reject )
					.then( next, next );
			}
			else {
				jqXHR = jQuery.ajax( ajaxOpts );
			}
		}
	};
	
	function WidgetState( id, templateStr, config, target, onAddFunc ) {
		this.id = id;
		this.templateStr = templateStr;
		this.config = config;
		this.target = target;
		this.added = false;
		this.loadingFromCapture = null;
		this.loaded = false;
		if ( onAddFunc ) {
			this.onAddFunc = onAddFunc;
		}
		this.excluded = { };
	};
	WidgetState.prototype.addExclusion = function( pageKey, pageVals ) {
		// TODO : If list already exists then add the vals to the array
		this.excluded[ pageKey ] = pageVals;
	};
	WidgetState.prototype.isExcludedFromPage = function() {
		if ( this.config.openDirection == 'embed' && 
		     this.config.embedVisibility && this.config.embedVisibility == 'n/a') {
			return true;
		}
		var excludedUrls = this.excluded[ 'url' ];
		if ( excludedUrls ) {
			for ( var i = 0; i < excludedUrls.length; i++ ) {
				var isExactMatch = ( excludedUrls[i].indexOf('http://') == 0 || 
				       excludedUrls[i].indexOf('https://') == 0 );
				if ( isExactMatch && document.location.href == excludedUrls[i] ) {
					return true;
				}
				else if ( !isExactMatch && document.location.href.indexOf( excludedUrls[i] ) != -1 ) {
					return true;
				}
			}
		}
		return false;
	};
	WidgetState.prototype.animateMaximized = function ( expandButton, collapseButton ) {
		var curWidget = this;
		this.jQueryObj.queue(this.id+'fx', function(next) { 
			if ( curWidget.config.openDirection == 'left' ) {
				curWidget.jQueryObj.animate( {'right': '0px'},
						100 );
			}
			else if ( curWidget.config.openDirection == 'right' ) {
				curWidget.jQueryObj.animate( {'left': '0px'},
						100 );
			}
			else if ( curWidget.config.openDirection == 'down' ) {
				curWidget.jQueryObj.animate( {'top': '0px'},
						100 );
			}
			else if ( curWidget.config.openDirection == 'up' ) {
				curWidget.jQueryObj.animate( {'bottom': '0px'},
						100 );
			}
			else if ( curWidget.config.openDirection == 'appear' ) {
				curWidget.jQueryObj.show();
			}
			if ( curWidget.config.expandButtonId ) {
				expandButton.hide();
				collapseButton.show();
			}
			else if ( curWidget.config.openDirection == 'left' || curWidget.config.openDirection == 'up' ) {
				expandButton.html( "&#9660;" );
			}
			else if ( curWidget.config.openDirection == 'right' || curWidget.config.openDirection == 'down' ) {
				expandButton.html( "&#9650;" );
			}
			if ( next ) {
				next();
			}
		});
		if ( this.config.openDirection == 'appear' ) {
			if ( Lang.isUndefined( this.closeListener ) ) {
				var curWidget = this;
				this.closeListener = function(e) {
					if ( curWidget.config.hideOverride ) {
						if ( !curWidget.jQueryObj.has( e.target ).length && ( curWidget.jQueryObj.attr("id") != e.target.id ) ) {
							curWidget.jQueryObj.hide();
						} else {
							return;
						}
					} else {
						curWidget.jQueryObj.hide();
					}
					if ( curWidget.config.closeButtonId ) {
						curWidget.jQueryObj.find( "#" + curWidget.config.closeButtonId ).unbind( 'click', curWidget.closeListener );
					} else {
						jQuery( 'html' ).unbind( 'click', curWidget.closeListener );
					}
					curWidget.addedCloseListener = false;
				};
			}

			if ( Lang.isUndefined( this.addedCloseListener ) ||
				this.addedCloseListener == false ) {
				if ( this.config.closeButtonId ) {
					this.jQueryObj.find( "#" + this.config.closeButtonId ).bind( 'click', this.closeListener );
				} else {
					jQuery( 'html' ).bind( 'click', this.closeListener );
				}
				this.addedCloseListener = true;
			}
		};
	};
	WidgetState.prototype.animateMinimized = function( expandButton, collapseButton ) {
		var curWidget = this;
		this.jQueryObj.queue(this.id+'fx', function(next) { 
			if ( curWidget.config.openDirection == 'left' ) {
				curWidget.jQueryObj.animate(
						{ 'right': '-'+ curWidget.jQueryObj.find("#" + curWidget.config.contentId ).outerWidth(true) +'px' },
						100 );
			}
			else if ( curWidget.config.openDirection == 'right' ) {
				curWidget.jQueryObj.animate(
						{ 'left': '-'+ curWidget.jQueryObj.find("#" + curWidget.config.contentId ).outerWidth(true) +'px' },
						100 );
			}
			else if ( curWidget.config.openDirection == 'down' ) {
				curWidget.jQueryObj.animate(
						{ 'top': '-'+ curWidget.jQueryObj.find("#" + curWidget.config.contentId ).outerHeight(true) +'px' },
						100 );
			}
			else if ( curWidget.config.openDirection == 'up' ) {
				curWidget.jQueryObj.animate(
						{ 'bottom': '-'+ curWidget.jQueryObj.find("#" + curWidget.config.contentId ).outerHeight(true) +'px' },
						100 );
			}
			else if ( curWidget.config.openDirection == 'appear' ) {
				curWidget.jQueryObj.show();
			}

			if ( curWidget.config.expandButtonId ) {
				collapseButton.hide();
				expandButton.show();
			}
			else if ( curWidget.config.openDirection == 'left' || curWidget.config.openDirection == 'up' ) {
				expandButton.html( "&#9650;" );
			}
			else if ( curWidget.config.openDirection == 'right' || curWidget.config.openDirection == 'down' ) {
				expandButton.html( "&#9660;" );
			}
			if ( next )  {
				next();
			}
		});
		if ( this.config.openDirection == 'appear' ) {
			if ( Lang.isUndefined( this.closeListener ) ) {
				var curWidget = this;
				this.closeListener = function(e) {
					if ( curWidget.config.hideOverride ) {
						if ( !curWidget.jQueryObj.has( e.target ).length && ( curWidget.jQueryObj.attr("id") != e.target.id ) ) {
							curWidget.jQueryObj.hide();
						} else {
							return;
						}
					} else {
						curWidget.jQueryObj.hide();
					}
					if ( curWidget.config.closeButtonId ) {
						curWidget.jQueryObj.find( "#" + curWidget.config.closeButtonId ).unbind( 'click', curWidget.closeListener );
					} else {
						jQuery( 'html' ).unbind( 'click', curWidget.closeListener );
					}
					curWidget.addedCloseListener = false;
				};
			}

			if ( Lang.isUndefined( this.addedCloseListener ) ||
			    this.addedCloseListener == false ) {
				if ( this.config.closeButtonId ) {
					this.jQueryObj.find( "#" + this.config.closeButtonId ).bind( 'click', this.closeListener );
				} else {
					jQuery( 'html' ).bind( 'click', this.closeListener );
				}
				this.addedCloseListener = true;
			}
		};
	};
	WidgetState.prototype.animateHide = function( multiplier ) { 
		var curWidget = this;
		this.jQueryObj.queue(this.id+'fx', function(next) {
			if ( curWidget.config.openDirection == 'left' ) {
				curWidget.jQueryObj.css( "right", '-'+ curWidget.jQueryObj.outerWidth(true)*multiplier +'px' );
			}
			else if ( curWidget.config.openDirection == 'right' ) {
				curWidget.jQueryObj.css( "left", '-'+ curWidget.jQueryObj.outerWidth(true)*multiplier +'px' );
			}
			else if ( curWidget.config.openDirection == 'down' ) {
				curWidget.jQueryObj.css( "top", '-'+ curWidget.jQueryObj.outerHeight(true)*multiplier +'px' );
			}
			else if ( curWidget.config.openDirection == 'up' ) {
				curWidget.jQueryObj.css( "bottom", '-'+ curWidget.jQueryObj.outerHeight(true)*multiplier +'px' );
			}
			else if ( curWidget.config.openDirection == 'appear' ) {
				curWidget.jQueryObj.hide();
			}
			if ( next )  {
				next();
			}
		});
		if ( this.config.openDirection == 'appear' ) {
			if ( this.addedCloseListener && this.addedCloseListener == true ) {
				if ( this.config.closeButtonId ) {
					this.jQueryObj.find( "#" + this.config.closeButtonId ).unbind( 'click', this.closeListener );
				} else {
					jQuery( 'html' ).unbind( 'click', this.closeListener );
				}
				this.addedCloseListener = false;
			}
		};
	};
	WidgetState.prototype.setUserClosed = function( isClosed ) {
		var userClosedCookie = new Cookie( 'tc_wos_' + this.id );
		userClosedCookie.load();
		userClosedCookie.setValue( isClosed ? "true" : "false" ).save();
	};
	
	WidgetState.prototype.isUserClosed = function() {
		var userClosedCookie = new Cookie( 'tc_wos_' + this.id );
		userClosedCookie.load();
		return userClosedCookie.getValue() == "true";
	};
	WidgetState.prototype.clearUserClosed = function() {
		var userClosedCookie = new Cookie( 'tc_wos_' + this.id );
		userClosedCookie.load();
		userClosedCookie.clear();
	};
	WidgetState.prototype.isUserClosedCleared = function() {
		var userClosedCookie = new Cookie( 'tc_wos_' + this.id );
		userClosedCookie.load();
		var userClosed = userClosedCookie.getValue();
		return Lang.isNull( userClosed ) || userClosed == "";
	};
	
	WidgetState.prototype.addToPage = function( loadCallback ) {
		if ( this.added ) {
			if ( loadCallback ) {
				loadCallback();
			}
			return;
		}

		this.jQueryObj = jQuery( this.templateStr );
		if ( this.config.openDirection == 'embed' ) {
			var origDiv = jQuery( "#" + this.config.pageDivId );
			if ( origDiv.length == 0 ) {
				Log.debug( "Could not find '#" + this.config.pageDivId + "'" );
				this.config.embedVisibility = 'n/a';
				this.jQueryObj.hide();
				jQuery('body').append( this.jQueryObj );
			}
			else {
				this.config.embedVisibility = 'none';
				if ( origDiv.css('visibility') == 'hidden' ) {
					this.config.embedVisibility = 'hidden';
				}
				if ( this.config.embedVisibility == 'hidden' ) {
					this.jQueryObj.css('visibility', 'hidden');
				}
				else {
					this.jQueryObj.hide();
				}
				origDiv.replaceWith( this.jQueryObj );
			}
		}
		
		if ( this.config.openDirection != 'embed' ) {
			this.jQueryObj.css('visibility', 'hidden');
			jQuery('body').append( this.jQueryObj );
			var curWidget = this;
		}
		
		if ( this.config.openDirection != 'embed' ) {
			var expandButton = this.jQueryObj.find("#"+this.config.idPrefix+"expand");
			var collapseButton = expandButton;
			if ( this.config.collapseButtonId && this.config.expandButtonId ) {
				expandButton = this.jQueryObj.find("#"+this.config.expandButtonId);
				collapseButton = this.jQueryObj.find("#"+this.config.collapseButtonId);
			}
			
			var curWidget = this;
			if ( expandButton ) {
				expandButton.bind('click',function() {
					curWidget.jQueryObj.stop(curWidget.id + 'fx', true);
					if ( !curWidget.config.expandButtonId &&
						 ( ( expandButton.html() == String.fromCharCode(9660) && 
						     (curWidget.config.openDirection == 'up' || curWidget.config.openDirection == 'left' ) ) ||
						   ( expandButton.html() == String.fromCharCode(9650) &&
						     (curWidget.config.openDirection == 'down' || curWidget.config.openDirection == 'right' ) ) ) ) {
						curWidget.animateMinimized( expandButton, collapseButton );
						if ( curWidget.loadedData && curWidget.loadedData["_behavior"].keepOpenedState ) {
							curWidget.setUserClosed( true );
						}
					}
					else {
						curWidget.animateMaximized( expandButton, collapseButton );
						if ( curWidget.loadedData && curWidget.loadedData["_behavior"].keepOpenedState ) {
							curWidget.setUserClosed( false );
						}
						if ( !curWidget.opened && curWidget.loadedData && curWidget.loadedData["_behavior"].onOpen ) {
							curWidget.loadedData["_behavior"].onOpen();
						}
						curWidget.opened = true;
					}
					curWidget.jQueryObj.dequeue( curWidget.id + 'fx' );
					return false;
				} );
			}
			
			if ( collapseButton && this.config.collapseButtonId && this.config.collapseButtonId != this.config.expandButtonId ) {
				collapseButton.hide();
				collapseButton.bind('click',function() {
					curWidget.jQueryObj.stop(curWidget.id + 'fx', true);
					curWidget.animateMinimized( expandButton, collapseButton );					
					curWidget.jQueryObj.dequeue( curWidget.id + 'fx' );
					if ( curWidget.loadedData && curWidget.loadedData["_behavior"].keepOpenedState ) {
						curWidget.setUserClosed( true );
					}
					return false;
				} );
			}
		}
		this.added = true;
		if ( this.onAddFunc ) {
			this.onAddFunc( this );
		}
		if ( loadCallback ) {
			loadCallback();
		}
	};
	
	WidgetState.prototype.loadState = function() {
		var stateCookie = new Cookie('tc_wq_'+this.id);
		stateCookie.load();
		var stateValueJSONStr = stateCookie.getValue();
		if ( Lang.isNull( stateValueJSONStr ) ) {
			return;
		}
		var data = JSON.parse( stateValueJSONStr );
		if ( Lang.isNull( data ) ) {
			return;
		}
		
		this.loadData( data );
		if ( data['_behavior'].keepState && 
			 data['_behavior'].keepState != 'infinite' ) {
			 data['_behavior'].keepState = data['_behavior'].keepState - 1;
			if ( data['_behavior'].keepState <= 0 ) {
				stateCookie.clear();
				delete data['_behavior']['keepState'];
			}
			else {
				stateCookie.setValue(JSON.stringify(data));
				stateCookie.setExpires( ElapsedExpiry ).save();
			}
		}
		if ( this.config.openDirection == 'embed' ) {
			if ( this.config.embedVisibility == 'hidden' ) {
				this.jQueryObj.css('visibility', 'visible');
			}
			else if ( this.config.embedVisibility != 'n/a' ) { 
				this.jQueryObj.show();
			}
		}
		else {
			var expandButton = this.jQueryObj.find( "#"+this.config.idPrefix+"expand" );
			var collapseButton = expandButton;
			if ( this.config.collapseButtonId && this.config.expandButtonId ) {
				expandButton = this.jQueryObj.find("#"+this.config.expandButtonId);
				collapseButton = this.jQueryObj.find("#"+this.config.collapseButtonId);
			}
			
			this.animateMinimized( expandButton, collapseButton );
			this.jQueryObj.dequeue( this.id + 'fx' );
		}
		this.opened = true;
	};
	
	WidgetState.prototype.show = function( onLoad ) {
		if ( this.loadedData["_behavior"].control ) {
			return;
		}

		var userClosed = null;
		var userClosedCleared = false;
		var openMinimized = false;
		if ( this.config.openDirection != 'embed' && this.config.openDirection != 'appear' ) {
			if ( this.loadedData && this.loadedData["_behavior"].delayBeforeOpen > 0 ) {
				 this.jQueryObj.delay( this.loadedData["_behavior"].delayBeforeOpen, this.id + 'fx' );
			}
			var curWidget = this;
			userClosedCleared = this.isUserClosedCleared();
			userClosed = this.isUserClosed();
			var expandButton = this.jQueryObj.find( "#"+this.config.idPrefix+"expand" );
			var collapseButton = expandButton;
			if ( this.config.collapseButtonId && this.config.expandButtonId ) {
				expandButton = this.jQueryObj.find("#"+this.config.expandButtonId);
				collapseButton = this.jQueryObj.find("#"+this.config.collapseButtonId);
			}
			var atScrollPosition = true;
			if ( this.scrollDetector ) {
				atScrollPosition = this.scrollDetector.isAtScrollPosition();
			}
			if ( (onLoad == false || atScrollPosition) && (userClosedCleared || userClosed == false) ) {
				this.animateMaximized( expandButton, collapseButton );
			}
			else {
				this.animateMinimized( expandButton, collapseButton );
				openMinimized = true;
			}
			this.jQueryObj.dequeue( this.id + 'fx' );
		} else if ( this.config.openDirection == 'appear' ) {
			if ( this.loadedData && this.loadedData["_behavior"].delayBeforeOpen > 0 ) {
				 this.jQueryObj.delay( this.loadedData["_behavior"].delayBeforeOpen, this.id + 'fx' );
			}
			this.animateMaximized( expandButton, collapseButton );
			this.jQueryObj.dequeue( this.id + 'fx' );
		}
		
		if ( openMinimized == false ) {
			if ( this.loadedData && this.loadedData["_behavior"].onOpen ) {
				this.loadedData["_behavior"].onOpen();
			}
			this.opened = true;
		}
		if ( !this.loadedData || !this.loadedData["_behavior"].keepOpenedState ) {
			this.clearUserClosed();
		} else if ( this.loadedData && this.loadedData["_behavior"].keepOpenedState ) {
			if ( this.isUserClosedCleared() ) {
				this.setUserClosed( false );
			}
		}	
		if ( this.loadedData && this.loadedData["_behavior"].xPosition && this.loadedData["_behavior"].yPosition) {
			this.jQueryObj.offset( { top: this.loadedData["_behavior"].yPosition, left: this.loadedData["_behavior"].xPosition } );
		}
		var widgetDiv = jQuery( "#" + this.config.idPrefix + "div" );
		if ( this.loadedData && this.loadedData["_behavior"].width) {
			if (widgetDiv) {
				widgetDiv.width( this.loadedData["_behavior"].width );
			}
		}
		if ( this.loadedData && this.loadedData["_behavior"].height) {
			if (widgetDiv) {
				widgetDiv.height( this.loadedData["_behavior"].height );
			}
		}

		if ( this.config.openDirection == 'embed' ) {
			if ( this.config.embedVisibility == 'hidden' ) {
				this.jQueryObj.css('visibility', 'visible');
			}
			else if ( this.config.embedVisibility != 'n/a' ) {
				this.jQueryObj.show();
			}
		}
		else {
			if ( ( userClosedCleared == true || userClosed == false ) && 
			       this.loadedData && this.loadedData["_behavior"].openDuration > 0 ) {
				this.jQueryObj.delay( this.loadedData["_behavior"].openDuration, this.id + 'fx' );
				var expandButton = this.jQueryObj.find( "#"+this.config.idPrefix+"expand" );
				var collapseButton = expandButton;
				if ( this.config.collapseButtonId && this.config.expandButtonId ) {
					expandButton = this.jQueryObj.find("#"+this.config.expandButtonId);
					collapseButton = this.jQueryObj.find("#"+this.config.collapseButtonId);
				}
				this.animateMinimized( expandButton, collapseButton );
			}
			this.jQueryObj.dequeue( this.id + 'fx' );
		}
	};
	
	WidgetState.prototype.loadData = function( data ) {
		Log.debug(this.id + " onLoad ( "+JSON.stringify(data)+" )");
		for ( var key in data ) {
			if ( data.hasOwnProperty( key ) == false || key == "_behavior" ) {
				continue;
			}
				
			if ( jQuery.isArray( data[key] ) ) {
				var numTemplates = this[this.config.classPrefix+key+'_numTemplates'];
				if ( Lang.isUndefined(numTemplates) ) {
					numTemplates = this.jQueryObj.find('.'+this.config.classPrefix+key).length;
					this[this.config.classPrefix+key+'_numTemplates'] = numTemplates;
				}
				this.loadElementsFromTemplate( this.config.classPrefix, 
						this.jQueryObj.find('.'+this.config.classPrefix+key+':first').parent(),
						key, data[key], numTemplates );
				
				if ( this.config.tabsId ) {

					// TODO: Check for multiple tab templates
					this.loadElementsFromTemplate( this.config.classPrefix,
						this.jQueryObj.find('#'+this.config.tabsId ), 
						null, data[key], 1 );
					
					var tabsId = this.config.tabsId;
					var tabsUL = this.jQueryObj.find('#'+this.config.tabsId );
					
					// remove old listener...
					if ( this.tabListener ) {
						tabsUL.find('a').unbind( 'click', this.tabListener );
					}

					tabsUL.children().not(':last').each( function(index,element) {
						jQuery(element).find('a').attr( 'href', '#'+tabsId+index);
					});
					this.jQueryObj.find('.'+this.config.classPrefix+key).not(':last').each( function(index,element) { 
						jQuery(element).attr('id', tabsId+index );
					});
					
					// setup active tab and listener
					var activeTab, activeContent, tabList = tabsUL.find('a').not(':last');

					activeTab = jQuery(tabList[0]);
					activeTab.addClass('active');
					activeContent = jQuery(activeTab.attr('href'));

					// Hide the remaining content
					tabList.not(activeTab).each(function () {
						jQuery(jQuery(this).attr('href')).hide();
					});

					this.tabListener = function(e){
						// Make the old tab inactive.
						activeTab.removeClass('active');
						activeContent.hide();

						// Update the variables with the new link and content
						activeTab = jQuery(this);
						activeContent = jQuery(jQuery(this).attr('href'));

						// Make the tab active.
						activeTab.addClass('active');
						activeContent.show();

						// Prevent the anchor's default click action
						e.preventDefault();
					};
					
					// Bind the click event handler
					tabsUL.find('a').bind('click', this.tabListener );
				}
			}
			else {
				var keyElem = this.jQueryObj.find( "#"+this.config.idPrefix+key );
				if ( keyElem.length == 0 ) {
					Log.debug("Could not find '#"+this.config.idPrefix+key+"'");
					continue;
				}
				this.loadElement( keyElem, data[key] );
			}
		}
		
		this.animationQueue = this.jQueryObj.queue( this.id + 'fx' );
		
		if ( this.config.openDirection != 'embed' ) {
			if ( data["_behavior"].control ) {
				this.animateHide( 1 );
				this.jQueryObj.css('visibility', 'hidden');
				this.jQueryObj.dequeue( this.id + 'fx' );
			}
			else {
				this.animateHide( 1.5 );
				this.jQueryObj.css('visibility', 'visible'); // we can show it because it is off screen
				this.jQueryObj.dequeue( this.id + 'fx' );
			}
		}
		
		if ( data["_behavior"].minimizeQueue ) {
			WidgetQueue.minimize( data["_behavior"].minimizeQueue );
		}
			
		if ( data["_behavior"].keepState ) {
			var stateCookie = new Cookie('tc_wq_'+this.id);
			stateCookie.setValue( JSON.stringify( data ) );
			stateCookie.setExpires( ElapsedExpiry ).save();
		}
		if ( data["_behavior"].sessionId ) {
			WidgetManager.setSessionId( data["_behavior"].sessionId );
		}
		// TODO: Test if the widget is already opened
		this.opened = false; // newly loaded data means the widget hasn't been opened
		this.loaded = true;
		this.loadedData = data;
		this.jQueryObj.trigger( "tcLoad", [ data ] );
	};

	WidgetState.prototype.loadElementsFromTemplate = function( classPrefix, parent, key, dataList, numTemplates ) {
		var templateElemList = null;
		if ( numTemplates == 1 ) {
			templateElemList = parent.children(':last');
			if ( key ) {
				parent.find( '.'+classPrefix+key).not(':last').remove();
				templateElemList = parent.find( '.'+classPrefix+key+':last');
			}
			else {
				parent.children().not(':last').remove();
			}
		}
		else {
			if ( key ) {
				var curLen = parent.find( '.'+classPrefix+key ).length;
				parent.find( '.'+classPrefix+key ).each( function( index, element ) {
					if ( index < curLen - numTemplates ) {
						element.remove();
					}
				});
				templateElemList = parent.find( '.'+classPrefix+key );
			}
			else {
				var curLen = parent.children().length;
				parent.children().each( function( index, element ) {
					if ( index < curLen - numTemplates ) {
						element.remove();
					}
				});
				templateElemList = parent.children();
			}
		}
		var widgetState = this;
		jQuery( dataList ).each( function(index, dataItem) {
			var dataElem = jQuery(templateElemList[index%numTemplates]).clone();
			for ( var itemKey in dataItem ) {
				if ( dataItem.hasOwnProperty( itemKey ) == false ) {
					continue;
				}
				var dataKeyElem = dataElem.find("."+classPrefix+itemKey);
				if ( dataKeyElem.length == 0 ) {
					Log.debug("Could not find '."+classPrefix+itemKey+"'");
					continue;
				}
				if ( jQuery.isArray( dataItem[itemKey] ) ) {
					var numSubElementTemplates = dataElem.find('.'+widgetState.config.classPrefix+itemKey).length;
					widgetState.loadElementsFromTemplate( classPrefix, dataElem, itemKey, dataItem[itemKey], numSubElementTemplates );
				}
				else { 
					widgetState.loadElement(dataKeyElem, dataItem[itemKey] );
				}
			}
			jQuery(templateElemList[0]).before( dataElem );
			dataElem.show();
		});
		
		// check for next/previous buttons (only when there is a key)
		if ( key && this.config.nextButtonClassSuffix && this.config.previousButtonClassSuffix ) {
			var nextButtonElem = parent.find( "."+classPrefix+key+'_'+this.config.nextButtonClassSuffix );
			var prevButtonElem = null;
			if ( this.config.previousButtonClassSuffix ) {
				prevButtonElem = parent.find( "."+classPrefix+key+'_'+this.config.previousButtonClassSuffix );
			}
			if ( nextButtonElem.length != 0 && prevButtonElem.length != 0 ) { // TODO: Support for loop-scrolling (no prev button)
				var visibilityUsed = false;
				if ( this.config["."+classPrefix+key+'_'+this.config.previousButtonClassSuffix] ) {
					visibilityUsed = this.config["."+classPrefix+key+'_'+this.config.previousButtonClassSuffix];
				}
				else {
					visibilityUsed = (!prevButtonElem.is(':visible') && prevButtonElem.css('display') != 'none');
					this.config["."+classPrefix+key+'_'+this.config.previousButtonClassSuffix] = visibilityUsed;
				}
				var itemList = parent.find( '.'+classPrefix+key );
				nextButtonElem.find('a').bind('click', function(e) {
					var curVisiblePos = 0;
					for ( var index = 0; index < itemList.length - numTemplates; index++ ) {
						if ( jQuery( itemList[index] ).css('display') != 'none' ) {
							curVisiblePos = index;
							break;
						}
					};
					curVisiblePos = curVisiblePos + numTemplates;
					if ( curVisiblePos > itemList.length - numTemplates - numTemplates ) {
						curVisiblePos = itemList.length - numTemplates - numTemplates;
						if ( visibilityUsed ) { 
							nextButtonElem.css('visibility', 'hidden');
						}
						else {
							nextButtonElem.hide();
						}
					}
					if ( curVisiblePos > 0 ) {
						if ( visibilityUsed ) { 
							prevButtonElem.css('visibility', 'visible');
						}
						else {
							prevButtonElem.show();
						}
					}
					for ( var index = 0; index < itemList.length - numTemplates; index++ ) {
						if ( index < curVisiblePos ) {
							jQuery(itemList[index]).hide();
						}
						else if ( index >= curVisiblePos && index < curVisiblePos + numTemplates ) {
							jQuery(itemList[index]).show();
						}
						else {
							jQuery(itemList[index]).hide();
						}
					}
					// Prevent the anchor's default click action
					e.preventDefault();
				});
				
				prevButtonElem.find('a').bind('click', function(e) {
					var curVisiblePos = 0;
					for ( var index = 0; index < itemList.length - numTemplates; index++ ) {
						if ( jQuery( itemList[index] ).css('display') != 'none' ) {
							curVisiblePos = index;
							break;
						}
					};
					curVisiblePos = curVisiblePos - numTemplates;
					if ( curVisiblePos < 0 ) {
						curVisiblePos = 0;
						if ( visibilityUsed ) { 
							prevButtonElem.css('visibility', 'hidden');
						}
						else {
							prevButtonElem.hide();
						}
					}
					
					if ( curVisiblePos < itemList.length - numTemplates - numTemplates ) {
						if ( visibilityUsed ) { 
							nextButtonElem.css('visibility', 'visible');
						}
						else {
							nextButtonElem.show();
						}
					}
					
					for ( var index = 0; index < itemList.length - numTemplates; index++ ) {
						if ( index < curVisiblePos ) {
							jQuery(itemList[index]).hide();
						}
						else if ( index >= curVisiblePos && index < curVisiblePos + numTemplates ) {
							jQuery(itemList[index]).show();
						}
						else {
							jQuery(itemList[index]).hide();
						}
					}
					// Prevent the anchor's default click action
					e.preventDefault();
				});
				if ( visibilityUsed ) { 
					if ( dataList.length > numTemplates ) {
						nextButtonElem.css('visibility', 'visible');
					}
					else {
						nextButtonElem.css('visibility', 'hidden');
					}
					prevButtonElem.css('visibility', 'hidden');
				} else {
					if ( dataList.length > numTemplates ) {
						nextButtonElem.show();
					}
					else {
						nextButtonElem.hide();
					}
					prevButtonElem.hide();
				}
				for ( var index = 0; index < itemList.length - numTemplates; index++ ) {
					if ( index >= 0 && index < numTemplates ) {
						jQuery(itemList[index]).show();
					}
					else {
						jQuery(itemList[index]).hide();
					}
				}
			}
		}
		templateElemList.hide();
	};
		
	WidgetState.prototype.loadElement = function ( elem, data ) {
		var nodeName = null;
		if ( elem.prop ) {
			nodeName = elem.prop('nodeName').toLowerCase();
		}
		else {
			nodeName = elem.attr('nodeName').toLowerCase();
		}
		if ( nodeName == 'img' ) {
			elem.attr( 'src', data );
		}
		else if ( nodeName == "a" ) {
			elem.css( 'cursor', 'pointer' );
			if ( data.href ) {
				elem.attr( 'href', data.href );
			}
			if ( data.onClick ) {
				elem.click( data.onClick );
			}

			if ( data.text ) {
				elem.html( data.text );
			}
			else if ( data.html ) {
				elem.html( data.html );
			}

			var childNode = elem.children(':first');
			if ( childNode.length != 0  ) {
				childNode.attr('src', data.img);
			}
		}
		else if ( nodeName == 'form' ) {
			if ( data.action ) {
				elem.attr( 'action', data.action );
			}
		}
		else { 
			elem.html( data );
		}
	};
	
	WidgetState.prototype.createScrollDetector = function() {
		if ( this.isUserClosed() == false &&
		     this.config.scrollToElemId != 'body' &&
			 ( this.config.scrollDistanceFromBottom ||
			   ( this.config.scrollDistanceFromTop && 
				 this.config.scrollDistanceFromTop > 0 ) ) ) {
			var scrollDistance = 0;
			var scrollDir = 'bottom';
			if ( this.config.scrollDistanceFromBottom ) {
				scrollDistance = this.config.scrollDistanceFromBottom;
				scrollDir = 'bottom';
			}
			else if ( this.config.scrollDistanceFromTop ) {
				scrollDistance = this.config.scrollDistanceFromTop;
				scrollDir = 'top';
			}

			return new ScrollDetect( 
				this.config.scrollToElemId, 
				scrollDistance,
				scrollDir,
				WidgetManager.createWidgetAddToPageCallback( this,
					WidgetManager.createWidgetJsonpLoadDataCallback( this,
						WidgetManager.createWidgetShowCallback( this, false ) ) ) 
			);
		}
		return null;
	};

	var WidgetManager = {
		sessionCookie: new Cookie("tc_ws"),
		sessionId : null,
		widgets : {},
		scrollDetectors : [],
		onLoadCallback : null,
		debug : false,
		
		setOnLoadCallback : function( callback ) {
			this.onLoadCallback = callback;
		},
		
		setDebug : function( debugOn ) {
			this.debug = debugOn;
		},
		
		isDebug : function() {
			return this.debug;
		},
		
		setSessionId : function( id ) {
			this.sessionCookie.setValue(id);
			this.sessionCookie.setExpires( ElapsedExpiry ).save();
			this.sessionId = id;
		},
		
		getSessionId : function( ) {
			if ( Lang.isNull( this.sessionId ) ) {
				this.sessionCookie.load();
				this.sessionId =  this.sessionCookie.getValue();
				return this.sessionId;
			}
			else {
				return this.sessionId;
			}
		},
		
		addWidget : function( id, templateStr, config, target, onAddFunc ) { 
			this.widgets[id] = new WidgetState( id, templateStr, config, target, onAddFunc );
		},
	
		addExclusionForWidget : function( pageKey, pageVals, widgetKeys ) {
			for ( var i = 0; i < widgetKeys.length; i++ ) {
				var widget = this.widgets[widgetKeys[i]];
				if ( Lang.isUndefined( widget ) ) {
					continue;
				}
				widget.addExclusion( pageKey, pageVals );
			}
		},
		
		hasWidgets : function() {
		    var size = 0;
		    for ( var key in this.widgets) {
		    	if (this.widgets.hasOwnProperty(key)) {
		    		size++;
		    	}
		    }
		    return size > 0;
		},
		
		hasWidgetsForPage : function() {
			var returnVal = false;
			jQuery.each( this.widgets, function() { 
				returnVal = returnVal || !this.isExcludedFromPage();
			} );
			return returnVal;
		},
		
		loadWidgetData : function( widgetListByLocation, widgetLocations, index ) {
			Log.debug("load widgets from location "+widgetLocations[index]);
			var widgetList = widgetListByLocation[ widgetLocations[index] ];
			var callbacks = [];
			for ( var i = 0; i < widgetList.length; i++ ) {
				var widget = widgetList[i];
				callbacks.push(WidgetManager.createWidgetShowCallback( widget, true ));
			}
			
			if ( widgetLocations.length > index + 1 && index < 10 ) {
				var widgetMgr = this;
				this.jsonpLoadData( widgetList, widgetLocations[index], function() {
					for ( var callbackIndex = 0; callbackIndex < callbacks.length; callbackIndex++ ) {
						callbacks[callbackIndex]();
					}
					widgetMgr.loadWidgetData( widgetListByLocation, widgetLocations, index + 1 );
				} );
			}
			else {
				this.jsonpLoadData( widgetList, widgetLocations[index], function() {
					for ( var callbackIndex = 0; callbackIndex < callbacks.length; callbackIndex++ ) {
						callbacks[callbackIndex]();
					}
				} );
			}
		},
		
		onPushTrack : function(fromQueue) {
			// Load Widgets 
			var widgetsToLoadByLocation = {};
			var widgetLocations = [];
			if ( Lang.isUndefined( fromQueue ) || fromQueue == false ) {
				for ( var key in this.widgets ) {
					if ( this.widgets.hasOwnProperty( key ) == false ) {
						continue;
					}			
					var widget = this.widgets[key];
				
					if ( widget.config.whenToLoad == 'track' &&
					     widget.loaded == false ) {
						if  ( Lang.isUndefined( widgetsToLoadByLocation[widget.target.location] ) ) {
							widgetsToLoadByLocation[widget.target.location] = [];
							widgetLocations.push( widget.target.location );
						}
						widgetsToLoadByLocation[widget.target.location].push( widget );
					}
				}
			}
			if ( widgetLocations.length > 0 ) {
				this.loadWidgetData( widgetsToLoadByLocation, widgetLocations, 0 );
			}
			else {
				this.jsonpLoadData();
			}
		},
		
		jsonpLoadData : function( widgetList, location, successCallback ) {
			var captures = WidgetQueue.getAll();
			var curCapture = null;
			var curCaptureId = null;
			if ( captures.length > 0 ) {
				if ( captures[captures.length-1].indexOf("{") != -1 ) { 
					curCapture = JSON.parse(captures[captures.length-1]);
					curCaptureId = curCapture.i.c;
				}
				else {
					curCaptureId = captures[captures.length-1];
				}
			}
			if ( widgetList ) {
				if ( curCaptureId ) {
					var newWidgetList = [];
					for ( var i = 0; i < widgetList.length; i++ ) {
						if ( widgetList[i].loadingFromCapture == null ) {
							widgetList[i].loadingFromCapture = curCaptureId;
							newWidgetList.push( widgetList[i] );
						}
					}
					widgetList = newWidgetList;
				}
				if ( widgetList.length == 0 && captures[captures.length-1].indexOf("{") == -1 ) {
					return;
				}
			}
			var fullLocation = this.getLocation(widgetList, location, captures);
			var curOnLoadCallback = this.onLoadCallback;
			jQuery.TCWidgetAjaxQueue( {
				url : fullLocation,
				cache : false,
				dataType : "jsonp",
				success : function(data, text, request ) {
					// data could be an action object (old way) or an array of action objects (new way)
					var onLoadData = null;
					if ( !jQuery.isArray( data ) ) {
						if ( data["segments"] ) {
							onLoadData = data;
							data = { "_behavior" : onLoadData["_behavior"] };
						}
					}
					else {
						var onLoadDataIndex = -1;
						for ( var dataI = 0; dataI < data.length; dataI++ ) {
							var nextData = data[dataI];
							if ( nextData["segments"] ) {
								onLoadDataIndex = dataI;
								onLoadData = nextData;
								break;
							}
						}
						if ( onLoadData ) {
							data.splice( onLoadDataIndex, 1 );
						}
					}
					if ( widgetList ) {
						if ( !jQuery.isArray( data ) && 
							Lang.isUndefined( data["_behavior"].widgetId ) ) {
							// let all widgets handle the control data
							for ( var i = 0; i < widgetList.length; i++ ) {
								widgetList[i].loadData( data );
							}
						}
						else {
							for ( var i = 0; i < widgetList.length; i++ ) {
								var widget = widgetList[i];
								var widgetData = null;
								if ( jQuery.isArray( data ) ) {
									for ( var dataIndex = 0; dataIndex < data.length; dataIndex++ ) {
										var nextData = data[dataIndex];
										if ( nextData["_behavior"].widgetId == widget.id ) {
											widgetData = nextData;
											break;
										}
									}
								}
								else if ( data["_behavior"].widgetId == widget.id ) {
									widgetData = data;
								}
								if ( widgetData ) {
									widget.loadData( widgetData );
								}
								else { // no data was sent back so control behavior
									widget.loadData( { '_behavior' : { 'control' : true } } );
								}
							}
						}
					}
					else { // make sure the session id and minimize queue are processed
						if ( !jQuery.isArray( data ) ) {
							if ( data["_behavior"].minimizeQueue ) {
								WidgetQueue.minimize( data["_behavior"].minimizeQueue );
							}
							if ( data["_behavior"].sessionId ) {
								WidgetManager.setSessionId( data["_behavior"].sessionId );
							}
						}
					}
					var onLoadSegments = [];
					if ( WidgetManager.isDebug() ) {
						if ( onLoadData && onLoadData["segments"] ) {
							onLoadSegments = onLoadData["segments"];
							jQuery(document).trigger("segmentsUpdated", onLoadSegments, curCapture);
						}
						else {
							jQuery(document).trigger("segmentsUpdated", onLoadSegments, curCapture);
						}
					}
					else {
						if ( onLoadData && onLoadData["segments"] ) {
							onLoadSegments = onLoadData["segments"];
						}
					}

					if ( curOnLoadCallback ) {
						curOnLoadCallback( onLoadSegments, curCapture );
					}

					if ( successCallback ) {
						successCallback();
					}
				},
				error : function() {
					Log.debug("Error sending capture");
				}
			} );
		},

		getLocation : function (widgetList, location, captures) {
			var returnLocation = dataStoreUrls.tcaction;
			if ( location ) {
				returnLocation = location;
			}
			if ( this.getSessionId() ) {
				returnLocation += ";tc_ws="+this.getSessionId();
			}
			returnLocation += "?";
			returnLocation += "tcla="+encodeURIComponent( WidgetQueue._tcla );
			returnLocation += "&tcptid="+encodeURIComponent( WidgetQueue._tcptid );
			returnLocation += "&tcttid="+encodeURIComponent( WidgetQueue._tcttid );
			returnLocation += "&tclv="+encodeURIComponent( WidgetQueue._tclv );
			returnLocation += "&tcep="+encodeURIComponent( WidgetQueue._tcep );
			if( WidgetQueue._tcnu) {
				returnLocation += "&tcnu="+encodeURIComponent( WidgetQueue._tcnu );
			}			
			if  ( widgetList ) {
				for ( var i = 0; i < widgetList.length; i++ ) {
					var widget = widgetList[i];
					returnLocation += "&tcnw="+encodeURIComponent( widget.id );
				}
			}
			if ( captures.length > 0 ) {
				returnLocation += "&tctc="+encodeURIComponent( captures[captures.length-1] );
				captures.pop();
				WidgetQueue.minimize(captures.length);
				returnLocation += "&tctl="+encodeURIComponent( JSON.stringify( captures ) );
			}
			return returnLocation;
		},

		onInit : function() {
			var widgetManager = this;
			// Add Widgets
			jQuery.each( this.widgets, function() { 
				if ( this.isExcludedFromPage() ) {
					return;
				}
				
				var scrollDetector = this.createScrollDetector();
				if ( scrollDetector ) {
					widgetManager.scrollDetectors.push( scrollDetector );
					this.scrollDetector = scrollDetector;
				}
				if ( this.config.whenToAdd == 'init' ) {
					this.addToPage( function( widget ) {
						return function() {
							widget.loadState();
							if ( widget.config.whenToLoad == 'init' ) {
								widgetManager.jsonpLoadData( [widget], widget.target.location );
							}
						};
					}(this) );
				}
				else if ( this.config.whenToLoad == 'init' ) {
					this.loadState();
					if ( this.config.whenToLoad == 'init' ) {
						widgetManager.jsonpLoadData( [widget], widget.target.location );
					}	
				}
			} );
			// Attach listeners
			// TODO: Only scroll to bottom supported 
			jQuery(window).scroll( function( widgetManager ) {
				return function() { widgetManager.onScroll() }; }(this) );
		},

		onScroll : function() {
			jQuery.each( this.scrollDetectors, function() {
				this.onScroll();
			} );
		},
		
		createWidgetShowCallback : function( widget, onLoad ) {
			return function() { 
				if ( !widget.opened ) {
					widget.show( onLoad );
				}
			};
		},
		
		createWidgetJsonpLoadDataCallback : function( widget, callback ) {
			var widgetManager = this;
			return function() { 
				if ( !widget.loaded ) {
					widgetManager.jsonpLoadData( [widget],
						widget.target.location,
						callback );
				}
				else {
					callback();
				}
			};
		},
		
		createWidgetAddToPageCallback : function( widget, callback ) {
			return function() {
				if ( !widget.added ) {
					widget.addToPage( callback );
				}
				else {
					callback();
				}
			};
		}
		
	};

	var WidgetQueue = {

		_cookie: new Cookie("tc_wq"),
		_tcla: "",
		_tcptid: "",
		_tcttid: "",
		_tclv: "",
		_tcep: "",
		_elementCache : [],
		_isElementsCached : false,
		_save: function(elements) {
			this._elementCache = elements;
			this._isElementsCached = true;
			if (!elements || elements.length == 0) {
				this._cookie.setValue("");
			}
			else {
				var maxQueueCookieSize = 4093;
				if (Lang.isDefined(TC_CONF) && Lang.isDefined(TC_CONF.maxQueueCookieSize)) {
					maxQueueCookieSize = TC_CONF.maxQueueCookieSize;
				}

				// Making sure cookie size less than 4093 before saving.
				while (this._cookie.newSize(ElapsedExpiry, elements.join("|")) > maxQueueCookieSize) {
					elements.splice(0, 1);
				}

				if (!elements || elements.length == 0) {
					this._cookie.setValue("");
				}
				else {
					this._cookie.setValue(elements.join("|"));
				}
			}
			this._cookie.setExpires(ElapsedExpiry).save();
		},
		_encode: function(element) {
			return element.replace(/ /g, "+").replace(/\|/g, "%7C");	
		},
		_decode: function(element) {
			return element.replace(/\+/g, " ").replace(/%7C/g, "|");
		},
		_getAllEncoded: function() {
			if ( this._isElementsCached ) {
				return this._elementCache.concat();
			}
			this._cookie.load();
			var value = this._cookie.getValue();
			if ( !value ) { // no queue, return empty queue
				return [];
			}
			else {
				return value.split("|");
			}
		},
		getAll: function() {
			var elements = this._getAllEncoded();
			for ( var inel = 0; inel < elements.length; inel++) {
				elements[inel] = this._decode(elements[inel]);
			}
			return elements;
		},
		getLength: function() {
			var elements = this._getAllEncoded();
			return elements.length;
		},
		add: function( o ) {
			if ( !o ) { // no value provided, return
				return;
			}
			Log.debug( o );
			var elements = this._getAllEncoded();
			for ( var inel = 0; inel < elements.length; inel++) {
				if (o == elements[inel]) {
					return;
				}
			}
			o = this._encode(o);
			elements.push(o);
			this._save(elements);
		},
		remove: function( index ) {
			var elements = this._getAllEncoded();
			elements.splice(index, 1);
			this._save(elements);
		},
		minimize: function( index ) {
			var elements = this._getAllEncoded();
			for ( var inel = 0; inel < elements.length && inel <= index; inel++) {
				if ( elements[inel].indexOf("{") != -1 ) {
					var nextElem = JSON.parse( elements[inel] );
					if ( nextElem.l && nextElem.i.c ) {
						elements[inel] = nextElem.i.c;
					}
					else {
						elements[inel] = elements[inel];
					}
				}
				else {
					elements[inel] = elements[inel];
				}
			}
			this._save(elements);
		},
		pushTrack: function(params, fromQueue) {
			if ( !WidgetManager.hasWidgets() && !(params["evalSegments"] && params["evalSegments"] === true) ) {
				return; // for now don't store
			}
			var tc = {
				v : {},
				e : {},
				i : {},
				l : {},
				m : {},
				x : {}
			};
		    for (var key in params) {
				if ( key == 'tcptid' ) {
					this._tcptid = params[key];
					continue;
				}
				else if ( key == 'tcttid' ) {
					this._tcttid = params[key];
					continue;
				}
				else if ( key == 'tclv' ) {
					this._tclv = params[key];
					continue;
				}
				else if ( key == 'tcla' ) {
					this._tcla = params[key];
					continue;
				}
				else if ( key == 'tcep' ) {
					this._tcep = params[key];
					continue;
				}
				else if ( key == "tcctid") {
					tc.i.c = params[key];
				}
				else if ( key == 'tcvc' ) {
					tc.v.c = params[key];
				}
				else if ( key == 'tcvv' ) {
					tc.v.v = params[key];
				}
				else if ( key == 'tcvr' ) {
					tc.v.r = params[key];
				}
				else if ( key == 'tcvt' ) {
					tc.v.t = params[key];
				}
				else if ( key == 'tcect' ) {
					tc.e.ct = params[key];
				}
				else if ( key == 'tclrnd' ) {
					tc.l.rnd = params[key];
				}
				else if ( key == 'tcii' ) {
					tc.i.i = params[key];
				}
				else if ( key == 'tciis' ) {
					tc.i.is = params[key];
				}
				else if ( key == 'tciic' ) {
					tc.i.ic = params[key];
				}
		    	else if ( params.hasOwnProperty( key ) ) {
					if ( key.indexOf('tci') == 0 ) {
						if ( Lang.isUndefined( tc.i.identities ) ) {
							tc.i.identities = {};
						}
						tc.i.identities[ key.substring('tci'.length) ] = params[key];
					}
					else if ( key.indexOf('tcm') == 0 ) {
						tc.m[ key.substring('tcm'.length) ] = params[key];
					}
					else {
						tc.x[ key ] = params[key];
					}
		    	}
		    }
		    
		    var elementToAdd = JSON.stringify(tc);

			this.add(elementToAdd);
			WidgetManager.onPushTrack(fromQueue);
			if ( WidgetManager.isDebug() ) {
				if ( tc.i.is ) {
					var identities = [];
					identities.push( { identity : tc.i.i, 'type' : tc.i.is } );
					if ( tc.i.identities ) {
						for (var identityType in tc.i.identities) {
				     		identities.push( { identity : tc.i.identities[identityType], 'type' : identityType } );
						}
					}
				} 
				jQuery(document).trigger("identitiesAdded", identities);
			}
		}
	};
	
	var WidgetAPI = {
		_init : false,
		init : function( params ) {
			if ( this._init ) {
				return;
			}
			WidgetManager.onInit(); 
			if ( params ) {
				Log.debug("Init with params : "+JSON.stringify(params));
			    for (var key in params) {
					if ( key == 'tcptid' ) {
						WidgetQueue._tcptid = params[key];
					}
					else if ( key == 'tcttid' ) {
						WidgetQueue._tcttid = params[key];
					}
					else if ( key == 'tclv' ) {
						WidgetQueue._tclv = params[key];
					}
					else if ( key == 'tcla' ) {
						WidgetQueue._tcla = params[key];
					}
					else if ( key == 'tcep' ) {
						WidgetQueue._tcep = params[key];
					}
					else if (key == "tcnu" ) {
						WidgetQueue._tcnu = params[key];
					}
			    }
			}
			this._init = true;
		},
		pushTrack : function(params, fromQueue) {
			WidgetQueue.pushTrack( params, fromQueue );
		},
		onLoad : function(callback) {
			WidgetManager.setOnLoadCallback(callback);
		},
		setDebug : function(debugOn) {
		    WidgetManager.setDebug( debugOn );
		}
	};

	
//	try {
		var urlPrefix = ('https:' == document.location.protocol ? 'https' : 'http');
		var dataStoreUrls = {
			"tcaction" : urlPrefix + "://" + "localhost:8080/tcaction"
		};
		
		// ADD_WIDGETS

		// END_ADD_WIDGETS
		
		// ADD_EXCLUSIONS

		// END_ADD_EXCLUSIONS
		
		var styleSheetUrls = [];
		var styleSheetsLoaded = [];
		var newTCWQLoaded = false;
		var origCssNum = document.styleSheets.length;
		var onStyleSheetTimer = setInterval(function() {
			if ( document.styleSheets.length > origCssNum) {
				for ( var j = origCssNum; j < document.styleSheets.length; j++ ) {
					onStyleSheetLoad( document.styleSheets[j].href );
				}
			}
		}, 100 );
		var onStyleSheetLoad = function( url ) {
			if ( jQuery.inArray( url, styleSheetsLoaded ) == -1 ) {
				styleSheetsLoaded.push( url );
			}
			if ( styleSheetsLoaded.length >= styleSheetUrls.length && !newTCWQLoaded ) {
				var newTCWQ = new CommandQueue(WidgetAPI);
				newTCWQ.processQueue( _tcwq );
				_tcwq = newTCWQ;
				newTCWQLoaded = true;
				clearInterval( onStyleSheetTimer );
			}
		};
		
		// ADD_WIDGET_STYLESHEETS
		
		// END_ADD_WIDGET_STYLESHEETS
		
		if ( styleSheetUrls.length == 0 ) {
			onStyleSheetLoad();
		}
		else {
			jQuery.each( styleSheetUrls, function() { 
				var url = this;
				var link = document.createElement("link");
				link.href = url;
				link.rel = "stylesheet";
				link.type = "text/css";
				link.onload = function() {
					onStyleSheetLoad( url );
				};
				if ( link.addEventListener ) {
					var event = "load";
					var eventCallback = function() {
						link.removeEventListener( event, eventCallback, false );
						onStyleSheetLoad( url );
					}
					link.addEventListener( event, eventCallback );
				}
				link.onreadystatechange = function() {
					if ( this.readState == 'loaded' || this.readyState == 'complete' ) {
						onStyleSheetLoad( url );
					}
				};
				var headElemList = document.getElementsByTagName("head");
				headElemList[0].appendChild( link );
				
			} );
		}
//	}
//	catch (e) {
	
//	}
	
})();
