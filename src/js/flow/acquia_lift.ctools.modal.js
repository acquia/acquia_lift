/**
 * @file Override CTools modal.js in order to provide dynamic sizing
 * capabilities.  Whenever possible, the original functionality is preserved.
 *
 * These capabilities are being added to CTools and at that time this will no
 * longer be necessary.
 *
 * @see  https://www.drupal.org/node/1294478
 */

(function ($, Drupal) {
  // Make sure our objects are defined.
  Drupal.CTools = Drupal.CTools || {};
  Drupal.CTools.Modal = Drupal.CTools.Modal || {};

  /**
   * Display the modal
   */
  Drupal.CTools.Modal.show = function(choice) {
    var opts = {};

    if (choice && typeof choice == 'string' && Drupal.settings[choice]) {
      // This notation guarantees we are actually copying it.
      $.extend(true, opts, Drupal.settings[choice]);
    }
    else if (choice) {
      $.extend(true, opts, choice);
    }

    var defaults = {
      modalTheme: 'CToolsModalDialog',
      throbberTheme: 'CToolsModalThrobber',
      animation: 'show',
      animationSpeed: 'fast',
      modalSize: {
        type: 'scale',
        width: .8,
        height: .8,
        addWidth: 0,
        addHeight: 0,
        // How much to remove from the inner content to make space for the
        // theming.
        contentRight: 25,
        contentBottom: 45
      },
      modalOptions: {
        opacity: .55,
        background: '#fff'
      }
    };

    var settings = {};
    $.extend(true, settings, defaults, Drupal.settings.CToolsModal, opts);

    if (Drupal.CTools.Modal.currentSettings && Drupal.CTools.Modal.currentSettings != settings) {
      Drupal.CTools.Modal.modal.remove();
      Drupal.CTools.Modal.modal = null;
    }

    Drupal.CTools.Modal.currentSettings = settings;

    if (!Drupal.CTools.Modal.modal) {
      Drupal.CTools.Modal.modal = $(Drupal.theme(settings.modalTheme));
    }

    $('#modal-title', Drupal.CTools.Modal.modal).html(Drupal.CTools.Modal.currentSettings.loadingText);
    Drupal.CTools.Modal.modalContent(Drupal.CTools.Modal.modal, settings.modalOptions, settings.animation, settings.animationSpeed);
    $('#modal-content').html(Drupal.theme(settings.throbberTheme));

    $(window).trigger('resize');

    // Position autocomplete results based on the scroll position of the modal.
    $('#modal-content').delegate('input.form-autocomplete', 'keyup', function() {
      $('#autocomplete').css('top', $(this).position().top + $(this).outerHeight() + $(this).offsetParent().filter('#modal-content').scrollTop());
    });
  };

  // The following are implementations of AJAX responder commands.

  /**
   * AJAX responder command to place HTML within the modal.
   */
  var ctoolsModalDisplay = Drupal.CTools.Modal.modal_display;
  Drupal.CTools.Modal.modal_display = function(ajax, response, status) {
    ctoolsModalDisplay(ajax, response, status);
    // Trigger a resize event to make sure modal is in the right place.
    $(window).trigger('resize');
  }

  // CTools.Modal.modalContent inner-scoped functions.
  // Get a list of the tabbable elements in the modal content.
  var getTabbableElements = function () {
    var tabbableElements = $('#modalContent :tabbable'),
      radioButtons = tabbableElements.filter('input[type="radio"]');

    // The list of tabbable elements from jQuery is *almost* right. The
    // exception is with groups of radio buttons. The list from jQuery will
    // include all radio buttons, when in fact, only the selected radio button
    // is tabbable, and if no radio buttons in a group are selected, then only
    // the first is tabbable.
    if (radioButtons.length > 0) {
      // First, build up an index of which groups have an item selected or not.
      var anySelected = {};
      radioButtons.each(function () {
        var name = this.name;

        if (typeof anySelected[name] === 'undefined') {
          anySelected[name] = radioButtons.filter('input[name="' + name + '"]:checked').length !== 0;
        }
      });

      // Next filter out the radio buttons that aren't really tabbable.
      var found = {};
      tabbableElements = tabbableElements.filter(function () {
        var keep = true;

        if (this.type == 'radio') {
          if (anySelected[this.name]) {
            // Only keep the selected one.
            keep = this.checked;
          }
          else {
            // Only keep the first one.
            if (found[this.name]) {
              keep = false;
            }
            found[this.name] = true;
          }
        }

        return keep;
      });
    }

    return tabbableElements.get();
  };

  // Keyboard and focus event handler ensures only modal elements gain focus.
  var modalEventHandler = function( event ) {
    var target = null;
    if ( event ) { //Mozilla
      target = event.target;
    } else { //IE
      event = window.event;
      target = event.srcElement;
    }

    var parents = $(target).parents().get();
    for (var i = 0; i < parents.length; ++i) {
      var position = $(parents[i]).css('position');
      if (position == 'absolute' || position == 'fixed') {
        return true;
      }
    }

    if ($(target).is('#modalContent, body') || $(target).filter('*:visible').parents('#modalContent').length) {
      // Allow the event only if target is a visible child node
      // of #modalContent.
      return true;
    }
    else {
      getTabbableElements()[0].focus();
    }

    event.preventDefault();
  };

  // Keypress handler Ensures you can only TAB to elements within the modal.
  // Based on the pseudo-code from WAI-ARIA 1.0 Authoring Practices section
  // 3.3.1 "Trapping Focus".
  var modalTabTrapHandler = function (evt) {
    // We only care about the TAB key.
    if (evt.which != 9) {
      return true;
    }

    var tabbableElements = getTabbableElements(),
      firstTabbableElement = tabbableElements[0],
      lastTabbableElement = tabbableElements[tabbableElements.length - 1],
      singleTabbableElement = firstTabbableElement == lastTabbableElement,
      node = evt.target;

    // If this is the first element and the user wants to go backwards, then
    // jump to the last element.
    if (node == firstTabbableElement && evt.shiftKey) {
      if (!singleTabbableElement) {
        lastTabbableElement.focus();
      }
      return false;
    }
    // If this is the last element and the user wants to go forwards, then
    // jump to the first element.
    else if (node == lastTabbableElement && !evt.shiftKey) {
      if (!singleTabbableElement) {
        firstTabbableElement.focus();
      }
      return false;
    }
    // If this element isn't in the dialog at all, then jump to the first
    // or last element to get the user into the game.
    else if ($.inArray(node, tabbableElements) == -1) {
      // Make sure the node isn't in another modal (ie. WYSIWYG modal).
      var parents = $(node).parents().get();
      for (var i = 0; i < parents.length; ++i) {
        var position = $(parents[i]).css('position');
        if (position == 'absolute' || position == 'fixed') {
          return true;
        }
      }

      if (evt.shiftKey) {
        lastTabbableElement.focus();
      }
      else {
        firstTabbableElement.focus();
      }
    }
  };

  var setSize = function(context, winWidth, winHeight) {
    var width = 0;
    var height = 0;

    if (Drupal.CTools.Modal.currentSettings.modalSize.type === 'scale') {
      width = $(window).width() * Drupal.CTools.Modal.currentSettings.modalSize.width;
      height = $(window).height() * Drupal.CTools.Modal.currentSettings.modalSize.height;
    }
    else {
      width = Drupal.CTools.Modal.currentSettings.modalSize.width;
      height = Drupal.CTools.Modal.currentSettings.modalSize.height;
    }

    if (Drupal.CTools.Modal.currentSettings.modalSize.type === 'dynamic') {
      // Use the additional pixels for creating the width and height.
      $('div.ctools-modal-content', context).css({
        'min-width': Drupal.CTools.Modal.currentSettings.modalSize.width,
        'min-height': Drupal.CTools.Modal.currentSettings.modalSize.height,
        'width': 'auto',
        'height': 'auto',
        'max-height': (winHeight / 2) * 1.8 + 'px',
        'max-width': (winWidth / 2) * 1.8 + 'px',
        'overflow': 'auto'
      });
      $('#modalContent').css({'width': 'auto'});
      $('div.ctools-modal-content .modal-content', context).css("overflow", "visible");
    }
    else {
      // Use the additional pixels for creating the width and height.
      $('div.ctools-modal-content', context).css({
        'width': width + Drupal.CTools.Modal.currentSettings.modalSize.addWidth + 'px',
        'height': height + Drupal.CTools.Modal.currentSettings.modalSize.addHeight + 'px'
      });
      $('#modalContent', context).css({
        'width': width + Drupal.CTools.Modal.currentSettings.modalSize.addWidth + 'px',
        'height': height + Drupal.CTools.Modal.currentSettings.modalSize.addHeight + 'px'
      });
      $('div.ctools-modal-content .modal-content', context).css({
        'width': (width - Drupal.CTools.Modal.currentSettings.modalSize.contentRight) + 'px',
        'height': (height - Drupal.CTools.Modal.currentSettings.modalSize.contentBottom) + 'px'
      });
    }
  };

  // Move and resize the modalBackdrop and modalContent on window resize.
  var modalContentResize = function(e) {
    // When creating the modal, it actually exists only in a theoretical
    // place that is not in the DOM. But once the modal exists, it is in the
    // DOM so the context must be set appropriately.
    var context = e ? document : Drupal.CTools.Modal.modal;

    // Reset the backdrop height/width to get accurate document size.
    $('#modalBackdrop').css('height', '').css('width', '');

    // Get our heights
    var docHeight = $(document).height();
    var docWidth = $(document).width();
    var winHeight = $(window).height();
    var winWidth = $(window).width();
    var bodyWidth = $('body').width();
    if( winWidth > bodyWidth ) winWidth = bodyWidth;
    if( docHeight < winHeight ) docHeight = winHeight;

    setSize(context, winWidth, winHeight);

    // Get where we should move content to
    var modalContent = $('#modalContent');

    var height = Math.max(modalContent.outerHeight(), $('div.ctools-modal-content', context).outerHeight());
    var width = Math.max(modalContent.outerWidth(), $('div.ctools-modal-content', context).outerWidth());

    var mdcTop = Math.max($(document).scrollTop() + ( winHeight / 2 ) - (  height / 2), 10);
    var mdcLeft = Math.max(( winWidth / 2 ) - ( width / 2), 10);

    // Apply attributes to fix the position of the modal relative to current
    // position of page. This is required when the modal is larger than the
    // browser window. This enables the modal to scroll with the rest of the
    // page, rather than remaining centered in the page whilst scrolling.
    if (height > $(window).height()) {
      if (e.type === 'resize') {
        // Is a resize event so get the position of top relative to current
        // position of document in browser window.
        mdcTop = 10 + $(document).scrollTop();
      }
      else if (e.type === 'scroll') {
        // Is a scroll event so maintain to current position of the modal
        // relative to page.
        var modalOffSet = modalContent.offset();
        mdcTop = modalOffSet.y;
      }
    }

    // Apply the changes
    $('#modalBackdrop').css({'height': winHeight + 'px', 'width': winWidth + 'px', 'top': $(document).scrollTop()}).show();
    modalContent.css('top', mdcTop + 'px').css('left', mdcLeft + 'px').show();
  };

  var oldFocus = null;
  var currentAnimation = 'show';
  var currentSpeed = 'fast';

  // Close the open modal content and backdrop
  function close() {
    // Unbind the events
    $(window).unbind('resize',  modalContentResize);
    $('body').unbind( 'focus', modalEventHandler);
    $('body').unbind( 'keypress', modalEventHandler );
    $('body').unbind( 'keydown', modalTabTrapHandler );
    $('.close').unbind('click', modalContentClose);
    $('body').unbind('keypress', modalEventEscapeCloseHandler);
    $(document).trigger('CToolsDetachBehaviors', $('#modalContent'));

    // Set our animation parameters and use them
    var animation = 'hide';
    if ( currentAnimation == 'fadeIn' ) animation = 'fadeOut';
    if ( currentAnimation == 'slideDown' ) animation = 'slideUp';
    if ( currentAnimation == 'show' ) animation = 'hide';

    // Close the content
    $('#modalContent').hide()[animation](currentSpeed);

    // Remove the content
    $('#modalContent').remove();
    $('#modalBackdrop').remove();

    // Restore focus to where it was before opening the dialog
    $(oldFocus).focus();
  }

  var modalContentClose = function() {
    close();
    return false;
  };

  var modalEventEscapeCloseHandler = function(event) {
    if (event.keyCode == 27) {
      close();
      return false;
    }
  };

  /**
   * modalContent
   * @param content string to display in the content box
   * @param css obj of css attributes
   * @param animation (fadeIn, slideDown, show)
   * @param speed (valid animation speeds slow, medium, fast or # in ms)
   * @param modalClass class added to div#modalContent
   */
  Drupal.CTools.Modal.modalContent = function(content, css, animation, speed, modalClass) {
    // If our animation isn't set, make it just show/pop
    if (!animation) {
      animation = 'show';
    }
    else {
      // If our animation isn't "fadeIn" or "slideDown" then it always is show
      if (animation != 'fadeIn' && animation != 'slideDown') {
        animation = 'show';
      }
    }
    currentAnimation = animation;

    if (!speed) {
      speed = 'fast';
    }
    currentSpeed = speed;

    // Build our base attributes and allow them to be overridden
    css = jQuery.extend({
      position: 'absolute',
      left: '0px',
      margin: '0px',
      background: '#000',
      opacity: '.55'
    }, css);

    // Add opacity handling for IE.
    css.filter = 'alpha(opacity=' + (100 * css.opacity) + ')';
    content.hide();

    // If we already have modalContent, remove it.
    if ($('#modalBackdrop').length) $('#modalBackdrop').remove();
    if ($('#modalContent').length) $('#modalContent').remove();

    // Get our dimensions

    // Get the docHeight and (ugly hack) add 50 pixels to make sure we dont have a *visible* border below our div
    var docHeight = $(document).height() + 50;
    var docWidth = $(document).width();
    var winHeight = $(window).height();
    var winWidth = $(window).width();
    var bodyWidth = $('body').width();
    if( winWidth > bodyWidth ) winWidth = bodyWidth;
    if( docHeight < winHeight ) docHeight = winHeight;

    // Create our divs
    $('body').append('<div id="modalBackdrop" class="backdrop-' + modalClass + '" style="z-index: 1000; display: none;"></div><div id="modalContent" class="modal-' + modalClass + '" style="z-index: 1001; position: absolute;">' + $(content).html() + '</div>');

    setSize(document, winWidth, winHeight);

    $('body').bind( 'focus', modalEventHandler );
    $('body').bind( 'keypress', modalEventHandler );

    $('body').bind('keydown', modalTabTrapHandler);

    // Create our content div, get the dimensions, and hide it
    var modalContent = $('#modalContent').css('top','-1000px');
    var mdcTop = Math.max($(document).scrollTop() + ( winHeight / 2 ) - (  modalContent.outerHeight() / 2), 10);
    var mdcLeft = Math.max(( winWidth / 2 ) - ( modalContent.outerWidth() / 2), 10);
    $('#modalBackdrop').css(css).css('top', 0).css('height', docHeight + 'px').css('width', docWidth + 'px').show();
    modalContent.css({top: mdcTop + 'px', left: mdcLeft + 'px'}).hide()[animation](speed, function () { /* $(window).trigger('resize'); */ });

    // Bind a click for closing the modalContent
    $('.close').bind('click', modalContentClose);

    // Bind a keypress on escape for closing the modalContent
    $(document).bind('keydown', modalEventEscapeCloseHandler);

    // Per WAI-ARIA 1.0 Authoring Practices, initial focus should be on the
    // close button, but we should save the original focus to restore it after
    // the dialog is closed.
    oldFocus = document.activeElement;
    $('.close').focus();

    $(window).bind('resize', modalContentResize);
    $(window).bind('scroll', modalContentResize);
  };

  /**
   * unmodalContent
   * @param content (The jQuery object to remove)
   * @param animation (fadeOut, slideUp, show)
   * @param speed (valid animation speeds slow, medium, fast or # in ms)
   */
  Drupal.CTools.Modal.unmodalContent = function(content, animation, speed)
  {
    // If our animation isn't set, make it just show/pop
    if (!animation) { var animation = 'show'; } else {
      // If our animation isn't "fade" then it always is show
      if (( animation != 'fadeOut' ) && ( animation != 'slideUp')) animation = 'show';
    }
    // Set a speed if we dont have one
    if ( !speed ) var speed = 'fast';

    // Unbind the events we bound
    $(window).unbind('resize', modalContentResize);
    $(window).unbind('scroll', modalContentResize);
    $('body').unbind('focus', modalEventHandler);
    $('body').unbind('keypress', modalEventHandler);
    $('body').unbind( 'keydown', modalTabTrapHandler );
    $('.close').unbind('click', modalContentClose);
    $('body').unbind('keypress', modalEventEscapeCloseHandler);
    $(document).trigger('CToolsDetachBehaviors', $('#modalContent'));

    // jQuery magic loop through the instances and run the animations or removal.
    content.each(function(){
      if ( animation == 'fade' ) {
        $('#modalContent').fadeOut(speed, function() {
          $('#modalBackdrop').fadeOut(speed, function() {
            $(this).remove();
          });
          $(this).remove();
        });
      } else {
        if ( animation == 'slide' ) {
          $('#modalContent').slideUp(speed,function() {
            $('#modalBackdrop').slideUp(speed, function() {
              $(this).remove();
            });
            $(this).remove();
          });
        } else {
          $('#modalContent').remove();
          $('#modalBackdrop').remove();
        }
      }
    });
  };

  Drupal.ajax.prototype.commands.modal_display = Drupal.CTools.Modal.modal_display;
  Drupal.ajax.prototype.commands.modal_dismiss = Drupal.CTools.Modal.modal_dismiss;

})(Drupal.jQuery, Drupal);
