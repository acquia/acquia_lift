/**
 * A generic DOM selector that allows the selection of an element and triggers
 * an event upon selection.
 *
 * Inspired by https://github.com/conductrics/dom-selector
 */
;(function ($) {

  /**
   * jQuery plugin definition.
   */
  var pluginName = 'DOMSelector',
    indicatorClass = 'acquia-lift-active-element',
    qtipCreated = false,
    defaults = {
      hoverCss: {
        background: '#666',
        opacity: .6,
        color: '#fff',
        cursor: 'pointer'
      },
      onElementSelect: function (element, selector) {
        console.log('selected: ' + selector);
      },
      onError: function (message) {
        console.log(message);
      }
    };

  function Plugin (element, options) {
    this.$element = $(element);
    this.settings = $.extend({}, defaults, options);
    this._defaults = defaults;
    this._name = pluginName;
    this.init();
  }

  $.extend(Plugin.prototype, {
    /**
     * Initialization logic.
     */
    init: function() {
      this._hovered.hoverCss = this.settings.hoverCss;
    },

    /**
     * Enables DOM watching capabilities.
     *
     * @returns the current jQuery element.
     */
    startWatching: function() {
      this.$element.bind('mousemove', $.proxy(this, '_onMouseMove'));
      this.$element.bind('click', $.proxy(this, '_onClick'));
      if (qtipCreated) {
        this.$element.find('*').qtip('enable');
      } else {
        this.$element.find('*').each(function() {
          $(this).qtip({
            content: getSelector(this),
            solo: true,
            position: {
              target: 'mouse',
              adjust: {
                mouse: true
              }
            },
            // Let the show event remain at mouseover to allow for deferred
            // instantiation, but handle only showing when highlighted via the
            // beforeShow callback.
            show: {
              delay: 0,
              effect: {
                type: 'show',
                length: 0
              }
            },
            hide: {
              delay: 0,
              when: false,
              effect: {
                type: 'hide',
                length: 0
              }
            },
            api: {
              beforeShow: function() {
                return this.elements.target.hasClass(indicatorClass);
              }
            }
          });
        });
      }
      return this.$element;
    },

    /**
     * Disables DOM watching capabilities.
     *
     * @returns the current jQuery element.
     */
    stopWatching: function() {
      this._hovered.unhighlight();
      this.$element.unbind('mousemove', this._onMouseMove);
      this.$element.unbind('click', this._onClick);
      this.$element.find('*').qtip('disable');
      return this.$element;
    },

    /**
     * Highlight functionality for a hovered element.
     */
    _hovered: {
      // The element that is currently hovered.
      $element: null,
      // The background css for the hovered element so that it can be returned
      // to its original state after hover.
      originalCss: {},
      // The CSS for the element on hover.
      hoverCss: {},

      // Unhighlight the element.
      unhighlight: function() {
        if (this.$element != null) {
          this.$element.qtip("hide");
          this.$element.css(this.originalCss);
          this.$element.removeClass(indicatorClass);
        }
        this.originalCss = {};
        return this.$element = null;
      },

      // Highlight the element.
      highlight: function() {
        if (this.$element != null) {
          for (var prop in this.hoverCss) {
            this.originalCss[prop] = this.$element.css(prop);
          }
          this.$element.addClass(indicatorClass);
          this.$element.qtip("show");
          return this.$element.css(this.hoverCss);
        }
      },

      // Update what element is the current for hover functionality.
      update: function(target) {
        if (target === null || typeof target === 'undefined' || (this.$element && target === this.$element[0])) {
          return;
        }
        this.unhighlight();
        this.$element = $(target);
        this.highlight();
      }
    },

    /**
     * Event listener for a mouse move event.
     *
     * Update the hover element to the current element being moused over.
     */
    _onMouseMove: function(event) {
      this._hovered.update(event.target);
    },

    /**
     * Event listener for an element click event.
     */
    _onClick: function(event) {
      var $selected = this._hovered.$element;
      if ($selected.length != 1) {
        this.settings.onError.call(this, Drupal.t('Invalid element selector.'));
      } else {
        // Remove the indicator class so it doesn't show in the final selector.
        $selected.removeClass(indicatorClass);
        this.settings.onElementSelect.call(this, $selected[0], getSelector($selected[0]));
      }
      event.preventDefault();
      event.stopPropagation();
      event.cancelBubble = true;
      this.stopWatching();
      return false;
    }
  })

  /**
   * Add DOMSelector to jQuery and protect against multiple instantiations.
   */
  $.fn.DOMSelector = function (options) {
    var args = arguments;
    // Create a new plugin.
    if (options === undefined || typeof options === 'object') {
      return this.each(function () {
        // Only allow the plugin to be instantiated once.
        if (!$.data(this, 'plugin_' + pluginName)) {
          $.data(this, 'plugin_' + pluginName, new Plugin(this, options));
        }
      });
      // If the first parameter is a string and it doesn't start
      // with an underscore and isn't the init function,
      // treat this as a call to a public method.
    } else if (typeof options === 'string' && options[0] !== '_' && options !== 'init') {
      var returns;

      this.each(function () {
        var instance = $.data(this, 'plugin_' + pluginName);
        if (instance instanceof Plugin && typeof instance[options] === 'function') {
          returns = instance[options].apply( instance, Array.prototype.slice.call( args, 1 ) );
        }

        // Allow instances to be destroyed via the 'destroy' method
        if (options === 'destroy') {
          $.data(this, 'plugin_' + pluginName, null);
        }
      });

      return returns !== undefined ? returns : this;
    }
  };


  /**
   * Utility function to test if a string value is empty.
   *
   * @param stringValue
   *   String value to test
   * @returns {boolean}
   *   True if not empty, false if null or empty.
   */
  function notEmpty(stringValue) {
    return (stringValue != null ? stringValue.length : void 0) > 0;
  };

  /**
   * Determines the element selector for a child element based on element
   * name.

   * @param element
   *   The element to use for determination
   * @returns string
   *   The selector string to use
   */
  function nthChild(element) {
    if (element == null || element.ownerDocument == null || element === document || element === document.body || element === document.head) {
      return "";
    }
    var parent = element.parentNode || null;
    if (parent) {
      var nthStack = [];
      var num = parent.childNodes.length;
      for (var i = 0; i < num; i++) {
        var nthName = parent.childNodes[i].nodeName.toLowerCase();
        if (nthName === "#text") {
          continue;
        }
        nthStack.push(nthName);
        if (parent.childNodes[i] === element) {
          if (nthStack.length > 1) {
            nthStack[0] += ":first-child";
          }
          return nthStack.join(" + ");
        }
      }
    }
    return element.nodeName.toLowerCase();
  }

  /**
   * Determines the unique selector for an element.
   */
  function getSelector(element) {
    var hasId = notEmpty(element.id),
      hasClass = notEmpty(element.className),
      isElement = element.nodeType === 1,
      isRoot = element.parentNode === element.ownerDocument,
      hasParent = element.parentNode != null,
      selector = '';

    if (!isRoot && isElement) {
      if (hasId) {
        selector = '#' + element.id;
      } else if (hasClass) {
        selector = "." + element.className.split(" ").join(".").replace(/\.$/, '');
      } else {
        selector = nthChild(element);
      }
    }

    if (hasId) {
      return selector;
    } else if (hasParent) {
      return getSelector(element.parentNode) + " > " + selector;
    }
    return selector;
  }

  /**
   * Determine whether a node contains a target node.
   *
   * @param node
   *   The node to check as a possible container of the target.
   * @param target
   *   The target node to check if contained within node.
   * @returns {boolean}
   *   True if target is within node, and false if not.
   */
  function nodeContains(node, target) {
    for (var child in node.childNodes) {
      if (child === target || nodeContains(child, target)) {
        return true;
      }
    }
    return false;
  };

})(jQuery);
