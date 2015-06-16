var TC_CONF = TC_CONF || {
	// TC_CONF_SECTION
"maxQueueCookieSize":4093,"activityLevel":"FULL","trackingTime":63072000000,"thirdPartyCookie":false

		
	// END_TC_CONF_SECTION
};

var _byteToHex = [];
for (var i = 0; i < 256; i++) {
  _byteToHex[i] = (i + 0x100).toString(16).substr(1);
}

// internal widget queue
var _tcwq = _tcwq || [];

// singleton and namespace
var TC = TC || (function() {

	"use strict";

	//
	// TCT
	//
	var TCT = {
		_version: "1.5.0-SNAPSHOT-272-4-g0c9ef71-dirty",
		
		version: function(){
			return this._version;
		},
	
		collector: function() {
			return "localhost:8080/TCT/tc";
		},
		
		generator: function() {
			return "tc-local.trucentric.com:8080/TCT/tc3ptidgen";
		},
		
		doNotTrackController: function() {
			return "${capture.doNotTrackControllerURL}";
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
			
		},
		
		"Base16ToBase62": function (val) {
			var base16Alphabet = "0123456789abcdef";
			var base16 = base16Alphabet.length;
			var decimalValue = 0;
			var multiplier = 1;
			while (val.length > 0) {
				var digit = val.charAt(val.length - 1);
				decimalValue += multiplier * base16Alphabet.indexOf(digit);
				val = val.substring(0, val.length - 1);
				multiplier *= base16;
			}
			
			var base62Alphabet = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
			var base62 = base62Alphabet.length;
			var base62Val = "";
			while (decimalValue >= 1) {
				var digitPosition = decimalValue % base62;
				var digit = base62Alphabet.charAt(digitPosition);
				base62Val = digit + base62Val;
				decimalValue /= base62;
			}
			return base62Val;			
		},		

		"Hash": function (val) {
			var add32 = function(a, b) {
				return (a + b) & 0xFFFFFFFF;
			}
			function md5cycle(x, k) {
				var a = x[0], b = x[1], c = x[2], d = x[3];
				
				a = ff(a, b, c, d, k[0], 7, -680876936);
				d = ff(d, a, b, c, k[1], 12, -389564586);
				c = ff(c, d, a, b, k[2], 17,  606105819);
				b = ff(b, c, d, a, k[3], 22, -1044525330);
				a = ff(a, b, c, d, k[4], 7, -176418897);
				d = ff(d, a, b, c, k[5], 12,  1200080426);
				c = ff(c, d, a, b, k[6], 17, -1473231341);
				b = ff(b, c, d, a, k[7], 22, -45705983);
				a = ff(a, b, c, d, k[8], 7,  1770035416);
				d = ff(d, a, b, c, k[9], 12, -1958414417);
				c = ff(c, d, a, b, k[10], 17, -42063);
				b = ff(b, c, d, a, k[11], 22, -1990404162);
				a = ff(a, b, c, d, k[12], 7,  1804603682);
				d = ff(d, a, b, c, k[13], 12, -40341101);
				c = ff(c, d, a, b, k[14], 17, -1502002290);
				b = ff(b, c, d, a, k[15], 22,  1236535329);
				
				a = gg(a, b, c, d, k[1], 5, -165796510);
				d = gg(d, a, b, c, k[6], 9, -1069501632);
				c = gg(c, d, a, b, k[11], 14,  643717713);
				b = gg(b, c, d, a, k[0], 20, -373897302);
				a = gg(a, b, c, d, k[5], 5, -701558691);
				d = gg(d, a, b, c, k[10], 9,  38016083);
				c = gg(c, d, a, b, k[15], 14, -660478335);
				b = gg(b, c, d, a, k[4], 20, -405537848);
				a = gg(a, b, c, d, k[9], 5,  568446438);
				d = gg(d, a, b, c, k[14], 9, -1019803690);
				c = gg(c, d, a, b, k[3], 14, -187363961);
				b = gg(b, c, d, a, k[8], 20,  1163531501);
				a = gg(a, b, c, d, k[13], 5, -1444681467);
				d = gg(d, a, b, c, k[2], 9, -51403784);
				c = gg(c, d, a, b, k[7], 14,  1735328473);
				b = gg(b, c, d, a, k[12], 20, -1926607734);
				
				a = hh(a, b, c, d, k[5], 4, -378558);
				d = hh(d, a, b, c, k[8], 11, -2022574463);
				c = hh(c, d, a, b, k[11], 16,  1839030562);
				b = hh(b, c, d, a, k[14], 23, -35309556);
				a = hh(a, b, c, d, k[1], 4, -1530992060);
				d = hh(d, a, b, c, k[4], 11,  1272893353);
				c = hh(c, d, a, b, k[7], 16, -155497632);
				b = hh(b, c, d, a, k[10], 23, -1094730640);
				a = hh(a, b, c, d, k[13], 4,  681279174);
				d = hh(d, a, b, c, k[0], 11, -358537222);
				c = hh(c, d, a, b, k[3], 16, -722521979);
				b = hh(b, c, d, a, k[6], 23,  76029189);
				a = hh(a, b, c, d, k[9], 4, -640364487);
				d = hh(d, a, b, c, k[12], 11, -421815835);
				c = hh(c, d, a, b, k[15], 16,  530742520);
				b = hh(b, c, d, a, k[2], 23, -995338651);
				
				a = ii(a, b, c, d, k[0], 6, -198630844);
				d = ii(d, a, b, c, k[7], 10,  1126891415);
				c = ii(c, d, a, b, k[14], 15, -1416354905);
				b = ii(b, c, d, a, k[5], 21, -57434055);
				a = ii(a, b, c, d, k[12], 6,  1700485571);
				d = ii(d, a, b, c, k[3], 10, -1894986606);
				c = ii(c, d, a, b, k[10], 15, -1051523);
				b = ii(b, c, d, a, k[1], 21, -2054922799);
				a = ii(a, b, c, d, k[8], 6,  1873313359);
				d = ii(d, a, b, c, k[15], 10, -30611744);
				c = ii(c, d, a, b, k[6], 15, -1560198380);
				b = ii(b, c, d, a, k[13], 21,  1309151649);
				a = ii(a, b, c, d, k[4], 6, -145523070);
				d = ii(d, a, b, c, k[11], 10, -1120210379);
				c = ii(c, d, a, b, k[2], 15,  718787259);
				b = ii(b, c, d, a, k[9], 21, -343485551);
				
				x[0] = add32(a, x[0]);
				x[1] = add32(b, x[1]);
				x[2] = add32(c, x[2]);
				x[3] = add32(d, x[3]);

			}

			function cmn(q, a, b, x, s, t) {
				a = add32(add32(a, q), add32(x, t));
				return add32((a << s) | (a >>> (32 - s)), b);
			}

			function ff(a, b, c, d, x, s, t) {
				return cmn((b & c) | ((~b) & d), a, b, x, s, t);
			}

			function gg(a, b, c, d, x, s, t) {
				return cmn((b & d) | (c & (~d)), a, b, x, s, t);
			}

			function hh(a, b, c, d, x, s, t) {
				return cmn(b ^ c ^ d, a, b, x, s, t);
			}

			function ii(a, b, c, d, x, s, t) {
				return cmn(c ^ (b | (~d)), a, b, x, s, t);
			}

			function md51(s) {
				var txt = '';
				var n = s.length,
				state = [1732584193, -271733879, -1732584194, 271733878], i;
				for (i=64; i<=s.length; i+=64) {
					md5cycle(state, md5blk(s.substring(i-64, i)));
				}
				s = s.substring(i-64);
				var tail = [0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0];
				for (i=0; i<s.length; i++)
					tail[i>>2] |= s.charCodeAt(i) << ((i%4) << 3);
				tail[i>>2] |= 0x80 << ((i%4) << 3);
				if (i > 55) {
					md5cycle(state, tail);
					for (i=0; i<16; i++) tail[i] = 0;
				}
				tail[14] = n*8;
				md5cycle(state, tail);
				return state;
			}

			/* there needs to be support for Unicode here,
			 * unless we pretend that we can redefine the MD-5
			 * algorithm for multi-byte characters (perhaps
			 * by adding every four 16-bit characters and
			 * shortening the sum to 32 bits). Otherwise
			 * I suggest performing MD-5 as if every character
			 * was two bytes--e.g., 0040 0025 = @%--but then
			 * how will an ordinary MD-5 sum be matched?
			 * There is no way to standardize text to something
			 * like UTF-8 before transformation; speed cost is
			 * utterly prohibitive. The JavaScript standard
			 * itself needs to look at this: it should start
			 * providing access to strings as preformed UTF-8
			 * 8-bit unsigned value arrays.
			 */
			function md5blk(s) { /* I figured global was faster.   */
				var md5blks = [], i; /* Andy King said do it this way. */
				for (i=0; i<64; i+=4) {
					md5blks[i>>2] = s.charCodeAt(i)
					+ (s.charCodeAt(i+1) << 8)
					+ (s.charCodeAt(i+2) << 16)
					+ (s.charCodeAt(i+3) << 24);
				}
				return md5blks;
			}

			var hex_chr = '0123456789abcdef'.split('');

			function rhex(n)
			{
				var s='', j=0;
				for(; j<4; j++)
				s += hex_chr[(n >> (j * 8 + 4)) & 0x0F]
				+ hex_chr[(n >> (j * 8)) & 0x0F];
				return s;
			}

			function hex(x) {
				for (var i=0; i<x.length; i++)
				x[i] = rhex(x[i]);
				return x.join('');
			}

			function md5(s) {
				return hex(md51(s));
			}

			/* this function is much faster,
			so if possible we use it. Some IEs
			are the only ones I know of that
			need the idiotic second function,
			generated by an if clause.  */

			if (md5('hello') != '5d41402abc4b2a76b9719d911017c592') {
				add32 = function(x, y) {
					var lsw = (x & 0xFFFF) + (y & 0xFFFF),
					msw = (x >> 16) + (y >> 16) + (lsw >> 16);
					return (msw << 16) | (lsw & 0xFFFF);
				}
			}
			
			return md5(val);
		}
	};
	
	

	//
	// ID
	//
	var ID = {

		create: function() {
			var millisBy16 = new Date().getTime() % 16;
			var uuid;

			if ((window.crypto && window.crypto.getRandomValues) || (window.msCrypto && window.msCrypto.getRandomValues)) {
				var buffer = new Uint8Array(16);
				if (window.crypto && window.crypto.getRandomValues) {
					window.crypto.getRandomValues(buffer);
				}
				else {
					window.msCrypto.getRandomValues(buffer);
				}
				buffer[6] = (buffer[6] & 0x0f) | 0x40;
				buffer[8] = (buffer[8] & 0x3f) | 0x80;

				var i = 0;
				var bth = _byteToHex;
				uuid = bth[buffer[i++]] + bth[buffer[i++]] +
			            bth[buffer[i++]] + bth[buffer[i++]] + '-' +
			            bth[buffer[i++]] + bth[buffer[i++]] + '-' +
			            bth[buffer[i++]] + bth[buffer[i++]] + '-' +
			            bth[buffer[i++]] + bth[buffer[i++]] + '-' +
			            bth[buffer[i++]] + bth[buffer[i++]] +
			            bth[buffer[i++]] + bth[buffer[i++]] +
			            bth[buffer[i++]] + bth[buffer[i++]];
			}
			else {
				uuid = 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replace(/[xy]/g, function(c) {
					var r = ((Math.random()*16 + millisBy16)/2)|0, v = c == 'x' ? r : (r&0x3|0x8);
					return v.toString(16);
				});
			}

			var uuidValue = uuid.replace(/-/g, '');
			var id = Lang.Base16ToBase62(uuidValue)
			
			return id;
		}

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
		"delete": function() {
			this.value = Lang.String.EMPTY;
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


	
	//
	// Browser Object Model
	//
	var BOM = {

		addEventListener: function(target, type, handler) {

			if (target.addEventListener) {
				target.addEventListener(type, handler, false);
				return true;
			}
			if (target.attachEvent) {
				return target.attachEvent('on' + type, handler);
			}
			target['on' + type] = handler;
		},

		findById: function(id) {
			return document.getElementById(id);
		},
		
		isDocument: function(o) {
			var isIt = (o === document);
			return isIt;
		},

		isAnchor: function(o) {
			var isIt = (o.constructor === HTMLAnchorElement);
			return isIt;
		},

		getURL: function(element) {
			var url = '';
			if (this.isDocument(element)) {
				url = element.URL;
			}
			else if (this.isAnchor(element)) {
				url = element.href;
			};
			return url.replace(/\#.*/, '');
		},
		
		getReferrer: function(element) {
			var url = '';
			if (this.isDocument(element)) {
				url = element.referrer;
			}
			else if (this.isAnchor(element)) {
				url = element.baseURI;
			};
			return url.replace(/\#.*/, '');
		},

		getProtocol: function() {
			return document.location.protocol;
		},

		getTrackableFormElements: function(form) {
			var values = {};
			var elements = form.elements;
			var elementLength = elements.length;
			for (var elementIndex = 0; elementIndex < elementLength; elementIndex++) {
				var element = elements[elementIndex];
				var name = element.name || element.id;
				if ( name ) {

					switch(element.type) {
						case "text":
							values[name] = element.value;
							break;
						case "select-one":
						case "select-multiple":
							var selectedOptionIndex = element.options.selectedIndex;
							if ( selectedOptionIndex > -1 )
							{
								// TODO - only one option will be sent back, in the multi-select selects
								values[name] = element.options[selectedOptionIndex].value;
							}
							break;
						case "radio":
							if (element.checked) {
								values[name] = element.value;
							}
							break;
						case "checkbox":
							// TODO - only one checkbox will be sent back
							if (element.checked) {
								values[name] = element.value;
							}
							break;
						default:
							// pass on other form elements
							break;
					}
				}
			}
			return values;
		},
		
		getMeta: function() {
			var metaElements = new Object();
			if ( document.all ) {
				metaElements = document.all.tags('META');
			}
			if ( document.getElementsByTagName ) {
				metaElements = document.getElementsByTagName ('META');
			}
			var meta= new Object();
			for (var m = 0; m < metaElements.length; m++) {
				var metaName = metaElements[m].getAttribute("name") || metaElements[m].getAttribute("http-equiv") || metaElements[m].getAttribute("property") || metaElements[m].getAttribute("itemProp");
				if ( metaName ) {
					meta[metaName] = metaElements[m].content; 
				}
			}
			return meta;
		},

		getTitle: function(element) {
			return element.title;
		},
		
		getPlatform: function() {
			if ( navigator ) {
				return navigator.platform;
			}
			return "";
		},

		getValue: function (nodeId) {
			var value = null;
			var startsWith = nodeId.charAt(0);
			if (startsWith === '#') {
				var node = document.getElementById(nodeId.replace("#", ""));
				if ( node !== null ) {
					value = node.value;	
				}
			}
			else {
				value = nodeId;
			}
			return value;
		},
		
		isSupported: function() {
			var supported = true;
			
			if (/MSIE (\d+\.\d+);/.test(navigator.userAgent)) {
				var ieVersion=new Number(RegExp.$1);
				if (ieVersion <= 7) {
					supported = false;
				}
			}
			
			return supported;
		}

	};


	
	//
	// CommandQueue
	//
	/**
	 * @param {Array} aq queue
	 */
	function CommandQueue(aq) {
		// first deal with backlog on the queue
		var backlog = Queue.getAll();
		for ( var backlogIndex = 0; backlogIndex < backlog.length; backlogIndex++ ) {
			var url = backlog[backlogIndex];
			Tracker._sendTrack(url);
			
			if ( Tracker._widgetListener ) {
				var paramList = url.split("&");
				var params = {};
				for ( var paramIndex = 0; paramIndex < paramList.length; paramIndex++ ) {
					var param = paramList[paramIndex];
					var paramNameAndValue = param.split("=");
					params[paramNameAndValue[0]] = decodeURIComponent(paramNameAndValue[1]);
				}
				Tracker._widgetListener( params, true );
			}
		}
		
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
				if ( Config.isTrackEnabled() || method.substr( 0, 'set'.length ) === 'set' ) {
					API[method].apply( API, parameterList ); // it's a method name
				}
			}
			else {
				method.apply( API, parameterList ); // it's a function - TODO - why do we need this?
			}

		};
	};

	
	/**
	 * Queue - saves the records in the cookie (internal queue) for later processing
	 */
	var Queue = {

		/**
		 * cookie that stores the queue information
		 * 
		 * @private
		 * @field
		 */
		_cookie: new Cookie("tc_q"),
		_elementCache : [],
		_isElementsCached : false,

		/**
		 * save elements on the queue
		 * 
		 * @private
		 * @param {Array} elements to save on the queue
		 */
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
				while (this._cookie.newSize(UniversalID.expires(), elements.join("|")) > maxQueueCookieSize) {
					elements.splice(0, 1);
				}

				if (!elements || elements.length == 0) {
					this._cookie.setValue("");
				}
				else {
					this._cookie.setValue(elements.join("|"));
				}
			}
			this._cookie.setExpires(UniversalID.expires()).save();
		},

		
		/**
		 * encode the string element for the queue
		 * 
		 * @private
		 * @param {element} string element to encode as it would be saved on the queue
		 */
		_encode: function(element) {
			return element.replace(/ /g, "+").replace(/\|/g, "%7C");	
		},
		

		/**
		 * gets all the elements from the queue, but does not remove them from the queue
		 * 
		 * @returns {Array} array of elements from the queue
		 */
		getAll: function() {
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



		/**
		 * gets next element from the queue, but does not remove it from the queue
		 * 
		 * @returns {Object} next element from the queue
		 */
		getNext: function() {
			var elements = getAll();
			if (elements.length > 0) {
				return elements[0]
			}
			else {
				return;
			}
		},
		/**
		 * adds element to the queue
		 * 
		 * @param {Object} o element to add to the queue
		 */
		add: function( o ) {
			if ( !o ) { // no value provided, return
				return;
			}
			var elements = this.getAll();
			for ( var inel = 0; inel < elements.length; inel++) {
				if (o == elements[inel]) {
					return;
				}
			}
			o = this._encode(o);
			elements.push(o);
			this._save(elements);
		},

		
		/**
		 * removes element from the queue
		 * 
		 * @param {Object} o element to remove from the queue
		 */
		remove: function( o ) {
			if ( !o ) { // no value provided, return
				return;
			}
			var oldElements = this.getAll();
			if ( !oldElements || oldElements.length === 0 ) { // queue empty, nothing to remove
				return;
			}
			else {
				var newElements = [];
				o = this._encode(o);
				for ( var inel = 0; inel < oldElements.length; inel++) {
					if (o != oldElements[inel]) {
						newElements.push(oldElements[inel]); // put it back on the queue
					}
				}
				this._save(newElements);
			}
		}


	};
	
	
	
	//
	// UniversalID
	//
	var UniversalID = {
			
			_cookie: new Cookie("tc_ptid"),
			_userCookie: new Cookie("tc_ptuid"),
			_newUserCookie: new Cookie("tc_nu"),
			_expiryCookie: new Cookie("tc_ptidexpiry"),
			
			set: function() {
				var createID = true;
				if ( this._cookie.exists() ) {
					this._cookie.load();
					if ( this.value() != Lang.String.EMPTY ) {
						createID = false;
					}
				}
				if ( this._userCookie.exists() ) {
					this._userCookie.load();
				}
				
				if ( createID ) {
					this._cookie.setValue( ID.create() );
					this._cookie.setExpires( this.expires() );
					this._cookie.save();

					// Create a new user cookie					
					this._newUserCookie.setExpires( 30 * 60 * 1000 ); // 30 minutes (in milliseconds)
					this._newUserCookie.save();
				}
			},
			expires: function() {
				if ( this._expiryCookie.exists() ) {
					this._expiryCookie.load();
				}
				else {
					this._expiryCookie.setValue( new Date().getTime() + TC_CONF.trackingTime ); // User controlled time setting
					this._expiryCookie.setExpires( TC_CONF.trackingTime ); 
					this._expiryCookie.save();
				}
				
				var expiryDate = new Date();
				expiryDate.setTime(this._expiryCookie.getValue());
				return expiryDate.getTime() - new Date().getTime();
				
			},
			value: function() {
				// Only use user cookie if userIdentitySourceInTrackingId is defined.
				if (this._userCookie.exists() && Lang.isDefined(TC_CONF) && Lang.isDefined(TC_CONF.userIdentitySourceInTrackingId)) {
					return this._cookie.getValue() + this._userCookie.getValue();
				}
				else {
					return this._cookie.getValue();
				}
			},
			
			setUser: function(identitySource, identity) {
				// On first identification, do no store the identity. This will ensure the base tracking id is associated to the person
				// On following identifications, store the identity. The new tracking id will be associated to the person.
				if (this._userCookie.exists()) {
					this._userCookie.setValue( Lang.Base16ToBase62( Lang.Hash(identitySource + "=" + identity) ) );
				}
				else {
					this._userCookie.setValue('');
				}

				this._userCookie.setExpires( this.expires() ); // Current Universal ID expiration

				this._userCookie.save();
			},
			exists: function() {
				if ( this._cookie.exists() ) {
					this._cookie.load();
					if ( this.value() != Lang.String.EMPTY ) {
						return true;
					}
				}
				return false;
			},
						
			isNewUser: function() {
				return this._newUserCookie.exists();
			},
			
			shouldLoadThirdParty: function() {
				return Config.isThirdPartyCookieEnabled() && SessionID.exists() == false;
			},
			
			loadThirdParty: function() {
				var tcptidExists = this.exists();
				var minAPI = {
					setAccount: function(accountId) {
						this.accountId = accountId;
					}
				};
				for ( var cmdI = 0; cmdI < _tcaq.length; cmdI++ ) {
					var i, method;
					var parameterList = _tcaq[cmdI].slice();
					method = parameterList.shift();
					if ( Lang.isString( method ) ) {
						if ( minAPI[method] != null ) {
							minAPI[method].apply( minAPI, parameterList ); // it's a method name
						}
					}
					else {
						method.apply( minAPI, parameterList ); // it's a function - TODO - why do we need this?
					}	
				}
				
				if ( minAPI.accountId ) {
					document.tcSetThirdParty = function( tc3ptid, doNotTrack, expiryTimeSeconds ) {
						UniversalID._cookie.setValue( tc3ptid );
						UniversalID._cookie.setExpires( expiryTimeSeconds * 1000 ); // Need to convert from seconds to milliseconds
						UniversalID._cookie.save();
						
						// Set up first-party tc_dnt cookie if the 3rd party one exists, else delete it
						if (doNotTrack) {
							DoNotTrack.set( doNotTrack );
						} else {
							DoNotTrack["delete"]();
						}

						if ( tcptidExists == false ) {
							// Create a new user cookie
							UniversalID._newUserCookie.setExpires( 30 * 60 * 1000 ); // 30 minutes (in milliseconds)
							UniversalID._newUserCookie.save();
						}
						document.tcSetThirdParty = null;
						_tcaq = new CommandQueue(_tcaq);
					};
					var scriptUrl = ('https:' == document.location.protocol ? 'https' : 'http') + '://'+ TCT.generator() + "?tcla="+minAPI.accountId + "&ttl=" + TC_CONF.trackingTime/1000 ;
					if ( this.exists() ) {
						this.set();
						scriptUrl += "&tcptid="+this.value();
					}
					scriptUrl += "&callback=document.tcSetThirdParty&tclrnd="+Math.round(2147483647 * Math.random());
					var script = document.createElement("script");
					script.type = "text/javascript";
					script.src = scriptUrl;
					var x = document.getElementsByTagName('script')[0];
					x.parentNode.insertBefore(script, x);						
				}
				else {
					this.set();
					_tcaq = new CommandQueue( _tcaq );
				}
							
			}
		};

	
	//
	// SessionID
	//
	var SessionID = {
		_cookie: new Cookie("tc_ttid"),
		_newUserCookie: new Cookie("tc_nu"),

		set: function() {
			var createID = true;
			if ( this._cookie.exists() ) {
				this._cookie.load();
				if ( this.value() != Lang.String.EMPTY ) {
					createID = false;
				}
			}
			
			if ( createID ) {
				this._cookie.setValue( ID.create() );
			}
			this._cookie.setExpires( 30 * 60 * 1000 ); // 30 minutes (in milliseconds)
			this._cookie.save();
			
			// The new user cookie should stay active as long as the session is active
			if(this._newUserCookie.exists()) {
				this._newUserCookie.setExpires( 30 * 60 * 1000 ); // 30 minutes (in milliseconds)
				this._newUserCookie.save();
			}
			
		},
		exists: function() {
			if ( this._cookie.exists() ) {
				this._cookie.load();
				if ( this.value() != Lang.String.EMPTY ) {
					return true;
				}
			}
			return false;
		},
		value: function() {
			return this._cookie.getValue();
		}
		
	};

	var DoNotTrack = {
		_cookie: new Cookie("tc_dnt"),

		set: function(value) {
			if ( this._cookie.exists() ) {
				this._cookie.load();
			}
			this._cookie.setValue( value );
			this._cookie.setExpires( 10 * 365 * 24 * 60 * 60 * 1000 );
			this._cookie.save();
		},
		exists: function() {
			if ( this._cookie.exists() ) {
				this._cookie.load();
				if ( this.value() != Lang.String.EMPTY ) {
					return true;
				}
			}
			return false;
		},
		value: function() {
			return this._cookie.getValue();
		},
		"delete": function() {
			return this._cookie["delete"]();
		},
		doNotTrack: function() {
			if ( this._cookie.exists() ) {
				this._cookie.load();
				if ( this.value() === 'true' ) {
					return true;
				}
			}
			return false;
		}
	};


	//
	// config
	//
	function Config(){};
	Config.setAccount = function( account ) {
		Config.account = account;
		if ( Config._widgetListener ) {
			Config._widgetListener( [ account ] );
		}		
	};
	
	Config.isSegmentAPIType = function() {
		var segmentAPIFlag = Lang.isDefined(TC_CONF) && (TC_CONF.segmentAPIType === 'SEGMENT');
		return segmentAPIFlag;
	};
	
	Config.isActivityLevelCapture = function() {
		var activeByScriptConf = Lang.isDefined(TC_CONF) && (TC_CONF.activityLevel === 'CAPTUREONLY' || TC_CONF.activityLevel === 'FULL');
		var activeCookieConf = new Cookie("TC_CONF.activityLevel");
		activeCookieConf.load();
		var activeByCookieConf = activeCookieConf.getValue() === 'CAPTUREONLY' || activeCookieConf.getValue() === 'FULL'; 
		var activeByClientConf = !activeByScriptConf && activeByCookieConf;
		
		var active = activeByScriptConf || activeByClientConf;
		return active;
	};

	Config.isActivityLevelFull = function() {
		var activeByScriptConf = Lang.isDefined(TC_CONF) && TC_CONF.activityLevel === 'FULL';
		var activeCookieConf = new Cookie("TC_CONF.activityLevel");
		activeCookieConf.load();
		var activeByCookieConf = activeCookieConf.getValue() === 'FULL'; 
		var activeByClientConf = !activeByScriptConf && activeByCookieConf;

		var active = activeByScriptConf || activeByClientConf;
		return active;
	};

	Config.setListener = function( callback ) {
		Config._widgetListener = callback;
	};

	Config.isTrackEnabled = function() {
	
		var cookieEnabled = (navigator.cookieEnabled) ? true : false;
		
		if (typeof navigator.cookieEnabled === "undefined" && !cookieEnabled) { 
			document.cookie = "tc_trackenabledtest=true";
			cookieEnabled = (document.cookie.indexOf("tc_trackenabledtest") != -1) ? true : false;
			document.cookie = "tc_trackenabledtest=;max-age=0";
		}
		return cookieEnabled && !(DoNotTrack.doNotTrack());
	};
	Config.isThirdPartyCookieEnabled = function() {
		return Lang.isDefined(TC_CONF) && TC_CONF.thirdPartyCookie == true;
	};

	//
	// tracker
	//
	function Tracker(){};
	Tracker.trackView = function( category, params, appendedParams ) {
		if ( !params ) {
			params = {};
		}
		params.tcvc = category;
		Tracker._track(params, appendedParams);
	};

	Tracker.track = function(category, params, appendedParams) {
		if ( !params ) {
			params = {};
		}
		params.tcvc = category;
		Tracker._track(params, appendedParams);
	};

	Tracker._createCaptureID = function() {
		return ID.create();
	};
	
	Tracker._track = function(params, appendedParams) {

		if (Lang.isUndefined(params.tcvv)) {
			params.tcvv = BOM.getURL(document);
		}
		if (Lang.isUndefined(params.tcvr)) {
			params.tcvr = BOM.getReferrer(document);
		}
		if (Lang.isUndefined(params.tcvt)) {
			params.tcvt = BOM.getTitle(document);
		}
		if (Lang.isUndefined(params.tcvc)) {
			params.tcvc = Lang.String.EMPTY;
		}

		params.tcptid = UniversalID.value();
		params.tcttid = SessionID.value();
		params.tcctid = Tracker._createCaptureID();
		params.tclv = TCT.version();
		params.tclrnd = Math.round(2147483647 * Math.random());
		params.tcla = Config.account;
		params.tcect = Lang.Date.now();
		params.tcep = BOM.getPlatform();

		var metas = BOM.getMeta();
		for (var metaName in metas) {
			var includeMetaName = true;
			if ( TC_CONF.includedMetaNames ) {
				includeMetaName = false;
			    for (var mI = 0; mI < TC_CONF.includedMetaNames.length; mI++) {
			        if (TC_CONF.includedMetaNames[mI] == metaName.toLowerCase()) {
			        	includeMetaName = true;
			        	break;
			        }
			    }
			}
			if ( includeMetaName ) {
				var trackingMetaName = "tcm" + metaName;
				params[trackingMetaName] = metas[metaName];
			}
		}

		if (Lang.isDefined(appendedParams)) {
			var identity = appendedParams['identity'];
			for (var identityName in identity) {
				var identityValue = BOM.getValue(identityName);
				if (!Lang.isNull(identityValue)) {
					var hash = false;
					//Hash identity if identity source in list
					if ( TC_CONF.identifierTypesHashed ) {
						for (var i = 0; i < TC_CONF.identifierTypesHashed.length; i++ ) {
							var identifierTypeHashed =  TC_CONF.identifierTypesHashed[i];
							if (identifierTypeHashed === identity[identityName]) {
								hash = true;
							}
						}
					}
					var trackingIdentityValue = "tci" + identityValue;
					if (hash) {
						trackingIdentityValue = "tci" + Lang.Hash(identityValue);
					}
					params[trackingIdentityValue] = identity[identityName];
				}
			}
		}
		
		var queryString = "";
		for (var paramName in params) {
			queryString += "&" + paramName + "=" + encodeURIComponent(params[paramName]);
		}
		queryString = queryString.substring(1, queryString.length); // strip first & - not needed
		this._sendTrack(queryString);
		if ( this._widgetListener ) {
			this._widgetListener( params );
		}
	};

	Tracker.setListener = function( callback ) {
		this._widgetListener = callback;
	};
	/**
	 * sends the actual image request to the server with all the information
	 */
	Tracker._sendTrack = function(queryString) {
		var url = BOM.getProtocol() + "//" + TCT.collector() + "?" + queryString;
		var image = new Image(1,1);
		Queue.add(queryString);
		BOM.addEventListener( image, "load", function(e) {
			if ( image ) {
				Queue.remove(queryString);
			}
		} );
		BOM.addEventListener( image, "error", function(e) {
			// fail fast, handle case where there is an error on the server
			// by removing the queryString so that it doesn't queue up and
			// cause problems on the customer site
			Queue.remove(queryString); 
		} );
		image.src = url;
	};

	Tracker.trackClick = function(e, category, params, appendedParams) {
		var event = e || window.event;

		if ( !params ) {
			params = {};
		}
		params.tcvv = BOM.getURL(event.target);
		params.tcvr = BOM.getReferrer(event.target);
		params.tcvc = category;
		params.tcvt = BOM.getTitle(event.target);
		
		Tracker._track(params, appendedParams);
	};
	
	Tracker.trackSubmit = function(e, category, params, appendedParams) {
		var event = e || window.event;

		if ( !params ) {
			params = {};
		}
		
		Lang.Object.merge(params, BOM.getTrackableFormElements(event.target));

		params.tcvc = category;
		Tracker._track(params, appendedParams);
	};

	Tracker.trackIdentity = function(identity, identitySource, params, appendedParams) {
		if ( !params ) {
			params = {};
		}
		var hash = false;
		//Hash identity if identity source in list
		if ( TC_CONF.identifierTypesHashed ) {
			for (var i = 0; i < TC_CONF.identifierTypesHashed.length; i++ ) {
				var identifierTypeHashed =  TC_CONF.identifierTypesHashed[i];
				if (identifierTypeHashed === identitySource) {
					hash = true;
				}
			}
		}
		if (hash) {
			params.tcii = Lang.Hash(identity);
		} else {
			params.tcii = identity;
		}
		params.tciis = identitySource;
		
		if (Lang.isDefined(TC_CONF) && (TC_CONF.userIdentitySourceInTrackingId === identitySource)) {
			var oldValue = UniversalID.value();
			UniversalID.setUser(identitySource, identity);
			if (UniversalID.value() === oldValue) {
				params.tciic = true;
			}
			else {
				params.tciic = false;
			}
		}
		else {
			params.tciic = true;

		}
		
		Tracker._track(params, appendedParams);
	};

	Tracker.updatePerson = function(params,appendedParams) {
		if ( !params ) {
			params = {};
		}
		
		params.tcptidc = true;
		
		Tracker._track(params, appendedParams);
	}
	
	Tracker.setDoNotTrack = function(value) { // value is a boolean
		Tracker.updatePerson({'person_doNotTrack':value});
		if ( value ) {
			DoNotTrack.set( value );
		} else {
			DoNotTrack["delete"]();
		}
		
		if (Config.isThirdPartyCookieEnabled()) {
			document.tcSetDoNotTrack = function( doNotTrack ) {
				if (doNotTrack) {
					DoNotTrack.set( doNotTrack );
				} else {
					DoNotTrack["delete"]();
				}
				document.tcSetDoNotTrack = null;
			};
			
			var scriptUrl = ('https:' == document.location.protocol ? 'https' : 'http') + '://'+ TCT.doNotTrackController() + "?tcla="+ Config.account;
			scriptUrl += "&callback=document.tcSetDoNotTrack" + "&doNotTrack=" + value;
			var script = document.createElement("script");
			script.type = "text/javascript";
			script.src = scriptUrl;
			var x = document.getElementsByTagName('script')[0];
			x.parentNode.insertBefore(script, x);
		}
	}
	//
	// api
	//
	var API = {

		setAccount: Config.setAccount,

		captureView: Tracker.trackView,
		
		captureClick: function(elementId, category, params, appendedParams) {
			var element = BOM.findById(elementId);
			BOM.addEventListener(element, 'click', function(e){ Tracker.trackClick(e, category, params, appendedParams); });
		},

		captureSubmit: function(elementId, category, params, appendedParams) {
			var element = BOM.findById(elementId);
			BOM.addEventListener(element, 'submit', function(e){ Tracker.trackSubmit(e, category, params, appendedParams); });
		},

		captureIdentity: Tracker.trackIdentity,

		capture: Tracker.track,
		
		updatePerson: Tracker.updatePerson,
		
		setDoNotTrack: Tracker.setDoNotTrack

	};
	
	//
	// init tag
	//
	try	{
		
		var jsFileToLoad = null;
		if ( Config.isActivityLevelCapture() ) {
			jsFileToLoad = 'tc-widget.js';

			var loadThirdParty = UniversalID.shouldLoadThirdParty();
			if ( !loadThirdParty ) {
				UniversalID.set();
			}
			SessionID.set();

			if ( BOM.isSupported() && Config.isActivityLevelFull()) {
				// Widget listener integration
				Config.setListener( function ( params ) {
					_tcwq.unshift( [ 'init',
						 { 'tcptid' : UniversalID.value(), 'tcttid' : SessionID.value(), 'tclv' : TCT.version(), 'tcla' : Config.account, 'tcep' : BOM.getPlatform(), 'tcnu' : UniversalID.isNewUser() } ] );
					// Load the widget tag here
			        var s = document.createElement('script');
			        s.type = 'text/javascript';
			        s.async = true;
			        s.src = ('https:' == document.location.protocol ? 'https' : 'http') + '://'+'localhost/~chris.nagy/cdn/'+Config.account+'/'+jsFileToLoad;
			        var x = document.getElementsByTagName('script')[0];
			        x.parentNode.insertBefore(s, x);	
				} );
							 
				Tracker.setListener( function ( params, fromQueue ) { 
					_tcwq.push( [ "pushTrack", params, (fromQueue ? fromQueue : false) ] );
				} );
			}
			
			if ( loadThirdParty ) {
				UniversalID.loadThirdParty();		
			}
			else {
				_tcaq = new CommandQueue(_tcaq);
			}
		}
	}
	catch (e) {
		if (window.console && window.console.log) {
			window.console.log(e);
		}
	}

	
})();

