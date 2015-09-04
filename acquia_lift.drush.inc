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
    'description' => 'Sync all or one Lift personalization(s).',
    'aliases' => array('al-sync'),
  );
  $items['acquia-lift-enable-v2'] = array (
    'description' => 'Convert to the V2 decision back-end.',
    'aliases' => array('al-v2'),
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

  // Step 1. Exit if there is no campaign to be synchronized.
  $agents = array();
  foreach (acquia_lift_get_agent_types() as $type => $info) {
    if ($type == 'acquia_lift_target') {
      continue;
    }
    $agents = array_merge($agents, personalize_agent_load_by_type($type));
  }
  if (empty($agents)) {
    drush_log(dt('There is no personalization to be synchronized.'), 'success');
    return;
  }

  // Step 2. Select sync type.
  $options = array(
    'all' => dt('Sync all personalizations'),
    'one' => dt('Sync one personalization'),
  );
  $type_selection = drush_choice($options, dt('Select sync type:'));
  if (!$type_selection) {
    return;
  }

  // Step 3. Based on type, do further selections.
  $agent_names = array_keys($agents);
  switch ($type_selection) {
    // Step 3a. Select all campaigns.
    case 'all':
      $options = array(
        'yes' => dt('Confirm'),
      );
      $options_message = "\n" . dt('Confirm syncing all personalizations:'). "\n- " . implode("\n- ", $agent_names);
      $confirm_selection = drush_choice($options, $options_message);
      if (!$confirm_selection) {
        return;
      }
      break;
    // Step 3b. Select one campaign.
    case 'one':
      $options_message = "\n" . dt('Select a personalization machine name to sync:');
      $name_selection = drush_choice($agent_names, $options_message);
      if (!$name_selection) {
        return;
      }
      $agent_name = $agent_names[$name_selection];
      $agent = $agents[$agent_name];
      $agents = array($agent_name => $agent);
      break;
  }

  // Step 4. Sync selected campaign(s)
  // For each agent, set up a batch job including all the required save operations,
  // then process immediately.
  foreach ($agents as $agent_name => $agent) {
    $message = "\n" . dt('Synchronizing personalization ') . $agent_name . " ...";
    drush_print($message);
    $operations = acquia_lift_get_sync_operations_for_agent($agent);

    $batch_operations = array();
    foreach ($operations as $operation) {
      $batch_operations[] = array('acquia_lift_batch_process_item', array($operation));
    }
    $batch = array(
      'operations' => $batch_operations,
      'finished' => 'acquia_lift_drush_batch_finished',
      'file' => drupal_get_path('module', 'acquia_lift') . '/acquia_lift.batch.inc',
    );

    // Set and process.
    batch_set($batch);
    $batch =& batch_get();
    $batch['progressive'] = false;
    drush_backend_batch_process();
  }
}

function drush_acquia_lift_enable_v2() {
  // First ensure v2 is *not* enabled yet.
  variable_set('acquia_lift_v2_enabled', FALSE);
  $agents = personalize_agent_load_by_type('acquia_lift');
  $options = array(
    'yes' => dt('Confirm'),
  );
  $options_message = "\n" . dt('Confirm that you want to retire all existing V1 tests and switch to using V2:'). "\n- " . implode("\n- ", array_keys($agents));
  $confirm_selection = drush_choice($options, $options_message);
  if (!$confirm_selection) {
    return;
  }
  module_load_include('inc', 'acquia_lift', 'acquia_lift.admin');
  $parent_personalizations = personalize_agent_load_by_type('acquia_lift_target');
  // We'll create a mapping of parent names to test names.
  $parents = array();
  foreach ($parent_personalizations as $parent_name => $parent) {
    $nested = acquia_lift_get_nested_tests($parent);
    foreach ($nested as $test_name) {
      $parents[$test_name] = $parent_name;
    }
  }
  $account_info = acquia_lift_get_account_info();
  // Get an instance of the V1 API client to get the reports.
  $lift_api = AcquiaLiftAPIClientFactory::create($account_info, ACQUIA_LIFT_TESTING_AGENT_V1);
  // Go through each test, archive its report, and then retire it.
  $errors = 0;
  foreach ($agents as $name => $agent) {
    if (!isset($parents[$name])) {
      if (!empty($agent->data['lift_retired'])) {
        drush_print("Test $name already retired.");
      }
      else {
        // This test has no parent, must be legacy - just complete it.
        //personalize_agent_set_status($name, PERSONALIZE_STATUS_COMPLETED);
        drush_print("Legacy campaign $name status set to completed");
      }
      continue;
    }
    $option_sets = personalize_option_set_load_by_agent($name);
    if (empty($option_sets)) {
      continue;
    }
    $os = reset($option_sets);
    $parent_name = $parents[$name];
    // The goals will be on the parent personalization, not the test itself.
    $goals = personalize_goal_load_by_conditions(array('agent' => $parent_name));
    $goal_names = array();
    foreach ($goals as $goal) {
      $goal_names[] = $goal->action;
    }

    $archived = acquia_lift_archive_test_report($lift_api, $agent, $goal_names);
    if (!$archived) {
      drush_print("There was a problem archiving test $name. Skipping", 0, STDERR);
      $errors++;
      continue;
    }
    // We need to find which audience in the parent personalization this test
    // is running for.
    $parent_os = acquia_lift_get_option_set_for_targeting($parent_name);
    foreach ($parent_os->targeting as $audience => $targeting) {
      if (isset($targeting['osid']) && $targeting['osid'] == $os->osid) {
        $audience_name = $audience;
        break;
      }
    }
    if (isset($audience_name)) {
      drush_print("Stopping test for audience $audience_name of personalization $parent_name");
      acquia_lift_stop_test_for_audience($parent_personalizations[$parent_name], $audience_name);
    }
    else {
      drush_print("Could not find audience for $name, setting status to completed");
      // At least just set the status of the test to completed.
      personalize_agent_set_status($name, PERSONALIZE_STATUS_COMPLETED);
    }
  }
  if (!$errors) {
    // Finally, enable V2 :)
    variable_set('acquia_lift_v2_enabled', TRUE);
  }
  else {
    drush_print("V2 has not been enabled due to errors archiving some tests. Please fix the issue and try again.");
  }
}

/**
 * Pulls all reports for a V1 and puts them in a json file.
 * @param \AcquiaLiftAPI $lift_api
 * @param $agent
 * @param $goals
 * @param null $destination
 * @return bool
 */
function acquia_lift_archive_test_report(AcquiaLiftAPI $lift_api, $agent, $goals, $destination = NULL) {
  $message = "\n" . dt('Archiving test ') . $agent->machine_name . " ...";
  drush_print($message);
  try {
    // Use today's date as the report end date unless the campaign has been
    // completed.
    $end_ts = time();
    $status = personalize_agent_get_status($agent->machine_name);
    if ($status == PERSONALIZE_STATUS_COMPLETED) {
      $end_ts = personalize_agent_get_stop_date($agent->machine_name);
    }
    $end = date('Y-m-d', $end_ts);
    if (is_numeric($agent->started) && $agent->started > 0) {
      $start = date('Y-m-d', $agent->started);
    }
    else {
      // Just take two weeks off the end date.
      $start_ts = $end_ts - (14 * 86400);
      $start = date('Y-m-d', $start_ts);
    }

    $json = '{"reports":{"agent-status":';
    // ...get agent status report
    $agent_status_report = $lift_api->getAgentStatusReport(array($agent->machine_name), NULL, TRUE);
    $json .= $agent_status_report;
    $json .= ', "confidence":';
    // ...get confidence report, aggregated.
    $conf_report_aggr = $lift_api->getConfidenceReport($agent->machine_name, $start, $end, NULL, array('aggregated-over-dates' => TRUE), TRUE);
    $json .= $conf_report_aggr;
    $json .= ', "confidence_detail":';
    // ...get confidence report, unaggregated.
    $conf_report_detail = $lift_api->getConfidenceReport($agent->machine_name, $start, $end, NULL, array('aggregated-over-dates' => FALSE), TRUE);
    $json .= $conf_report_detail;
    foreach ($goals as $goal) {
      $json .= ', "confidence_' . $goal . '":';
      // ...get confidence report for goal, aggregated.
      $json .= $lift_api->getConfidenceReport($agent->machine_name, $start, $end, NULL, array('aggregated-over-dates' => TRUE, 'goal' => $goal), TRUE);
      $json .= ', "confidence_' . $goal . '_detail":';
      // ...get confidence report for goal, unaggregated.
      $json .= $lift_api->getConfidenceReport($agent->machine_name, $start, $end, NULL, array('aggregated-over-dates' => FALSE, 'goal' => $goal), TRUE);
    }
    $json .= '}}';
  }
  catch (AcquiaLiftNotFoundException $e) {
    // If the report cannot be found, there's nothing to be archived... all good.
    return TRUE;
  }
  catch (AcquiaLiftException $e) {
    // Something else went wrong and we should not proceed.
    drush_print($e->getMessage());
    return FALSE;
  }

  if ($destination == NULL) {
    $destination = variable_get('file_public_path', conf_path() . DIRECTORY_SEPARATOR . 'files');
  }

  $file_name = "$destination/{$agent->machine_name}-reports.json";
  if (file_exists($file_name)) {
    unlink($file_name);
  }
  if (file_put_contents($file_name, $json)) {
    drush_print("Report for {$agent->machine_name} archived successfully");
    variable_set('acquia_lift_report_source_' . $agent->machine_name, $file_name);
    return TRUE;
  }
  return FALSE;
}

/**
 * Batch API callback for when processing of all items is complete.
 *
 * @param $success
 *   Whether or not the batch completed successfully.
 * @param $results
 *   An array holding the results of each operation.
 * @param $operations
 *   An array of unprocessed operations.
 */
function acquia_lift_drush_batch_finished($success, $results, $operations) {
  // See if an error occurred.
  if (!$success) {
    // $operations contains the operations that remained unprocessed.
    $error_operation = reset($operations);
    $message = '- ' . dt('An error occurred while processing %error_operation with arguments: @arguments', array('%error_operation' => $error_operation[0], '@arguments' => print_r($error_operation[1], TRUE)));
    drush_log($message, 'error');
    return;
  }

  foreach ($results as $result) {
    $message = '- ' . dt($result);
    $message_type = 'ok';
    if (strpos($result, ACQUIA_LIFT_OPERATION_ERROR_PREFIX) === 0) {
      $message_type = 'error';
    }
    drush_log($message, $message_type);
  }
}
