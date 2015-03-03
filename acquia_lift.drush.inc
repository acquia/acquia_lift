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
  $items['acquia-lift-campaign-sync'] = array (
    'description' => 'Sync all or one Lift campaign(s).',
    'aliases' => array('al-sync'),
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

/**
 * Syncs the Acquia Lift Campaigns.
 *
 * This command is an alternative to the UI's "Sync with Lift" buttons.
 */
function drush_acquia_lift_campaign_sync() {
  module_load_include('inc', 'acquia_lift', 'acquia_lift.batch');

  // Step 1. Select sync type.
  $options = array(
    'all' => dt('Sync all campaigns.'),
    'one' => dt('Sync one campaign by its machine name.')
  );
  $type_selection = drush_choice($options, dt('Select sync type:'));
  if (!$type_selection) {
    return;
  }
  $agents = array();
  foreach (acquia_lift_get_agent_types() as $type => $info) {
    $agents = array_merge($agents, personalize_agent_load_by_type($type));
  }

  // Step 2a. Select a campaign (if the type is not "all").
  if ($type_selection == 'one') {
    $agent_names = array_keys($agents);
    $name_selection = drush_choice($agent_names, dt('Select a campaign machine name to sync:'));
    if (!$name_selection) {
      return;
    }
    $agent_name = $agent_names[$name_selection];
    $agent = $agents[$agent_name];
    $agents = array($agent_name => $agent);
  }

  // Step 3. Sync selected campaign(s)
  acquia_lift_batch_sync_campaigns($agents);
  $agent_names = array_keys($agents);
  drush_log('The campaign(s) '. implode(', ', $agent_names). ' have been synchronized with Lift.', 'success');
}
