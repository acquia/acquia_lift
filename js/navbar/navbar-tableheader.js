/**
 * @file
 * Provide displacement information sticky tableheaders.
 */

(function ($, Drupal, displace) {

"use strict";

Drupal.navbar = Drupal.navbar || {};

$.extend(Drupal.navbar, {
  height: function () {
    return displace().top;
  }
});

}(Drupal.jQuery, Drupal, Drupal.displace));
