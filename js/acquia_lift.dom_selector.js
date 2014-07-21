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
    defaults = {
      hoverCss: {
        background: '#666',
        opacity: .6,
        color: '#fff',
        cursor: 'pointer'
      },
      onElementSelect: function(element, selector) {
        console.log('selected: ' + selector);
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
      this.hovered.hoverCss = this.settings.hoverCss;
    },

    /**
     * Enables DOM watching capabilities.
     *
     * @returns the current jQuery element.
     */
    watchDOM: function() {
      this.$element.bind('mousemove', $.proxy(this, 'onMouseMove'));
      this.$element.bind('click', $.proxy(this, 'onClick'));
      this.$element.find('*').each(function() {
        $(this).qtip({
          content: getSelector(this),
          position: {
            corner: {
              target: 'topMiddle',
              tooltip: 'bottomMiddle'
            },
            adjust: {
              y: 10
            }
          },
          show: {
            delay: 0
          },
          hide: {
            effect: {
              type: null
            }
          }
        });
      });
      return this.$element;
    },

    /**
     * Disables DOM watching capabilities.
     *
     * @returns the current jQuery element.
     */
    unwatchDOM: function() {
      this.hovered.unhighlight();
      this.$element.unbind('mousemove', this.onMouseMove);
      this.$element.unbind('click', this.onClick);
      this.$element.find('*').each(function() {
        $(this).qtip('destroy');
      });
      return this.$element;
    },

    /**
     * Highlight functionality for a hovered element.
     */
    hovered: {
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
          this.$element.css(this.originalCss);
          console.log('hide qtip: ' + getSelector(this.$element[0]));
          this.$element.qtip("hide");
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
          this.$element.qtip("show");
          console.log('show qtip: ' + getSelector(this.$element[0]));
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
    onMouseMove: function(event) {
      this.hovered.update(event.target);
    },

    /**
     * Event listener for an element click event.
     */
    onClick: function(event) {
      this.settings.onElementSelect.call(this, this.hovered.$element[0], getSelector(this.hovered.$element[0]));
      event.preventDefault();
      event.stopPropagation();
      event.cancelBubble = true;
      this.unwatchDOM();
      return false;
    }
  })

  /**
   * Add DOMSelector to jQuery and protect against multiple instantiations.
   */
  $.fn.DOMSelector = function(options) {
    this.each(function() {
      if (!$.data(this, 'plugin_' + pluginName)) {
        $.data(this, 'plugin_' + pluginName, new Plugin(this, options));
      }
    });
    // Chain for jQuery functions.
    return this;
  }

  /**
   * Utility function to test if a string value empty.
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
