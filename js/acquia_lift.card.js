/**
 * @file
 * Javascript functionality for elemental-card.
 *
 * @group component
 */

(function ($) {
  'use strict';

  // CARD PUBLIC CLASS DEFINITION
  // ================================

  var Card = function (element, options) {
    this.type =
    this.options =
    this.enabled =
    this.$element = null;

    this.init('card', element, options);
  };

  Card.DEFAULTS = {
    collapsible: true,
    collapsed: false,
    footerVisible: true,
    eventExpanded: 'el-card-expanded',
    eventCollapsed: 'el-card-collapsed'
  };

  Card.prototype.init = function (type, element, options) {
    this.type = type;
    this.$element = $(element);
    this.options = this.getOptions(options);
    this.enabled = true;

    this.render();
  };

  Card.prototype.enable = function () {
    this.enabled = true;
  };

  Card.prototype.disable = function () {
    this.enabled = false;
  };

  /**
   * Read a card data attribute from the element.
   */
  Card.prototype.dataAttr = function(key) {
    var value = this.$element.attr('data-' + this.type + '-' + key);
    if (value === undefined) {
      return undefined;
    }
    // All values will be strings by default.
    // Convert them to native booleans or numbers if appropriate.
    if (value === "true" || value === "false") {
      return value === "true";
    } else if (!isNaN(Number(value))) {
      return Number(value);
    }
    return value;
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
    this.hide().$element.off('.' + this.type).removeData('bs.' + this.type);
  };

  Card.prototype.render = function () {
    var $label, $details, detailsId, that = this;
    if (!this.options.footerVisible) {
      this.getFooter().hide();
    }
    if (this.options.collapsible) {
      // Add a collapse control
      $details = this.getDetails();
      // Generate an ID to use with WAI-ARIA if none exists.
      detailsId = $details.attr('id') || 'el-card-details-' + Date.now();
      $details.attr('id', detailsId);
      $label = this.getHeaderLabel();
      $label.attr('aria-controls', detailsId);
      // Toggle opening details when clicking the card label.
      $label.on('click', function (e) {
        that.toggleOpen();
      });
      this.setOpen(!this.options.collapsed);
    } else {
      this.setOpen(true);
    }
  };

  /**
   * Toggle whether the main content area is open or closed.
   *
   * @param bool open
   *   Indicates if the content area should be forced open or closed.
   *   If not passed then the element is simply toggled.
   */
  Card.prototype.toggleOpen = function (open) {
    var isOpen = this.isOpen(),
      $content = this.getDetails(),
      that = this;

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
      $content.hide();
      $content.slideDown('fast', function () {
        that.setOpen(true);
        that.$element.removeClass('is-transitioning');
        that.$element.trigger(that.options.eventExpanded);
      });
    } else {
      $content.slideUp('fast', function () {
        that.setOpen(false);
        that.$element.removeClass('is-transitioning');
        that.$element.trigger(that.options.eventCollapsed);
      });
    }
  };

  Card.prototype.setOpen = function (open) {
    if (open) {
      this.$element.addClass('is-active');
    } else {
      this.$element.removeClass('is-active');
    }
    this.getDetails().attr('aria-expanded', open);
  };

  Card.prototype.isOpen = function() {
    return this.$element.hasClass('is-active');
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
      if (!data && option === 'destroy') {
        return;
      }
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
