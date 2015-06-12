/**
 * @file
 * Javascript functionality for elemental-card.
 *
 * @group card
 */

(function ($, undefined) {
  'use strict';

  // CARD PUBLIC CLASS DEFINITION
  // ================================

  var Card = function (element, options) {
    this.type =
      this.options =
        this.$element = null;

    this.init('card', element, options);
  };

  Card.DEFAULTS = {
    collapsible: true,
    collapsed: false,
    sortable: false,
    footerVisible: true,
    enabled: true,
    eventExpanded: 'card-expanded',
    eventCollapsed: 'card-collapsed'
  };

  Card.prototype.init = function (type, element, options) {
    this.type = type;
    this.$element = $(element);
    this.options = this.getOptions(options);

    if (this.options.enabled) {
      this.enable();
    } else {
      this.disable();
    }

    this.render();
  };

  Card.prototype.enable = function () {
    this.enabled = true;
    this.$element.removeClass('is-disabled');
  };

  Card.prototype.disable = function () {
    this.enabled = false;
    this.$element.addClass('is-disabled');
  };

  /**
   * Read a card data attribute from the element.
   */
  Card.prototype.dataAttr = function(key) {
    var dataKey = this.type + key.charAt(0).toUpperCase() + key.slice(1);
    var oldKey = dataKey.replace(/[A-Z]/g, function(a) {return '-' + a.toLowerCase()});

    return this.$element.data(dataKey) || this.$element.data(oldKey);
  };

  Card.prototype.getDefaults = function () {
    return Card.DEFAULTS;
  };

  Card.prototype.getOptions = function (options) {
    var i, dataValue;
    options = $.extend({}, this.getDefaults(), options);
    // Look for option in data attributes like data-card-[key].
    for (i in options) {
      if (options.hasOwnProperty(i)) {
        dataValue = this.dataAttr(i);
        if (dataValue !== undefined) {
          options[i] = dataValue;
        }
      }
    }
    return options;
  };

  Card.prototype.destroy = function () {
    this.$element.unbind('.' + this.type).removeData('el.' + this.type);
  };

  Card.prototype.render = function () {
    var $label, $details, detailsId;
    var self = this;
    this.getFooter().toggle(this.options.footerVisible);
    if (this.options.collapsible) {
      // Add a collapse control
      $details = this.getDetails();
      // Generate an ID to use with WAI-ARIA if none exists.
      detailsId = $details.attr('id') || 'el-card-details-' + Date.now();
      $details.attr('id', detailsId);
      $label = this.getHeaderLabel();
      $label.attr('aria-controls', detailsId);
      // Toggle opening details when clicking the card label.
      $label.bind('click.card', function (e) {
        self.toggleOpen();
      });
      this.setOpen(!this.options.collapsed);
    } else {
      this.setOpen(true);
    }
    this.$element.toggleClass('is-sortable', this.options.sortable);
  };

  /**
   * Toggle whether the main content area is open or closed.
   *
   * @param bool open
   *   Indicates if the content area should be forced open or closed.
   *   If not passed then the element is simply toggled.
   */
  Card.prototype.toggleOpen = function (open) {
    var isOpen = this.isOpen();
    var $content = this.getDetails();
    var self = this;

    // Set a value for the toggle.
    open = open === undefined ? !isOpen : open;

    // Not collapsible
    if (!this.options.collapsible) {
      return;
    }
    // No change needed if the card is already in the desired state.
    if (open === isOpen) {
      return;
    }

    // Allow the last action to finish.
    if (this.$element.hasClass('is-transitioning')) {
      return;
    }
    this.$element.addClass('is-transitioning');
    if (open) {
      self.setOpen(true);
      self.$element.removeClass('is-transitioning');
      self.$element.trigger(self.options.eventExpanded);
    } else {
      self.setOpen(false);
      self.$element.removeClass('is-transitioning');
      self.$element.trigger(self.options.eventCollapsed);
    }
  };

  /**
   * Sets the final open state values on the element.
   *
   * @param boolean open
   *   True if the final state should be expanded, false if collapsed.
   */
  Card.prototype.setOpen = function (open) {
    var $details = this.getDetails();
    if (open) {
      this.$element.removeClass('is-compact');
    } else {
      this.$element.addClass('is-compact');
    }
    $details.attr('aria-expanded', open);
  };

  /**
   * Returns whether the element is currently expanded (true) or collapsed.
   */
  Card.prototype.isOpen = function() {
    return !this.$element.hasClass('is-compact');
  };

  /**
   * Helper functions to return the main components of the card.
   */
  Card.prototype.getHeader = function () {
    return this.$element.find('.el-card__header');
  };

  Card.prototype.getHeaderLabel = function () {
    return this.getHeader().find('.el-card__title');
  };

  Card.prototype.getDetails = function () {
    return this.$element.find('.el-card__content');
  };

  Card.prototype.getFooter = function () {
    return this.$element.find('.el-card__footer');
  };

  // CARD PLUGIN DEFINITION
  // =========================

  var old = $.fn.card;

  $.fn.card = function (option) {
    return this.each(function () {
      var $this = $(this),
        data = $this.data('el.card'),
        options = typeof option === 'object' && option;
      if (!data) {
        $this.data('el.card', (data = new Card(this, options)));
      }
      if (typeof option === 'string') {
        data[option]();
      }
    });
  };

  $.fn.card.Constructor = Card;

  // Card NO CONFLICT
  // ===================

  $.fn.card.noConflict = function () {
    $.fn.card = old;
    return this;
  };

})(jQuery);

/**
 * @file acquia_lift.cards.js  Initialization and Acquia Lift specific handling
 * of cards.
 */

(function ($, Drupal) {
  'use strict';

  Drupal.behaviors.acquiaLiftCards = {
    attach: function (context, settings) {
      $('.el-card').once('acquia-lift-card', function() {
        $(this).card();
      });
    }
  }

})(jQuery, Drupal);

//# sourceMappingURL=acquia_lift.card.js.map