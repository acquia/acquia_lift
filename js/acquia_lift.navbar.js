/**
 * @file
 * personalize_ui.navbar.js
 */

(function (Drupal, $) {

"use strict";

Drupal.behaviors.acquiaLiftNavbarMenu = {
  attach: function (context) {
    if ('drupalNavbarMenu' in $.fn) {
      $('.navbar-menu-acquia-lift-controls')
        .children('.menu')
        .drupalNavbarMenu({
          twisties: false,
          findItem: function ($list, $menu) {
            var $items = $list.children('li');
            var $wrappedItems = $list.children().not('li').children('li');
            $items = $items.add($wrappedItems);
            if ($items.length) {
              return $items;
            }
          },
          findItemElement: function ($item, $menu) {
            var $campaigns = $item.children('div').children('a.acquia-lift-campaign')
            var $contentVariations = $item.children('.acquia-lift-preview-option-set').children('span');
            var $handle = $campaigns.add($contentVariations);
            if ($handle.length) {
              return $handle;
            }
          },
          findItemSubMenu: function ($item, $menu) {
            var $subMenus = $item.find('ul');
            var $wrappedSubMenus = $item.children().not('ul').children('ul');
            $subMenus = $subMenus.add($wrappedSubMenus);
            if ($subMenus.length) {
              return $subMenus;
            }
          }
        });
    }
    // Hide object counts in vertical navbar try orientation.
    var $onceler = $('body').once('acquia-lift-navbar');
    if ($onceler.length) {
      $(document).on('drupalNavbarOrientationChange.acquiaLift', function (event, orientation) {
        var hide = (orientation === 'horizontal') ? false : true;
        $('.acquia-lift-personalize-type-count').toggleClass('acquia-lift-hidden', hide);
      });
    }
  }
};

}(Drupal, jQuery));
