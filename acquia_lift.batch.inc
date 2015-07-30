<?php
/**
 * @file acquia_lift.batch.inc
 *
 * Provides functions for batch syncing agents to Lift
 */

/**
 * Batch syncs the nested tests for the specified agent.
 *
 * @param stdClass $agent
 *
 * @return bool
 *   Returns TRUE if batch syncing completed successfully, FALSE otherwise.
 */
function acquia_lift_batch_sync_tests_for_agent($agent, $status_change = NULL) {
  try {
    $operations = acquia_lift_get_sync_operations_for_agent($agent);
    // We don't get to pass additional info to the callback, such as what we'd
    // like it to do upon success, so we define a different callback depending
    // on what has been passed as the $status_change parameter.
    $finish_callback = 'acquia_lift_batch_finished';
    if ($status_change == PERSONALIZE_STATUS_RUNNING) {
      $finish_callback = 'acquia_lift_batch_finish_and_start_campaign';
    }
    elseif ($status_change == PERSONALIZE_STATUS_SCHEDULED) {
      $finish_callback = 'acquia_lift_batch_finish_and_schedule_campaign';
    }
    $_SESSION['acquia_lift_agent_sync'] = $agent->machine_name;
    acquia_lift_do_batch_sync($operations, $finish_callback);
  }
  catch (AcquiaLiftException $e) {
    drupal_set_message(t('There was a problem syncing your personalization components to Lift: %error. Please !review your personalization and correct any problems listed.', array('%error' => $e->getMessage(), '!review' => l('review', 'admin/structure/personalize/manage/' . $agent->machine_name . '/review'))), 'error');
  }
}

/**
 * Batch syncs the nested tests for all agents.
 *
 * @return bool
 *   Returns TRUE if batch syncing completed successfully, FALSE otherwise.
 */
function acquia_lift_batch_sync_all() {
  $agents = personalize_agent_load_by_type('acquia_lift_target');
  $operations = array();
  foreach ($agents as $agent) {
    try {
      $operations += acquia_lift_get_sync_operations_for_agent($agent);
    }
    catch (AcquiaLiftException $e) {
      // Something went wrong getting all necessary batch operations for this
      // agent, just skip it and sync the next one.
      drupal_set_message(t('Could not sync the @agent personalization, most likely due to missing goals or variations.', array('@agent' => $agent->machine_name)), 'warning');
      continue;
    }
  }
  acquia_lift_do_batch_sync($operations);
}

/**
 * Performs batch syncing, given an array of operations.
 *
 * @param $operations
 */
function acquia_lift_do_batch_sync($operations, $finished = 'acquia_lift_batch_finished') {
  if (empty($operations)) {
    $finished(TRUE, array(), array(), 0);
    return;
  }
  $batch_operations = array();
  foreach ($operations as $operation) {
    $batch_operations[] = array('acquia_lift_batch_process_item', array($operation));
  }
  $batch = array(
    'operations' => $batch_operations,
    'finished' => $finished,
    'title' => t('Syncing personalization components to Acquia Lift'),
    'init_message' => t('Starting personalization sync.'),
    'progress_message' => t('Processed @current out of @total.'),
    'error_message' => t('Lift personalization sync has encountered an error.'),
    'file' => drupal_get_path('module', 'acquia_lift') . '/acquia_lift.batch.inc'
  );

  batch_set($batch);
}

/**
 * Batch API callback to process an API call to Lift.
 *
 * @param $item
 *   An array representing an API call that can be passed directly to
 *   acquia_lift_sync_item().
 *
 * @param &$context
 *   Parameter passed in by Batch API to allow keeping track of operation
 *   progress.
 */
function acquia_lift_batch_process_item($item, &$context) {
  if (!isset($context['sandbox']['progress'])) {
    $context['sandbox']['progress'] = 0;
  }
  if (!isset($context['sandbox']['retries'])) {
    $context['sandbox']['retries'] = 0;
  }
  $errors = array();
  acquia_lift_batch_sync_item($item, $errors);
  if (empty($errors)) {
    $context['results'][] = t('Method @method called successfully', array('@method' => $item['method']));
    $context['sandbox']['progress']++;
  }
  else {
    // We allow one retry per operation.
    if (!$context['sandbox']['retries']) {
      $context['finished'] = 0.5;
      $context['sandbox']['retries'] = 1;
    }
    else {
      // Is there a better way to signal that a particular operation was
      // unsuccessful during batch processing?
      $context['results'][] = ACQUIA_LIFT_OPERATION_ERROR_PREFIX . implode(',', $errors);
      $context['sandbox']['progress']++;
      // Reset retries for the next operation.
      $context['sandbox']['retries'] = 0;
    }
  }
}

/**
 * Batch API callback that also starts an agent after syncing items.
 *
 * @param $success
 *   Whether or not the batch completed successfully.
 * @param $results
 *   An array holding the results of each operation.
 * @param $operations
 *   An array of unprocessed operations.
 */
function acquia_lift_batch_finish_and_start_campaign($success, $results, $operations, $elapsed_time) {
  acquia_lift_batch_finished($success, $results, $operations, $elapsed_time, PERSONALIZE_STATUS_RUNNING);
}

/**
 * Batch API callback that also schedules an agent after syncing items.
 *
 * @param $success
 *   Whether or not the batch completed successfully.
 * @param $results
 *   An array holding the results of each operation.
 * @param $operations
 *   An array of unprocessed operations.
 */
function acquia_lift_batch_finish_and_schedule_campaign($success, $results, $operations, $elapsed_time) {
  acquia_lift_batch_finished($success, $results, $operations, $elapsed_time, PERSONALIZE_STATUS_SCHEDULED);
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
 * @param $next_status
 *   An optional next status to set the campaign to, if a single campaign was
 *   being sync'd.
 */
function acquia_lift_batch_finished($success, $results, $operations, $elapsed_time, $next_status = NULL) {
  if ($success) {
    // See if any of the results contains an error.
    $errors = array();
    foreach ($results as $result) {
      if (strpos($result, ACQUIA_LIFT_OPERATION_ERROR_PREFIX) === 0) {
        $errors[] = substr($result, strlen(ACQUIA_LIFT_OPERATION_ERROR_PREFIX));
      }
    }
    if (empty($errors)) {
      if (!empty($next_status)) {
        if ($agent = $_SESSION['acquia_lift_agent_sync']) {
          $updated = personalize_agent_set_status($agent, $next_status);
          if ($updated && $next_status == PERSONALIZE_STATUS_RUNNING) {
            drupal_set_message(t('Congratulations! You have started your personalization.'));
          }
          else if ($updated && $next_status == PERSONALIZE_STATUS_SCHEDULED) {
            module_load_include('inc', 'personalize', 'personalize.admin.campaign');
            $start_date = personalize_agent_get_start_date($agent);
            drupal_set_message(t('Congratulations!  You have scheduled your personalization to start on @date', array(
              '@date' => _personalize_campaign_wizard_date($start_date),
            )));
          }
        }
      }
    }
    else {
      $message_type = 'error';
      $message = t('Some errors occurred while syncing personalizations to Acquia Lift:');
      $message .= theme('item_list', array('items' => $errors));
      drupal_set_message($message, $message_type);
    }
  }
  if (isset($_SESSION['acquia_lift_agent_sync'])) {
    unset($_SESSION['acquia_lift_agent_sync']);
  }
}

/**
 * Returns all operations required to sync the testing components of an agent.
 *
 * @param $agent
 *   The agent to sync testing components for.
 * @return array
 *   An array of operations representing DELETE and PUT requests to be made to
 *   Lift.
 * @throws \AcquiaLiftCredsException
 * @throws \AcquiaLiftException
 */
function acquia_lift_get_sync_operations_for_agent($agent) {
  $operations = array();
  // First see if there are any tests that need to be deleted from Lift.
  if (isset($agent->tests_to_delete)) {
    foreach ($agent->tests_to_delete as $old_test_agent) {
      $operations[] = array(
        'method' => 'deleteAgent',
        'args' => array(
          $old_test_agent,
        ),
      );
    }
  }
  $nested = acquia_lift_get_nested_tests($agent);

  // If there are no nested tests then there is nothing further to sync.
  if (empty($nested)) {
    return $operations;
  }
  $nested_tests = personalize_agent_load_multiple($nested);
  $goals = personalize_goal_load_by_conditions(array('agent' => $agent->machine_name));
  if (empty($goals)) {
    throw new AcquiaLiftException('No goals have been set up for personalization ' . $agent->machine_name);
  }
  $lift_api = AcquiaLiftAPI::getInstance(variable_get('acquia_lift_account_info', array()));
  foreach ($nested_tests as $agent_name => $test) {
    if (!acquia_lift_is_testing_agent($test)) {
      continue;
    }

    if ($agent_instance = personalize_agent_load_agent($agent_name, TRUE)) {
      if (!$agent_instance instanceof AcquiaLiftAgent) {
        continue;
      }

      // If we're dealing with an agent that already exists in Lift
      // then we may need to delete some of its components.
      try {
        $existing_agent = $lift_api->getAgent($agent_name);
      }
      catch (AcquiaLiftException $e) {
        if ($e instanceof AcquiaLiftNotFoundException) {
          $existing_agent = FALSE;
        }
        else {
          // Any other exception means we can't communicate with Lift so throw
          // an exception to indicate batch sync cannot complete.
          throw new AcquiaLiftException('There is a problem communicating with Lift, syncing cannot complete at this time.');
        }
      }

      // First sync the agent itself.
      $operations = array_merge($operations, $agent_instance->getAgentSyncOperations());

      // Now go through all option sets and sync those.
      $option_sets = personalize_option_set_load_by_agent($agent_name, TRUE);
      if (empty($option_sets)) {
        throw new AcquiaLiftException('Missing variation set for test ' . $agent_name);
      }
      $first_os = reset($option_sets);
      if (count($first_os->options) < 2) {
        throw new AcquiaLiftException('Test ' . $agent_name . ' has fewer than two variations.');
      }
      $decisions = AcquiaLiftAgent::convertOptionSetsToDecisions($option_sets);

      $operations = array_merge($operations, $agent_instance->getDecisionSyncOperations(array(), $decisions));
      $new_goals = $old_goals = array();
      foreach ($goals as $goal) {
        $new_goals[$goal->action] = $goal->value;
      }
      if ($existing_agent && ($existing_goals = $lift_api->getGoalsForAgent($agent_name))) {
        // If it's an existing agent there may be goals that need
        // to be deleted.
        foreach ($existing_goals as $goal) {
          // The array of goals has goal names as keys and goal values
          // as values, but the values are not used so we can pass
          // any value for each old goal.
          $old_goals[$goal] = 1;
        }
      }
      $operations = array_merge($operations, $agent_instance->getGoalSyncOperations($old_goals, $new_goals));
    }
  }
  return $operations;
}
