<?php

/**
 * @file acquia_lift.admin.inc
 * Provides functions needed for the admin UI.
 */

/**
 * Menu callback for the Acquia Lift settings page.
 *
 * Consists of multiple forms.
 *
 * @return array
 *   A render array for the page.
 */
function acquia_lift_configuration_page() {
  $build['main_config'] = drupal_get_form('acquia_lift_admin_form');
  $build['batch_sync'] = drupal_get_form('acquia_lift_batch_sync_form');
  return $build;
}

/**
 * Admin form for configuring personalization backends.
 */
function acquia_lift_admin_form($form, &$form_state) {
  $form = array(
    '#attached' => array(
      'css' => array(
        drupal_get_path('module', 'acquia_lift') . '/css/acquia_lift.admin.css',
      ),
    ),
  );
  $account_info = variable_get('acquia_lift_account_info', array());
  $account_info_provided = !empty($account_info['owner_code']) && !empty($account_info['api_key']);
  if ($account_info_provided) {
    // Add a button for checking the connection.
    $form['ping_test_wrapper'] = array(
      '#theme_wrappers' => array('container'),
      '#attributes' => array('id' => 'acquia-lift-config-messages'),
    );
    $form['ping_test'] = array(
      '#type' => 'submit',
      '#value' => t('Test connection to Acquia Lift'),
      '#attributes' => array('title' => t('Click here to check your Acquia Lift connection.')),
      '#submit' => array('acquia_lift_ping_test_submit'),
      '#ajax' => array(
        'callback' => 'acquia_lift_ping_test_ajax_callback',
        'wrapper' => 'acquia-lift-ping-test',
        'effect' => 'fade',
      ),
      '#limit_validation_errors' => array(),
    );
    // Add info about number of API calls made last month and current month
    // to date.
    try {
      $api = AcquiaLiftAPI::getInstance(variable_get('acquia_lift_account_info', array()));
      $ts = time();
      $calls_last_month = $api->getTotalRuntimeCallsForPreviousMonth($ts);
      $form['calls_last_month'] = array(
        '#type' => 'markup',
        '#markup' => '<div>' . t('Number of API calls made last month: ') . $calls_last_month . '</div>',
      );
      $calls_this_month = $api->getTotalRuntimeCallsForMonthToDate($ts);
      $form['calls_this_month'] = array(
        '#type' => 'markup',
        '#markup' => '<div>' . t('Number of API calls made so far this month: ') . $calls_this_month . '</div>',
      );
    }
    catch (Exception $e) {
      drupal_set_message($e->getMessage());
    }
  }

  $form['acquia_lift_account_info'] = array(
    '#type' => 'fieldset',
    '#title' => 'Acquia Lift Account Settings',
    '#tree' => TRUE,
    '#collapsible' => TRUE,
    '#collapsed' => $account_info_provided
  );

  $form['acquia_lift_account_info']['msg'] = array(
    '#markup' => t("<p>This information is used to link your !acquialift account to Drupal.</p><p>If you are already an Acquia Lift customer, contact Acquia Support to obtain your credentials. Otherwise, contact !advocacyemail to purchase a subscription to the Acquia Lift service.</p>", array('!acquialift' => l(t('Acquia Lift'), 'http://www.acquia.com/products-services/website-personalization', array('attributes' => array('target' => '_blank'))), '!advocacyemail' => l('advocacy@acquia.com', 'mailto:advocacy@acquia.com'))),
  );

  $form['acquia_lift_account_info']['owner_code'] = array(
    '#type' => 'textfield',
    '#title' => t('Owner Code'),
    '#default_value' => !empty($account_info['owner_code']) ? $account_info['owner_code'] : '',
    '#size' => 35,
    '#maxlength' => 50,
    '#description' => t("Paste in your Acquia Lift owner code"),
    '#required' => TRUE,
  );

  $form['acquia_lift_account_info']['api_key'] = array(
    '#type' => 'textfield',
    '#title' => t('Runtime API Key'),
    '#default_value' => !empty($account_info['api_key']) ? $account_info['api_key'] : '',
    '#size' => 35,
    '#maxlength' => 50,
    '#description' => t("Paste in your Acquia Lift api key"),
    '#required' => TRUE,
  );

  $form['acquia_lift_account_info']['admin_key'] = array(
    '#type' => 'textfield',
    '#title' => t('Admin API Key'),
    '#default_value' => !empty($account_info['admin_key']) ? $account_info['admin_key'] : '',
    '#size' => 35,
    '#maxlength' => 50,
    '#description' => t("Paste in your Acquia Lift admin key"),
    '#required' => TRUE,
  );

  $form['acquia_lift_account_info']['api_url'] = array(
    '#type' => 'textfield',
    '#title' => t('API Server URL'),
    '#default_value' => !empty($account_info['api_url']) ? $account_info['api_url'] : '',
    '#field_prefix' => 'http(s)://',
    '#size' => 35,
    '#maxlength' => 50,
    '#description' => t("Paste in your Acquia Lift API URL"),
    '#required' => TRUE,
  );
  $form['acquia_lift_batch_decisions'] = array(
    '#type' => 'checkbox',
    '#default_value' => variable_get('acquia_lift_batch_decisions', FALSE),
    '#title' => t('Make all decisions on a page in one call'),
    '#description' => t('If this is enabled, Lift will look at every decision to be made on the page and make it in a single request. If disabled, there will be a separate HTTP request per decision.')
  );
  $form['acquia_lift_confidence_measure'] = array(
    '#type' => 'textfield',
    '#title' => t('Confidence measure'),
    '#size' => 3,
    '#field_suffix' => '%',
    '#required' => TRUE,
    '#default_value' => variable_get('acquia_lift_confidence_measure', 95),
    '#description' => t('The confidence percentage at which a test is considered statistically significant.'),
    '#element_validate' => array('element_validate_number'),
  );

  $form['minimum_runtime'] = array(
    '#type' => 'container',
    '#tree' => FALSE,
    '#attributes' => array(
      'class' => array('acquia-lift-config-minimum-runtime'),
    ),

  );
  $form['minimum_runtime']['acquia_lift_min_runtime_title'] = array(
    '#type' => 'markup',
    '#markup' => '<label for="acquia_lift_min_runtime_num">' . t('Duration') . '<span class="form-required" title="This field is required.">*</span></label>',
  );
  $form['minimum_runtime']['acquia_lift_min_runtime_num'] = array(
    '#type' => 'textfield',
    '#size' => 3,
    '#required' => TRUE,
    '#default_value' => variable_get('acquia_lift_min_runtime_num', 2),
    '#element_validate' => array('element_validate_number'),
  );
  $form['minimum_runtime']['acquia_lift_min_runtime_unit'] = array(
    '#type' => 'select',
    '#required' => TRUE,
    '#default_value' => variable_get('acquia_lift_min_runtime_unit', 'week'),
    '#options' => array(
      'minute' => t('minutes'),
      'hour' => t('hours'),
      'day' => t('days'),
      'week' => t('weeks')
    ),
  );
  $form['minimum_runtime']['acquia_lift_min_runtime_desc'] = array(
    '#type' => 'markup',
    '#markup' => '<div class="description">' . t('The amount of time a personalization will run before it can be completed. Applies only to personalizations with the "End when personalization thresholds are reached" option selected.') . '</div>',
  );
  $form['acquia_lift_min_decisions'] = array(
    '#type' => 'textfield',
    '#title' => t('Decisions'),
    '#size' => 3,
    '#required' => TRUE,
    '#default_value' => variable_get('acquia_lift_min_decisions', 1000),
    '#description' => t('The number of variations selected by Acquia Lift for display to visitors before the personalization can be completed. Applies only to personalizations with the "End when personalization thresholds are reached" option selected.'),
    '#element_validate' => array('element_validate_number'),
  );
  $form = system_settings_form($form);
  $form['#submit'][] = 'acquia_lift_admin_form_submit';
  return $form;
}

/**
 * Simple form for initiating batch syncing of agents to Lift.
 */
function acquia_lift_batch_sync_form($form, &$form_state) {
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Synchronize with Acquia Lift service')
  );
  $form['explanation'] = array(
    '#type' => 'markup',
    '#markup' => '<div>' . t('Sends your local personalization information to the hosted Acquia Lift service. Use this feature if you change your Acquia Lift credentials after creating one or more personalizations.') . '</div>'
  );
  return $form;
}

/**
 * Submit callback for the batch sync form.
 */
function acquia_lift_batch_sync_form_submit($form, &$form_state) {
  module_load_include('inc', 'acquia_lift', 'acquia_lift.batch');
  if (!acquia_lift_batch_sync_all()) {
    form_set_error(NULL, t('A problem occurred during batch sync, please try again later'));
  }
}

/**
 * Submit callback for the ping test button.
 */
function acquia_lift_ping_test_submit($form, &$form_state) {
  $api = AcquiaLiftAPI::getInstance(variable_get('acquia_lift_account_info', array()));
  if ($api->pingTest()) {
    drupal_set_message(t('Successfully connected to the Acquia Lift service'));
  }
  else {
    drupal_set_message(t('There was a problem connecting to the Acquia Lift service. Please check your credentials'), 'error');
  }
}

/**
 * Ajax callback for the ping test button.
 */
function acquia_lift_ping_test_ajax_callback($form, &$form_state) {
  $commands = array();
  // Show status messages.
  $commands[] = ajax_command_replace('#acquia-lift-config-messages', '<div id="acquia-lift-config-messages">' . theme('status_messages') . '</div>');
  return array(
    '#type' => 'ajax',
    '#commands' => $commands,
  );
}

/**
 * Validation callback for the Acquia Lift admin form.
 */
function acquia_lift_admin_form_validate($form, &$form_state) {
  if (!AcquiaLiftAPI::codeIsValid($form_state['values']['acquia_lift_account_info']['owner_code'])) {
    form_set_error('acquia_lift_account_info][owner_code', 'You must enter a valid owner code');
  }
  if (!valid_url($form_state['values']['acquia_lift_account_info']['api_url'])) {
    form_set_error('acquia_lift_account_info][api_url', t('You must enter a valid URL'));
  }
  // Strip any scheme from the API URL.
  $form_state['values']['acquia_lift_account_info']['api_url'] = preg_replace('/(^[a-z]+:\/\/)/i', '', $form_state['values']['acquia_lift_account_info']['api_url']);
  if ($form_state['values']['acquia_lift_confidence_measure'] <= 0 || $form_state['values']['acquia_lift_confidence_measure'] >= 100) {
    form_set_error('acquia_lift_confidence_measure', t('Confidence measure must be a value between 0 and 100.'));
  }
}

/**
 * Submit handler for the Acquia Lift admin form.
 *
 * Creates a default Acquia Lift agent if one does not yet exist.
 */
function acquia_lift_admin_form_submit($form, &$form_state) {
  if ($form_state['values']['acquia_lift_confidence_measure'] < 95) {
    drupal_set_message(t('A minimum confidence measure of 95% is recommended to ensure proper evaluation of test results.'), 'warning');
  }

  // Clear the ctools plugin "agent_type" cache for personalize, clear loaded
  // files cache, and rebuild the autoloader class definitions.
  cache_clear_all('plugins:personalize:agent_type', 'cache', TRUE);
  cache_clear_all('ctools_plugin_files:personalize:agent_type', 'cache', TRUE);
  registry_rebuild();
}

/**
 * =======================================================================
 *  A C Q U I A  L I F T  A G E N T  R E P O R T I N G
 * =======================================================================
 */

/**
 * Form build function for the Acquia Lift report, which has filters.
 *
 * @param stdClass $agent_data
 *   The campaign agent data for this report.
 * @param stdClass $option_set
 *   (optional) The content variation to show in the default view.
 */
function acquia_lift_report($form, &$form_state, $agent_data, $option_set = NULL, $nested = FALSE) {
  $legacy = FALSE;
  if (acquia_lift_is_targeting_agent($agent_data)) {
    // Display a report for any nested test.
    $report = array();
    $option_set = acquia_lift_get_option_set_for_targeting($agent_data->machine_name);
    if (empty($option_set->targeting)) {
      return $report;
    }
    foreach($option_set->targeting as $audience_name => $audience) {
      if (isset($audience['osid'])) {
        if ($option_set = personalize_option_set_load($audience['osid'])) {
          $nested_agent = personalize_agent_load($option_set->agent);
          if (!empty($nested_agent)) {
            $report[$nested_agent->machine_name] = array(
              'title' => array(
                '#markup' => '<h2>' . t('Test report for audience @audience', array('@audience' => $audience_name)) . '</h2>'
              ),
              'report' => acquia_lift_report($form, $form_state, $nested_agent, NULL, TRUE)
            );
          }
        }
      }
    }
    return $report;
  }
  elseif (!$nested && acquia_lift_is_testing_agent($agent_data)) {
    $legacy = TRUE;
  }

  if ($agent_data->started == 0) {
    return array(
      'no_report' => array(
        '#markup'=> t('This agent has not started running yet, no reports to show.')
      )
    );
  }
  // Instantiate the agent class to perform some further checks before trying to
  // show reports for it.
  if (!$agent_instance = personalize_agent_load_agent($agent_data->machine_name)) {
    return array();
  }
  // If this agent is not currently enabled in Acquia Lift, there are no reports
  // to show.
  $errors = $agent_instance->errors();
  if (!empty($errors)) {
    return array(
      'no_report' => array(
        '#markup'=> t('This agent is not properly configured, no reports to show.')
      )
    );
  }
  // If this agent doesn't implement the reporting interface then there are no
  // reports to show.
  if (!$agent_instance instanceof PersonalizeAgentReportInterface) {
    return array(
      'no_report' => array(
        '#markup' => t('This agent does not support reporting.')
      )
    );
  }

  return acquia_lift_report_custom($form, $form_state, $agent_instance, $agent_data, $option_set, $legacy);
}

/**
 * Submit handler for Acquia Lift reports.
 */
function acquia_lift_report_submit($form, &$form_state) {
  $form_state['rebuild'] = TRUE;
}

/**
 * Form build function for a custom Acquia Lift agent report.
 *
 * @param AcquiaLiftAgentInterface $agent
 *   The instantiated agent for the selected campaign.
 * @param stdClass $agent_data
 *   The campaign agent data for this report.
 * @param stdClass $option_set
 *   (optional) The content variation to show in the default view.
 */
function acquia_lift_report_custom($form, &$form_state, $agent, $agent_data, $option_set, $legacy = FALSE) {
  // Check for Rickshaw and D3 libraries and alert users if not exist.
  if (_acquia_lift_missing_library_warning(array('rickshaw', 'd3'), t('The following libraries are required in order to view the Acquia Lift reports:'))) {
    return array();
  }
  $agent_name = $agent->getMachineName();
  // Generate report filters.
  $data = $agent->getData();
  $form = array(
    '#prefix' => '<div id="acquia-lift-report-' . $agent_name . '">',
    '#suffix' => '</div>',
    '#attached' => array(
      'css' => array(
        drupal_get_path('module', 'acquia_lift') . '/css/acquia_lift.admin.css',
      ),
      'js' => array(
        drupal_get_path('module', 'acquia_lift') . '/js/acquia_lift.admin.js',
      ),
      'library' => array(
        array('acquia_lift', 'acquia_lift.reports'),
      ),
    ),
  );

  $form['report_filters'] = array(
    '#type' => 'container',
    '#tree' => FALSE,
    '#attributes' => array(
      'class' => array('acquia-lift-report-filters', 'clearfix'),
    ),
  );

  // Get the decision points for this agent so we can provide a filter on this.
  $option_sets = personalize_option_set_load_by_agent($agent_name);
  $decisions = AcquiaLiftAgent::convertOptionSetsToDecisions($option_sets);
  $decision_options = array();
  foreach ($decisions as $name => $decision) {
    $decision_options[$name] = personalize_get_label_for_decision_point_name($name);
  }

  // Decision point filters.
  if (isset($form_state['values']['decision_point'])) {
    $decision_point = $form_state['values']['decision_point'];
  }
  else {
    $decision_point = !empty($option_set) ? personalize_get_decision_name_for_option_set($option_set) : key($decisions);
  }
  $form['report_filters']['decision_point'] = acquia_lift_report_decision_point_dropdown($decision_options, $decision_point, $agent_name);

  // Default to showing the complete history of the campaign.
  $date_start_report = date('Y-m-d', $agent_data->started);
  $date_end_report = $date_end_targeting = date('Y-m-d');

  // If the campaign's status is "completed" then we need to use the date it ended.
  $agent_name = $agent->getMachineName();
  $status = variable_get(_personalize_agent_get_status_variable($agent_name));
  if ($status == PERSONALIZE_STATUS_COMPLETED && $end_time = personalize_agent_get_stop_date($agent_name)) {
    $date_end_report = $date_end_targeting = date('Y-m-d', $end_time);
  }
  // Conversion report filters.
  $selected_goal = empty($form_state['values']['goal']) ? NULL : $form_state['values']['goal'];
  $selected_metric = empty($form_state['values']['metric']) ? 'rate' : $form_state['values']['metric'];

  if ($legacy) {
    // The file containing the legacy report class needs to be explicitly loaded.
    module_load_include('inc', 'acquia_lift', 'includes/AcquiaLiftReportLegacy');
  }
  $options = array(
    'decision' => $decision_point,
    'start' => $date_start_report,
    'end' => $date_end_report,
    'goal' => $selected_goal,
    'conversion_metric' => $selected_metric,
    'legacy' => $legacy
  );

  $reports = $agent->buildCampaignReports($options);
  if (!($reports['#has_data'])) {
    // This campaign hasn't been shown yet, so there is no data for reporting.
    return array(
      'no_report' => array(
        '#markup' => t('This personalization does not yet contain information about your visitors\' website interactions. This situation generally occurs for personalizations that have either just been started or do not generate much website traffic.'),
      ),
    );
  }

  // Generate mark-up for adaptive style report labels.
  $report_title_additional = '';
  if ($data['decision_style'] === 'adaptive') {
    $report_title_additional = theme('acquia_lift_percentage_label', array(
      'percent_label' => t('Random'),
      'rest_label' => t('Personalized'),
      'percent' => $data['explore_rate']
    ));
  }

  // Overview report section.
  $form['overview_report'] = array(
    'overview_report_title' => array(
      '#markup' => '<h2>' . t('Overview') . '</h2>',
    ),
    '#theme_wrappers' => array('container'),
    '#attributes' => array(
      'id' => 'acquia-lift-overview-report',
      'class' => array('acquia-lift-report-section', 'clearfix'),
    ),
  );
  $form['overview_report']['report'] = array(
    '#markup' => drupal_render($reports['overview']),
    '#theme_wrappers' => array('container'),
    '#id' => 'acquia-lift-overview-report-data',
  );

  // Conversion details section.
  $form['experiment_report'] = array(
    '#type' => 'container',
    'header' => array(
      '#type' => 'container',
      '#attributes' => array(
        'class' => array('acquia-lift-report-section-header', 'clearfix'),
      ),
      'title' => array(
        '#type' => 'container',
        '#attributes' => array(
          'class' => array('acquia-lift-report-section-title'),
        ),
        'report_title' => array(
          '#markup' => '<h2>' . t('Experiment') . '</h2>',
        ),
      ),
    ),
    'summary' => array(
      '#type' => 'container',
      '#attributes' => array(
        'class' => array('acquia-lift-report-header-summary'),
      )
    ),
    '#attributes' => array(
      'id' => 'acquia-lift-experiment-report',
      'class' => array('acquia-lift-report-section'),
    ),
  );
  if (!empty($report_title_additional)) {
    $form['experiment_report']['header']['title']['groups'] = array(
      '#markup' => t('random group'),
    );
    $form['experiment_report']['header']['summary']['distribution'] = array(
      '#markup' => $report_title_additional,
      '#theme_wrappers' => array('container'),
    );
  }
  // Get the conversion report options.
  $form['experiment_report']['header']['options'] = array(
    '#type' => 'container',
    '#attributes' => array(
      'class' => array('acquia-lift-report-section-options'),
    ),
    '#tree' => FALSE,
    'goal' => acquia_lift_report_goal_dropdown($agent->getMachineName(), $selected_goal),
    'metric' => acquia_lift_report_conversion_metric_dropdown($selected_metric),
    'submit' => array(
      '#type' => 'submit',
      '#value' => t('Filter'),
    ),
  );
  $form['experiment_report']['header']['summary']['report_summary'] = array(
    '#theme_wrappers' => array('container'),
    '#markup' => t('See which content variations are winning'),
    '#attributes' => array(
      'class' => array('acquia-lift-report-summary'),
    ),
  );
  $form['experiment_report']['report'] = array(
    '#markup' => drupal_render($reports['experiment']),
    '#theme_wrappers' => array('container'),
    '#id' => 'acquia-lift-experiment-report-data',
  );

  if ($legacy) {
    acquia_lift_legacy_report_form($form, $form_state, $reports, $report_title_additional);
  }

  return $form;
}

/**
 * Returns a dropdown for filtering by decision point.
 *
 * @param array $options
 *   The different decision point options.
 * @param $selected
 *   The decision point to set as the default value.
 * @return array
 *   An array representing a dropdown select list.
 */
function acquia_lift_report_decision_point_dropdown($options, $selected, $agent_name) {
  if (empty($options)) {
    return array();
  }
  if (count($options) == 1) {
    $option = key($options);
    return array(
      '#type' => 'hidden',
      '#value' => $option,
    );
  }
  return array(
    '#title' => t('Variation Set'),
    '#type' => 'select',
    '#options' => $options,
    '#default_value' => $selected,
    '#ajax' => array(
      'callback' => "acquia_lift_report_ajax_callback",
      'wrapper' => "acquia-lift-report-" . $agent_name,
    ),
    '#id' => 'acquia-lift-report-decision-point-filter',
  );
}

/**
 * Returns the drop-down for filtering reports by goal.
 *
 * @param $agent_name
 *   The machine name of the campaign.
 * @param $selected
 *   The selected goal action.
 * @return array
 *   A form element array to be used as the dropdown.
 */
function acquia_lift_report_goal_dropdown($agent_name, $selected = NULL) {
  $goals = personalize_goal_load_by_conditions(array('agent' => $agent_name));
  // There should always be at least one goal in an Acquia Lift report.
  if (empty($goals)) {
    return array();
  }
  if (count($goals) == 1) {
    $goal = current($goals);
    return array(
      '#type' => 'hidden',
      '#value' => $goal->action,
    );
  }
  else {
    $actions = visitor_actions_get_actions();
    foreach ($goals as $goal) {
      $options[$goal->action] = $actions[$goal->action]['label'];
    }
    return array(
      '#title' => t('Goals'),
      '#type' => 'select',
      '#options' => $options,
      '#default_value' => $selected,
      '#empty_option' => t('All goals'),
    );
  }
}

/**
 * Returns the drop-down for filtering reports by metric.
 *
 * @param $selected
 *   The currently display metric.
 * @return array
 *   A form element array to be used as the dropdown.
 */
function acquia_lift_report_conversion_metric_dropdown($selected) {
  return array(
    '#type' => 'select',
    '#title' => t('Metrics'),
    '#options' => array(
      'rate' => t('Conversion rate %'),
      'value' => t('Conversion value')
    ),
    '#default_value' => $selected,
    '#required' => TRUE,
  );
}

/**
 * Ajax callback for filtering options.
 */
function acquia_lift_report_ajax_callback($form, &$form_state) {
  return $form;
}

/**
 * AJAX callback to return the tabular HTML for conversion reports.
 *
 * The following parameters are supplied within the query string:
 *   - campaign: the machine name of the campaign
 *   - decision: the decision name to get data
 *   - start: (optional) the timestamp to use as start date
 *   - end: (optional) the timestamp to use as the end date
 *   - goal: (optional) a goal name to limit results (defaults to all goals)
 */
function acquia_lift_report_conversion() {
  $params = drupal_get_query_parameters();
  $agent_name = empty($params['campaign']) ? NULL : filter_xss($params['campaign']);
  $options = array();
  $options['decision'] = empty($params['decision']) ? NULL : filter_xss($params['decision']);
  $options['goal'] = empty($params['goal']) ? NULL : filter_xss($params['goal']);
  $options['start'] = empty($params['start']) ? NULL : check_plain($params['start']);
  $options['end'] = empty($params['end']) ? NULL : check_plain($params['end']);
  $agent = personalize_agent_load($agent_name);

  if (!acquia_lift_is_testing_agent($agent)) {
    return array();
  }
  if (is_numeric($options['start'])) {
    $options['start'] = date('Y-m-d', $options['start']);
  }
  if (is_numeric($options['end'])) {
    $options['end'] = date('Y-m-d', $options['end']);
  }

  $reports = array();
  if ($plugin = personalize_agent_load_agent($agent_name)) {
    $reports = $plugin->buildConversionReport($options);
  }
  $reports['#type'] = 'ajax';
  return drupal_json_output(drupal_render($reports));
}


/**
 * =======================================================================
 *  A C Q U I A  L I F T  T A R G E T I N G  W O R K F L O W
 * =======================================================================
 */

/**
 * Saves a target audience for an agent.
 *
 * @param $label
 *   The human-readable name of the audience to save.
 * @param $agent_name
 *   THe name of the agent to add the audience to.
 * @param $contexts
 *   The contexts that make up the audience definition.
 * @param $strategy
 *   The strategy to use for multiple contexts, i.e. 'AND' or 'OR'
 * @param number $weight
 *   The weight for this audience within all audiences for the agent.
 *   Audiences with the lowest weight will be evaluated first.
 * @param $machine_name
 *   The machine name for the audience if it already exists.
 */
function acquia_lift_target_audience_save($label, $agent_name, $contexts, $strategy, $weight = 50, $machine_name = NULL) {
  module_load_include('inc', 'personalize', 'personalize.admin');
  // Find the option set to use for targeting and add the audience.
  $option_set = acquia_lift_get_option_set_for_targeting($agent_name);
  if (empty($option_set)) {
    return FALSE;
  }
  $machine_name = empty($machine_name) ? personalize_generate_machine_name($label, NULL, '-') : $machine_name;
  if (!isset($option_set->targeting[$machine_name])) {
    $option_set->targeting[$machine_name] = array();
  }
  $option_set->targeting[$machine_name]['label'] = $label;
  $option_set->targeting[$machine_name]['weight'] = $weight;
  // Generate the feature strings and rules for the contexts.
  $agent = personalize_agent_load_agent($agent_name);
  $feature_strings = $feature_rules = array();
  foreach ($contexts as $context_values) {
    list($plugin_name, $context_name) = explode(PERSONALIZE_TARGETING_ADMIN_SEPARATOR, $context_values['context']);
    // Generate a value code based on the operator used.
    $value = personalize_targeting_generate_value_code($context_values['match'], $context_values['operator']);
    // Create a feature string for this context value that can be consumed
    // by the agent that will be using it.
    $feature_string = $agent->convertContextToFeatureString($context_name, $value);
    $feature_strings[] = $feature_string;
    // Save the actual rule information as this is what will be used
    // for evaluating it.
    $feature_rules[$feature_string] = $context_values;
    // Override the context to split it into plugin and context parts.
    $feature_rules[$feature_string]['context'] = $context_name;
    $feature_rules[$feature_string]['plugin'] = $plugin_name;
  }
  $option_set->targeting[$machine_name]['targeting_features'] = $feature_strings;
  $option_set->targeting[$machine_name]['targeting_rules'] = $feature_rules;
  $option_set->targeting[$machine_name]['targeting_strategy'] = $strategy;
  try {
    personalize_option_set_save($option_set);
    return TRUE;
  }
  catch (PersonalizeException $e) {
    return FALSE;
  }
}

/**
 * Implements the structure, including all sub-tests, required by the targeting.
 *
 * @param $agent_data
 *   The parent personalization.
 * @return array
 *   An array of nested tests that need to be deleted in Lift following the
 *   implementation of the test structure.
 * @throws \AcquiaLiftException
 */
function acquia_lift_implement_test_structure($agent_data) {
  if (isset($agent_data->data['variation_set_handling']) && $agent_data->data['variation_set_handling'] == ACQUIA_LIFT_DECISION_MULTIVARIATE) {
    $tests_to_delete = acquia_lift_implement_mvt($agent_data);
  }
  else {
    $tests_to_delete = acquia_lift_implement_targeting($agent_data);
  }
  // Once we implement the structure for a personalization, we do not allow
  // adding or removing variation sets.
  // @todo This can be revisited later, it is simply too cumbersome to support
  //   such post-implementation changes at this time and the use-case of multiple
  //   variation sets so is small that it does not warrant such effort.
  $agent_data->data['locked_for_variation_sets'] = TRUE;
  personalize_agent_save($agent_data);

  return $tests_to_delete;
}

/**
 * Takes whatever is in the 'lift_targeting' data property and converts it into
 * the required campaign structure (including nested tests where needed).
 *
 * @param stdClass $agent
 *   The agent to create the targeting structure for.
 *
 * @return array
 *   An array of agent names that have been deleted and therefore need to be
 *   deleted in Lift.
 */
function acquia_lift_implement_targeting($agent) {

  if (empty($agent->data['lift_targeting'])) {
    return array();
  }
  $all_option_sets = personalize_option_set_load_by_agent($agent->machine_name);

  // Bail if not all option sets have an equal number of options.
  $num_decisions = $decision_name = NULL;
  foreach ($all_option_sets as $os) {
    if ($num_decisions == NULL) {
      $num_decisions = count($os->options);
    }
    elseif (count($os->options) !== $num_decisions) {
      throw new AcquiaLiftException('Variation sets do not have equal numbers of options');
    }
  }
  // First we need to figure out what existing tests we have running as nested
  // tests for this agent.
  $targeting_option_set = acquia_lift_get_option_set_for_targeting($agent->machine_name);
  // Make sure we only have one reference to the targeting option set.
  $all_option_sets[$targeting_option_set->osid] = $targeting_option_set;
  $existing_structure = acquia_lift_get_structure_from_targeting($targeting_option_set);

  // Any target audience that has multiple variations in it has a test running.
  $existing_tests = $existing_options = array();
  foreach ($existing_structure as $audience => $variations) {
    if (count($variations) > 1) {
      $existing_tests[$audience] = $variations;
    }
    else {
      $existing_options[$audience] = reset($variations);
    }
  }
  // Now look at the tests and options required by the new structure.
  $rules = $new_tests = $new_options = array();
  foreach ($agent->data['lift_targeting'] as $new_audience => $variations) {
    if (count($variations) == 1) {
      $new_options[$new_audience] = reset($variations);
      continue;
    }
    // See if this variation combination already exists as a test.
    foreach ($existing_tests as $existing_audience => $vars) {
      if ($variations == $vars) {
        $rules[$new_audience] = $existing_audience;
      }
    }
    // If we didn't find an existing test corresponding to this combination,
    // we'll need to create a new one.
    if (!isset($rules[$new_audience])) {
      $new_tests[$new_audience] = $variations;
    }
  }
  // Keep track of nested agents that get deleted.
  $to_delete = array();
  foreach (array_keys($existing_tests) as $audience) {
    if (!in_array($audience, $rules)) {
      // Delete the old test if it's no longer being used.
      if ($deleted = acquia_lift_delete_old_nested_test($targeting_option_set, $audience)) {
        $to_delete[] = $deleted;
      }
    }
    else {
      // Change the audience for the test if necessary.
      $new_audience = array_search($audience, $rules);
      if ($new_audience != $audience) {
        acquia_lift_set_audience_for_test($targeting_option_set, $audience, $new_audience);
      }
    }
  }
  foreach ($existing_options as $audience => $option) {
    if (!isset($agent->data['lift_targeting'][$audience])) {
      acquia_lift_remove_option_for_audience($targeting_option_set, $audience);
    }
  }

  // Now create any new tests required by the new structure.
  foreach ($new_tests as $audience => $variations) {
    acquia_lift_add_new_test_for_audience($targeting_option_set, $audience, $agent, $variations);
  }
  foreach ($new_options as $audience => $option) {
    acquia_lift_set_option_for_audience($targeting_option_set, $option, $audience);
  }

  // Go through any other option sets for this personalization, assign the same
  // decision name and option IDs to each of them, and copy the targeting infor-
  // mation to them.
  foreach ($all_option_sets as $os) {
    if ($os->osid != $targeting_option_set->osid) {
      $os->targeting = $targeting_option_set->targeting;
      foreach ($os->options as $i => &$option) {
        $option['option_id'] = $targeting_option_set->options[$i]['option_id'];
      }
    }
    $os->decision_name = $agent->machine_name;
    personalize_option_set_save($os);
  }
  // Update the test options for any embedded tests.
  acquia_lift_save_nested_test_options($agent);
  // Now clobber the "lift_targeting" data as we only use that to store *changes*
  // to the running targeting.
  $agent->data['lift_targeting'] = array();
  personalize_agent_save($agent);
  return $to_delete;
}

/**
 * Assigns an option to a target audience.
 *
 * @param $option_set
 *   The option set whose targeting is being changed.
 * @param $option_id
 *   The option ID
 * @param $audience
 *   The name of hte targeting audience to assign the option to.
 * @throws \PersonalizeException
 */
function acquia_lift_set_option_for_audience(&$option_set, $option_id, $audience) {
  if (isset($option_set->targeting[$audience])) {
    // If there was a test assigned to this audience, remove it.
    if (isset($option_set->targeting[$audience]['osid'])) {
      unset($option_set->targeting[$audience]['osid']);
    }
    $option_set->targeting[$audience]['option_id'] = $option_id;
  }
}

/**
 * Unsets the option_id property for a given target audience.
 *
 * @param $option_set
 *   The option set whose targeting is being changed.
 * @param $audience
 *   The audience name.
 * @throws \PersonalizeException
 */
function acquia_lift_remove_option_for_audience(&$option_set, $audience) {
  if (isset($option_set->targeting[$audience])) {
    // If there was a test assigned to this audience, remove it.
    if (isset($option_set->targeting[$audience]['option_id'])) {
      unset($option_set->targeting[$audience]['option_id']);
    }
  }
}

/**
 * Creates a new nested test of the specified variations for the specified agent.
 *
 * @param $option_set
 *   The option set whose targeting is being changed.
 * @param $audience
 *   The target audience for the test.
 * @param $parent_agent
 *   The parent agent.
 * @param $variations
 *   The variations to create a test for.
 * @throws \PersonalizeException
 */
function acquia_lift_add_new_test_for_audience(&$option_set, $audience, $parent_agent, $variations) {
  if (!isset($option_set->targeting[$audience])) {
    return;
  }

  // First we need to create a new agent.
  $agent = new stdClass();
  $agent->label = 'Sub-test for ' . $parent_agent->label;
  $agent->plugin = acquia_lift_get_test_agent_plugin();
  $agent->data = array();
  $agent->machine_name = personalize_generate_machine_name($parent_agent->machine_name . '-test', 'personalize_agent_machine_name_exists');
  $agent = personalize_agent_save($agent);
  $nested_os = new stdClass();
  $nested_os->agent = $agent->machine_name;
  $nested_os->label = $agent->label;
  $nested_os->is_new = TRUE;
  $nested_os->data = array();
  $nested_os->options = array();
  // This is basically a non-renderable option set - just set the plugin to
  // 'options'.
  $nested_os->plugin = 'options';
  foreach ($variations as $option_id) {
    $nested_os->options[] = array(
      'option_id' => $option_id
    );
  }
  $nested_os = personalize_option_set_save($nested_os);
  $option_set->targeting[$audience]['osid'] = $nested_os->osid;
  if (isset($option_set->targeting[$audience]['option_id'])) {
    unset($option_set->targeting[$audience]['option_id']);
  }
}

/**
 * Saves a test to a particular targeting audience.
 *
 * @param $option_set
 *  The option set whose targeting is being changed.
 * @param $old_audience
 *   The target audience that currently has this test running.
 * @param $new_audience
 *   The target audience to move the test to.
 */
function acquia_lift_set_audience_for_test(&$option_set, $old_audience, $new_audience) {
  if ($old_audience == $new_audience) {
    return;
  }
  if (isset($option_set->targeting[$old_audience]) && isset($option_set->targeting[$old_audience]['osid'])) {
    $nested_osid = $option_set->targeting[$old_audience]['osid'];
    unset($option_set->targeting[$old_audience]['osid']);
  }
  else {
    return;
  }

  // Now add the test to the new audience.
  if (isset($option_set->targeting[$new_audience])) {
    // If this audience previously had a single option assigned to it, remove
    // that option.
    if (isset($option_set->targeting[$new_audience]['option_id'])) {
      unset($option_set->targeting[$new_audience]['option_id']);
    }
    $option_set->targeting[$new_audience]['osid'] = $nested_osid;
  }
}

/**
 * Deletes a nested test that is no longer needed.
 *
 * @param $option_set
 *   The option set whose targeting is being changed.
 * @param $audience
 *   THe name of the target audience the test was assigned to.
 * @throws \PersonalizeException
 *
 * @return string
 *   The machine name of the agent that was deleted so it can be scheduled for
 *   deletion in Lift.
 */
function acquia_lift_delete_old_nested_test(&$option_set, $audience) {
  $agent_to_delete = NULL;
  // Find the nested option set for the specified targeting rule.
  if (isset($option_set->targeting[$audience]) && isset($option_set->targeting[$audience]['osid'])) {
    $nested_os = personalize_option_set_load($option_set->targeting[$audience]['osid']);
    // Delete the option set, then delete the agent.
    personalize_option_set_delete($nested_os->osid);
    $agent_to_delete = $nested_os->agent;
    personalize_agent_delete($agent_to_delete);
    unset($option_set->targeting[$audience]['osid']);
  }
  return $agent_to_delete;
}

/**
 * Saves the targeting structure defined via the UI.
 *
 * This does not actually *create* the structure defined via the UI as that
 * happens once the final review stage has been completed.
 *
 * @param $agent
 *   A stdClass object representing the agent to save the targeting structure for.
 * @param $targeting
 *   An array keyed by target audience with arrays of option IDs as values.
 * @throws \AcquiaLiftException
 */
function acquia_lift_save_targeting_structure($agent, $targeting) {
  if ($agent->plugin != 'acquia_lift_target') {
    throw new AcquiaLiftException('Invalid agent');
  }
  $option_set = acquia_lift_get_option_set_for_targeting($agent->machine_name);
  // Check that the targeting rules for the specified audiences exist in the
  // option set.
  foreach ($targeting as $audience => $vars) {
    if (!isset($option_set->targeting[$audience])) {
      throw new AcquiaLiftException('Invalid audience');
    }
  }
  $agent->data['lift_targeting'] = $targeting;
  personalize_agent_save($agent);
}

/**
 * Returns a mapping of audiences to option IDs based on the targeting set-up.
 *
 * Returns an array of option IDs for each audience, so if the audience is
 * assigned a single option, then it's a single element array, if it's assigned
 * a nested option set, we pull out the option IDs for the nested option set and
 * return them as an array.
 *
 * @param $option_set
 *   The option set to get the targeting structure for.
 * @return array
 *   An associative array whose keys are target audiences and whose values are
 *   arrays of option IDs.
 */
function acquia_lift_get_structure_from_targeting($option_set) {
  if (empty($option_set->targeting)) {
    return array();
  }
  $targeting_structure = array();
  foreach ($option_set->targeting as $name => $targ) {
    $targeting_structure[$name] = array();
    if (isset($targ['osid'])) {
      $nested_option_set = personalize_option_set_load($targ['osid']);
      foreach ($nested_option_set->options as $option) {
        $targeting_structure[$name][] = $option['option_id'];
      }
    }
    elseif (isset($targ['option_id'])) {
      $targeting_structure[$name][] = $targ['option_id'];
    }
  }
  return $targeting_structure;
}

/**
 * Saves the current test options on each nested test.
 *
 * @param stdClass $parent_agent
 *   The parent targeting agent that may contain nested tests to update.
 */
function acquia_lift_save_nested_test_options($parent_agent) {
  $option_set = acquia_lift_get_option_set_for_targeting($parent_agent->machine_name);
  if (!isset($option_set->targeting)) {
    return;
  }
  $agent_data = array(
    'control_rate' => isset($parent_agent->data['control_rate']) ? $parent_agent->data['control_rate'] : 10,
    'explore_rate' => isset($parent_agent->data['explore_rate']) ? $parent_agent->data['explore_rate'] : 20,
    'decision_style' => isset($parent_agent->data['decision_style']) ? $parent_agent->data['decision_style'] : 'adaptive',
    'cache_decisions' => isset($parent_agent->data['cache_decisions']) ? $parent_agent->data['cache_decisions'] : 1,
  );

  foreach ($option_set->targeting as $vars) {
    if (!isset($vars['osid'])) {
      continue;
    }
    $nested_option_set = personalize_option_set_load($vars['osid']);
    $nested_agent = personalize_agent_load($nested_option_set->agent);
    $nested_agent->data = $agent_data;
    personalize_agent_save($nested_agent);
  }
}

/**
 * Validate that an agent has the minimum targeting required and add it if it
 * is not available.
 *
 * Targeting agents should have a default "Everyone else" audience that has all
 * unassigned variations assigned.  Variations should only be automatically
 * added when the agent has not been started.
 *
 * @param stdClass $agent
 *   The object representing the agent that owns the targeting option set.
 */
function acquia_lift_validate_minimum_targeting($agent, $option_set) {
  if (personalize_agent_get_status($agent->machine_name) !== PERSONALIZE_STATUS_NOT_STARTED) {
    return;
  }
  // Check and create the "Everyone else" audience if it doesn't exist.
  $targeting_option_set = acquia_lift_get_option_set_for_targeting($agent->machine_name);
  if ($targeting_option_set->osid == $option_set->osid) {
    $targeting_option_set = $option_set;
  }
  if (empty($targeting_option_set->targeting)) {
    $audience = _acquia_lift_personalize_campaign_wizard_everyone_else_audience();
    acquia_lift_target_audience_save($audience['name'], $agent->machine_name, $audience['contexts'], $audience['strategy'], $audience['weight'], $audience['id']);
    // Indicate that there are no variations assigned.
    $assigned = array();
  }
  else {
    // Find all of the options that are assigned to other audiences.  This will
    // be stored in the agent data property until the agent has been verified
    // to start.  After that time changes shouldn't be made automatically.
    $assigned = array();
    $targeting = !empty($agent->data['lift_targeting']) ? $agent->data['lift_targeting'] : acquia_lift_get_structure_from_targeting($option_set);
    foreach ($targeting as $audience_id => $options) {
      $assigned = array_merge($assigned, $options);
    }
  }
  $targeting_options = array();
  foreach ($targeting_option_set->options as $option) {
    $targeting_options[] = $option['option_id'];
  }
  // Any unassigned option is automatically assigned to the "Everyone else"
  // category.
  $unassigned_options = array_diff($targeting_options, $assigned);
  if (!empty($unassigned_options)) {
    acquia_lift_save_targeting_structure($agent, array(ACQUIA_LIFT_TARGETING_EVERYONE_ELSE => $unassigned_options));
  }

}

/**
 * Update the status of an acquia lift target agent and synchronize any
 * necessary configuration to Lift.
 *
 * @param stdClass $agent_data
 *   The agent to update and synchronize if necessary.
 * @param $next_status
 *   The new status for the agent.
 * @return bool
 *   True if the synchronization and status changes were successful.
 */
function acquia_lift_target_set_status($agent_data, $next_status) {
  if ($agent_data->plugin !== 'acquia_lift_target') {
    personalize_agent_set_status($agent_data->machine_name, $next_status);
    return TRUE;
  }
  if ($next_status == PERSONALIZE_STATUS_RUNNING || $next_status == PERSONALIZE_STATUS_SCHEDULED) {
    module_load_include('inc', 'acquia_lift', 'acquia_lift.batch');
    try {
      $agent_data->tests_to_delete = acquia_lift_implement_test_structure($agent_data);
      if (acquia_lift_batch_sync_tests_for_agent($agent_data)) {
        personalize_agent_set_status($agent_data->machine_name, $next_status);
        // Unset the started property which will have been changed during
        // status update and we don't want it getting clobbered by other
        // saves of the agent.
        unset($agent_data->started);
        return TRUE;
      }
      else {
        drupal_set_message(t('There was a problem syncing your personalization components to Lift and it cannot be started at this time.'), 'error');
        return FALSE;
      }
    }
    catch (Exception $e) {
      return FALSE;
    }
  }
  else {
    return personalize_agent_set_status($agent_data->machine_name, $next_status);
  }
}

/**
 * Sets up a multivariate test as defined by the variation sets created.
 */
function acquia_lift_implement_mvt($agent) {
  // There will only ever be one nested test for this campaign and it follows a
  // naming convention (e.g. "{parent-campaign-name}-mvt"
  $mvt_name = acquia_lift_get_mvt_name_for_agent($agent->machine_name);
  // If this is our first time implementing this MVT, then the option sets will
  // still be on the parent personalization and we'll need to move them.
  $option_sets = personalize_option_set_load_by_agent($agent->machine_name);
  if (empty($option_sets)) {
    $option_sets = personalize_option_set_load_by_agent($mvt_name);
  }
  if (count($option_sets) < 2) {
    throw new AcquiaLiftException('Cannot implement a multi-variate test with fewer than 2 variation sets');
  }
  $tests_to_delete = array();
  $goals = array();
  if ($mvt = personalize_agent_load($mvt_name)) {
    $old_goals = personalize_goal_load_by_conditions(array('agent' => $mvt_name));
    foreach ($old_goals as $goal) {
      // This goal will get deleted so we'll just need to recreate it on the new
      // agent.
      $goal->id = NULL;
      $goals[] = $goal;
    }
    // If this agent already exists delete it and recreate from scratch.
    personalize_agent_delete($mvt_name);
    $tests_to_delete = array($mvt_name);
  }

  // Create a test agent for the MVT.
  $mvt_agent = new stdClass();
  $mvt_agent->label = 'MVT for ' . $agent->label;
  $mvt_agent->plugin = acquia_lift_get_test_agent_plugin();
  $mvt_agent->data = array();
  $mvt_agent->machine_name = $mvt_name;
  try {
    personalize_agent_save($mvt_agent);
    personalize_agent_set_status($mvt_name, PERSONALIZE_STATUS_RUNNING);
    // Ensure the option sets are attached to the mvt, not the parent.
    foreach ($option_sets as $os) {
      $os->agent = $mvt_name;
      $os->mvt = $mvt_name;
      personalize_option_set_save($os);
    }
    // Ensure goals are attached to the mvt, not the parent.
    $new_goals = personalize_goal_load_by_conditions(array('agent' => $agent->machine_name));
    $goals += array_values($new_goals);
    foreach ($goals as $goal) {
      personalize_goal_save($mvt_name, $goal->action, $goal->value, $goal->id);
    }
  }
  catch (PersonalizeException $e) {

  }
  return $tests_to_delete;
}

/**
 * Check to see if an acquia lift target agent allows changes that would affect
 * the definition of the agent or any nested agents.
 *
 * @param stdClass $agent_data
 *   The agent to check.
 * @return bool
 *   True if changes can be made, false otherwise.
 */
function acquia_lift_target_definition_changes_allowed($agent_data) {
  if ($agent_data->plugin !== 'acquia_lift_target') {
    return TRUE;
  }
  $status = personalize_agent_get_status($agent_data->machine_name);
  return $status == PERSONALIZE_STATUS_NOT_STARTED || $status == PERSONALIZE_STATUS_PAUSED;
}

/**
 * Checks to see if an option within an option set is currently targeted.
 *
 * @param string $agent_name
 *   The name of the agent to check.
 * @param string $option_id
 *   The option id to check.
 * @return bool
 *   True if the option set is targeted, false otherwise.
 */
function acquia_lift_target_option_targeted($agent_name, $option_id) {
  // We only care about saved targeting that has been created.
  $option_set = acquia_lift_get_option_set_for_targeting($agent_name);
  $targeting = acquia_lift_get_structure_from_targeting($option_set);
  foreach ($targeting as $options) {
    if (in_array($option_id, $options)) {
      return TRUE;
    }
  }
  return FALSE;
}

/**
 * Checks to see if any options within an option set are currently targted.
 *
 * @param string $agent_name
 *   The name of the agent to check.
 * @param string $osid
 *   The id of the option set to check.
 * @return bool
 *   True if any option in the option set is targeted, false if none of the
 *   options are used in explicit targeting.
 */
function acquia_lift_target_option_set_targeted($agent_name, $option_set) {
  // We only care about saved targeting that has been created.
  $targeting_option_set = acquia_lift_get_option_set_for_targeting($agent_name);
  $targeting = acquia_lift_get_structure_from_targeting($targeting_option_set);
  $targeted_options = array();
  foreach($targeting as $options) {
    $targeted_options += $options;
  }
  foreach ($option_set->options as $option) {
    if (in_array($option['option_id'], $targeted_options)) {
      return TRUE;
    }
  }
  return FALSE;
}

/**
 * Return the default values used for an "Everyone else" audience.
 */
function _acquia_lift_personalize_campaign_wizard_everyone_else_audience() {
  return array(
    'name' => t('Everyone else'),
    'contexts' => array(),
    'weight' => 1,
    'strategy' => 'OR',
    'id' => ACQUIA_LIFT_TARGETING_EVERYONE_ELSE,
  );
}

/**
 * Page callback for the legacy campaign listing.
 */
function acquia_lift_legacy_campaign_list() {
  $legacy_campaigns = variable_get('acquia_lift_legacy_agents', array());
  if (empty($legacy_campaigns)) {
    drupal_goto('admin/structure/personalize');
  }
  $agents = personalize_agent_load_multiple($legacy_campaigns);
  module_load_include('inc', 'personalize', 'personalize.admin');
  return personalize_agent_list($agents, FALSE, FALSE);
}
