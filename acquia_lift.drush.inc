<?php
/**
* @file acquia_lift.drush.inc
 * Provides Acquia Lift drush commands.
*/

/**
 * Implements hook_drush_command().
 */
function acquia_lift_drush_command() {
  $items['acquia-lift-menu-rebuild'] = array (
    'description' => 'Rebuild the menu items and classes',
    'aliases' => array('al-menu'),
  );
  return $items;
}

/**
 * Rebuilds the Drupal menu items.
 *
 * If any menu items get removed from the database, the Lift unified toolbar is
 * not able to properly display or wire up the backbone application
 * functionality.
 *
 * This rebuilds the menu so that it is as the JavaScript code expects.
 */
function drush_acquia_lift_menu_rebuild() {
  module_load_install('acquia_lift');
  // Delete the existing menu.
  menu_delete_links('acquia-lift-controls');
  // Re-generate the menu correctly.
  _acquia_lift_build_menu('acquia-lift-controls');
  drush_log('The menu has been rebuilt.', 'success');
}
