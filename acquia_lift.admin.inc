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
    '#markup' => t("<p>This information is used to link your !acquialift account to Drupal.</p><p>Email !liftemail to get set up with Acquia Lift credentials.</p>", array('!acquialift' => l(t('Acquia Lift'), 'http://www.acquia.com/products-services/website-personalization', array('attributes' => array('target' => '_blank'))), '!liftemail' => l('lift@acquia.com', 'mailto:lift@acquia.com'))),
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
    '#markup' => '<label for="acquia_lift_min_runtime_num">' . t('Minimum Time to Run') . '<span class="form-required" title="This field is required.">*</span></label>',
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
    '#markup' => '<div class="description">' . t('The minimum amount of time a campaign should be left to run for. Only affects campaigns that are set to stop automatically when a winner is found.') . '</div>',
  );
  $form['acquia_lift_min_decisions'] = array(
    '#type' => 'textfield',
    '#title' => t('Minimum number of decisions'),
    '#size' => 3,
    '#required' => TRUE,
    '#default_value' => variable_get('acquia_lift_min_decisions', 1000),
    '#description' => t('Minimum number of decisions before campaign can be stopped automatically. Only affects campaigns that are set to stop automatically when a winner is found.'),
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
    '#markup' => '<div>' . t('Sends your local campaign information to the hosted Acquia Lift service. Use this feature if you change your Acquia Lift credentials after creating one or more campaigns.') . '</div>'
  );
  return $form;
}

/**
 * Submit callback for the batch sync form.
 */
function acquia_lift_batch_sync_form_submit($form, &$form_state) {
  module_load_include('inc', 'acquia_lift', 'acquia_lift.batch');
  acquia_lift_batch_sync_campaigns();
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
  acquia_lift_ensure_default_agent($form_state['values']['acquia_lift_account_info']);

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
function acquia_lift_report($form, &$form_state, $agent_data, $option_set) {
  if (!$agent = personalize_agent_load_agent($agent_data->machine_name)) {
    return array();
  }
  if (!$agent instanceof AcquiaLiftAgent) {
    return array();
  }
  if ($agent_data->started == 0) {
    return array(
      'no_report' => array(
        '#markup'=> t('This agent has not started running yet, no reports to show.')
      )
    );
  }

  // If this agent is not currently enabled in Acquia Lift, there are no reports
  // to show.
  $errors = $agent->errors();
  if (!empty($errors)) {
    return array(
      'no_report' => array(
        '#markup'=> t('This agent is not properly configured, no reports to show.')
      )
    );
  }
  // If this agent doesn't implement the reporting interface then there are no
  // reports to show.
  if (!$agent instanceof PersonalizeAgentReportInterface) {
    return array(
      'no_report' => array(
        '#markup' => t('This agent does not support reporting.')
      )
    );
  }

  if ($agent instanceof AcquiaLiftSimpleAB) {
    return acquia_lift_report_ab($form, $form_state, $agent, $agent_data);
  }
  else {
    return acquia_lift_report_custom($form, $form_state, $agent, $agent_data, $option_set);
  }
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
 *   The plugin agent for the selected campaign.
 * @param stdClass $agent_data
 *   The campaign agent data for this report.
 * @param stdClass $option_set
 *   (optional) The content variation to show in the default view.
 */
function acquia_lift_report_custom($form, &$form_state, $agent, $agent_data, $option_set) {
  // Check for Rickshaw and D3 libraries and alert users if not exist.
  if (_acquia_lift_missing_library_warning(array('rickshaw', 'd3'), t('The following libraries are required in order to view the Acquia Lift reports:'))) {
    return array();
  }

  // Generate report filters.
  $data = $agent->getData();
  $form = array(
    '#prefix' => '<div id="acquia-lift-reports">',
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
  $decisions = AcquiaLiftAgent::convertOptionSetsToDecisions($data['decisions']);
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
  $form['report_filters']['decision_point'] = acquia_lift_report_decision_point_dropdown($decision_options, $decision_point);

  // Default to showing the complete history of the campaign.
  $date_start_report = date('Y-m-d', $agent_data->started);
  $date_end_report = $date_end_targeting = date('Y-m-d');

  // If the campaign's status is "completed" then we need to use the date it ended.
  $agent_name = $agent->getMachineName();
  $status = variable_get(_personalize_agent_get_status_variable($agent_name));
  if ($status == PERSONALIZE_STATUS_COMPLETED && $end_time = variable_get(_personalize_agent_get_stoptime_variable($agent_name), 0)) {
    $date_end_report = $date_end_targeting = date('Y-m-d', $end_time);
  }
  // Conversion report filters.
  $selected_goal = empty($form_state['values']['goal']) ? NULL : $form_state['values']['goal'];
  $selected_metric = empty($form_state['values']['metric']) ? 'rate' : $form_state['values']['metric'];

  $options = array(
    'decision' => $decision_point,
    'start' => $date_start_report,
    'end' => $date_end_report,
    'goal' => $selected_goal,
    'conversion_metric' => $selected_metric,
  );

  $reports = $agent->buildCampaignReports($options);

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

  // The context and stability reports are only relevant if there is context
  // targeting in place for this campaign.
  if (empty($reports['targeting'])) {
    return $form;
  }

  // Context report section.
  $context_select = $reports['targeting'];
  acquia_lift_chosenify_element($context_select, array('acquia-lift-chosen-select-half', 'acquia-lift-report-context-select'));
  $form['context_report'] = array(
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
          '#markup' => '<h2>' . t('Context') . '</h2>',
        ),
      ),
      'summary' => array(
        '#type' => 'container',
        '#attributes' => array(
          'class' => array('acquia-lift-report-header-summary'),
        ),
      ),
    ),
    '#attributes' => array(
      'id' => 'acquia-lift-context-report',
      'class' => array('acquia-lift-report-section'),
    )
  );
  if (!empty($report_title_additional)) {
    $form['context_report']['header']['title']['groups'] = array(
      '#markup' => t('random and personalized groups'),
    );
    $form['context_report']['header']['summary']['distribution'] = array(
      '#markup' => $report_title_additional,
      '#theme_wrappers' => array('container'),
    );
  }
  $form['context_report']['header']['summary']['report_summary'] = array(
    '#theme_wrappers' => array('container'),
    '#markup' => t('See who converts best for each content variation'),
    '#attributes' => array(
      'class' => array('acquia-lift-report-summary'),
    ),
  );
  $form['context_report']['header']['controls'] = array(
    '#type' => 'container',
    '#attributes' => array(
      'class' => array('acquia-lift-report-controls'),
    ),
    'context' => $context_select,
  );
  $form['context_report']['report'] = array(
    '#markup' => drupal_render($reports['context']),
    '#theme_wrappers' => array('container'),
    '#id' => 'acquia-lift-context-report-data',
  );

  // Stability report section.
  $context_select = $reports['targeting'];
  acquia_lift_chosenify_element($context_select, array('acquia-lift-chosen-select-half', 'acquia-lift-report-context-select'));
  $form['advanced_reports'] = array(
    '#type' => 'fieldset',
    '#title' => t('Advanced reporting'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $form['advanced_reports']['stability_report'] = array(
    '#type' => 'container',
    'header' => array(
      '#type' => 'container',
      '#attributes' => array('acquia-lift-report-section-header', 'clearfix'),
      'title' => array(
        '#type' => 'container',
        '#attributes' => array(
          'class' => array('acquia-lift-report-section-title'),
        ),
        'report_title' => array(
          '#markup' => '<h2>' . t('Context Stability') . '</h2>',
        ),
      ),
      'summary' => array(
        '#type' => 'container',
        '#attributes' => array(
          'class' => array('acquia-lift-report-header-summary'),
        ),
      ),
    ),
    '#attributes' => array(
      'id' => 'acquia-lift-stability-report',
      'class' => array('acquia-lift-report-section'),
    ),
  );
  if (!empty($report_title_additional)) {
    $form['advanced_reports']['stability_report']['header']['title']['groups'] = array(
      '#markup' => t('random and personalized groups'),
    );
    $form['advanced_reports']['stability_report']['header']['summary']['distribution'] = array(
      '#markup' => $report_title_additional,
      '#theme_wrappers' => array('container'),
    );
  }
  $form['advanced_reports']['stability_report']['header']['controls'] = array(
    '#type' => 'container',
    '#attributes' => array(
      'class' => array('acquia-lift-report-controls'),
    ),
    'context' => $context_select,
  );
  $form['advanced_reports']['stability_report']['report'] = array(
    '#markup' => drupal_render($reports['stability']),
    '#theme_wrappers' => array('container'),
    '#id' => 'acquia-lift-stability-report-data',
  );

  // We have to specify the include file so as not to lose it during rendering from ajax.
  // @see drupal_retrieve_form():734
  $form_state['build_info']['files'] = array(
    drupal_get_path('module', 'acquia_lift') . '/acquia_lift.admin.inc',
    drupal_get_path('module', 'acquia_lift') . '/acquia_lift.ui.inc',
  );
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
function acquia_lift_report_decision_point_dropdown($options, $selected) {
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
      'wrapper' => "acquia-lift-reports",
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
 * Form build function for an A/B Acquia Lift agent report.
 *
 * @param AcquiaLiftAgentInterface $agent
 *   The plugin agent for the selected campaign.
 * @param stdClass $agent_data
 *   The campaign agent data for this report.
 */
function acquia_lift_report_ab($form, &$form_state, $agent, $agent_data) {
  $agent_name = $agent->getMachineName();
  // Check for Rickshaw and D3 libraries and alert users if not exist.
  if (_acquia_lift_missing_library_warning(array('rickshaw', 'd3'), t('The following libraries are required in order to view the Acquia Lift reports:'))) {
    return array();
  }
  $selected_goal = empty($form_state['values']['goal']) ? NULL : $form_state['values']['goal'];
  $selected_metric = empty($form_state['values']['metric']) ? 'rate' : $form_state['values']['metric'];

  $build = array();
  $build['options'] = array(
    '#type' => 'container',
    '#attributes' => array(
      'class' => array('acquia-lift-report-section-options'),
    ),
    '#tree' => FALSE,
    'goal' => acquia_lift_report_goal_dropdown($agent_name, $selected_goal),
    'metric' => acquia_lift_report_conversion_metric_dropdown($selected_metric),
    'submit' => array(
      '#type' => 'submit',
      '#value' => t('Filter'),
    ),
  );

  // Default to showing the complete history of the campaign.
  $date_start_report = date('Y-m-d', $agent_data->started);
  $date_end_report = $date_end_targeting = date('Y-m-d');

  // If the campaign's status is "completed" then we need use the date it ended.
  $status = variable_get(_personalize_agent_get_status_variable($agent_name));
  if ($status == PERSONALIZE_STATUS_COMPLETED && $end_time = variable_get(_personalize_agent_get_stoptime_variable($agent_name), 0)) {
    $date_end_report = $date_end_targeting = date('Y-m-d', $end_time);
  }
  // Acquia Lift A/B reports have a decision name that is the same as the
  // campaign machine name.
  $options = array(
    'decision' => $agent_data->machine_name,
    'start' => $date_start_report,
    'end' => $date_end_report,
  );
  $build['reports'] = $agent->buildCampaignReports($options);
  $build['reports']['#attached']['library'][] = array('acquia_lift', 'acquia_lift.reports');

  return $build;
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

  if (!acquia_lift_owns_agent($agent)) {
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
 *  C A M P A I G N  M A N A G E M E N T  F L O W
 * =======================================================================
 */

/**
 * Page callback to start the create a campaign process.  The first step is to select
 * the type of
 * @param bool $ajax
 *   Indicates if form is called with ajax support.
 */
function acquia_lift_create_campaign_callback($ajax) {
  if (!$ajax) {
    drupal_goto('admin/structure/personalize/add');
    return;
  }
  // Show the campaign type selection form in a modal.
  ctools_include('modal');
  ctools_modal_add_js();

  $path = drupal_get_path('module', 'acquia_lift');
  $output = theme('acquia_lift_type_list', array(
    'id' => 'acquia-lift-modal-campaign-type-list',
    'items' => array(
      'ab' => array(
        'title' => t('A/B test'),
        'description' => t('Create and compare content variations.'),
        'url' => 'admin/structure/acquia_lift/add/ab',
        'modal' => TRUE,
        'ctools_style' => 'ctools-modal-acquia-lift-style',
        'logo' => theme('image', array(
          'path' => $path . '/images/campaign-type-ab.png',
          'alt' => t('A/B test'),
          'title' => t('Select this option to create an A/B test.'),
        )),
      ),
      'custom' => array(
        'title' => t('Custom Lift campaign'),
        'description' => t('Create a content targeting or multivariate campaign using the Drupal-integrated campaign tools.'),
        'url' => 'admin/structure/personalize/add/',
        'modal' => FALSE,
        'logo' => theme('image', array(
          'path' => $path . '/images/campaign-type-custom.png',
          'alt' => t('Custom Lift campaign'),
          'title' => t('Select this option to create a custom Lift campaign.'),
        ))
      ),
    ),
  ));
  return ctools_modal_render(t('Create a campaign'), $output);
}

/**
 * Page callback to generate the ctools modal form to create a campaign
 * of a specific type.
 *
 * @param $type
 *   The type of campaign to create.  Current supported choices:  'ab'
 * @param $ajax
 *   Indicates if AJAX is supported
 */
function acquia_lift_create_campaign_type_callback($type, $ajax) {
  // If the campaign type is not supported or AJAX is not supported then go to the full campaign
  // creation process.
  if (!$ajax || !in_array($type, array('ab'))) {
    drupal_goto('admin/structure/personalize/add');
    return;
  }
  acquia_lift_create_ctools_form(
    t('Create a campaign'),
    'acquia_lift_create_campaign_type_form',
    array('type' => $type),
    'acquia_lift_create_campaign_type_complete_callback'
  );
}

/**
 * Ctools form processing complete handler for the creation of a new campaign
 * by type.
 * @see acquia_lift_create_ctools_form().
 */
function acquia_lift_create_campaign_type_complete_callback($form, $form_state) {
  $agent_name = personalize_get_campaign_context();
  return array(
    ctools_modal_command_dismiss(),
    ajax_command_settings(array('personalize' => array(
      'activeCampaign' => $agent_name,
      'campaigns' => personalize_agent_get_settings(),
      'agent_map' => array(
        $agent_name => personalize_agent_get_map_settings($agent_name),
      )
    )), TRUE),
    acquia_lift_command_messagebox(t('Click the element you want to change in %variation.', array(
      '%campaign' => $form_state['values']['agent_basic_info']['title'],
      '%variation' => 'Variation #1',
    )), 10),
    acquia_lift_command_page_variation_toggle(),
    // @todo add this back in when the campaigns can be processed along with
    // variation set creation.
    // acquia_lift_command_process_queue(),
  );
}

/**
 * Form builder function to create a new campaign in the modal process
 * of a specific type.
 *
 * @param $type
 *   The type of campaign to create.  Current supported choices: 'ab'
 */
function acquia_lift_create_campaign_type_form($form, &$form_state, $type) {
  ctools_include('modal');
  ctools_include('ajax');

  ctools_add_js('ajax-responder');

  $change_link = ctools_modal_text_button(t('Change type of test'), 'admin/structure/personalize/add/nojs', t('Change type of test'), 'ctools-modal-acquia-lift-style');
  // Add individual forms specific to the type of campaign being created.
  switch ($type) {
    case 'ab':
      $form['campaign_type'] = array(
        '#markup' => theme('acquia_lift_create_type_change', array(
          'type' => t('A/B test'),
          'change_link' => $change_link,
        )),
      );
      $form['agent'] = _acquia_lift_create_simplified_campaign_form('acquia_lift_simple_ab');
      break;
  }
  if (empty($form['agent'])) {
    // There isn't any agent data to act upon.
    return $form;
  }
  // Add common actions.
  $form['actions']['submit_form'] = array(
    '#type' => 'submit',
    '#attributes' => array(
      'class' => array('action-item-primary-active', 'acquia-lift-submit-button'),
    ),
    '#value' => t('Create campaign'),
  );
  $form['actions']['reset'] = array(
    '#markup' => ctools_ajax_text_button(t('Cancel'), 'admin/structure/acquia_lift/cancel/nojs', t('Cancel'), 'acquia-lift-cancel-button'),
  );
  return $form;
}

/**
 * Loads the simplified campaign creation form for an A/B test with an
 * Acquia Lift agent.
 */
function _acquia_lift_create_simplified_campaign_form($agent_type) {
  ctools_include('plugins');

  if ($agent_class = ctools_plugin_load_class('personalize', 'agent_type', $agent_type, 'handler')) {
    $method = is_subclass_of($agent_class, 'AcquiaLiftSimplifiedAgentInterface') ? 'simplifiedForm' : 'optionsForm';
    $options_form = call_user_func_array(array($agent_class, $method), array(array()));
  }
  else {
    drupal_set_message(t('There is a problem accessing the Acquia Lift agent. Acquia Lift campaigns can not be created until you configure your account info !here', array('!here' => l('here', 'admin/config/content/personalize/acquia_lift'))), 'error');
    return array();
  }
  // Recreate a basic campaign form with the appropriate AB options.
  // @see personalize_agent_build_basic_form.
  $form['agent_basic_info'] = array(
    '#tree' => TRUE,
  );
  $form['agent_basic_info']['title'] = array(
    '#title' => t('Name'),
    '#type' => 'textfield',
    '#required' => TRUE,
  );
  $form['agent_basic_info']['agent_type'] = array(
    '#type' => 'value',
    '#value' => $agent_type,
  );
  $form['agent_basic_info']['options'] = array(
    '#tree' => TRUE,
  );
  $form['agent_basic_info']['options'][$agent_type] = $options_form;
  // Defaults to caching decisions.
  $form['cache_decisions'] = array(
    '#type' => 'value',
    '#value' => 1,
  );
  return $form;
}

function acquia_lift_create_campaign_type_form_validate($form, &$form_state) {
  $form_state['values']['agent_basic_info']['machine_name'] = personalize_generate_machine_name($form_state['values']['agent_basic_info']['title'], 'personalize_agent_machine_name_exists');
  module_load_include('inc', 'personalize', 'personalize.admin');
  personalize_agent_form_validate($form, $form_state);
}

/**
 * Submit handler to create a campaign from the Acquia Lift UI flow.
 */
function acquia_lift_create_campaign_type_form_submit($form, &$form_state) {
  module_load_include('inc', 'personalize', 'personalize.admin');
  $agent_data = _personalize_agent_from_form_values($form_state['values']['agent_basic_info']);
  $agent_data->data['cache_decisions'] = $form_state['values']['cache_decisions'];
  $agent = personalize_agent_save($agent_data);
  personalize_set_campaign_context($agent->machine_name);
  // @todo find out how to remove this in favor of asynchronous processing.
  acquia_lift_process_queue(FALSE);
}

/**
 * Form handler for a page variation details form.
 *
 * @param string $variation_type
 *   The variation type that defines the type of details to include.  This
 *   is a key defined as a personalize_elements variation.
 * @see hook_personalize_elements_variation_types
 */
function acquia_lift_page_variation_details_form($form, &$form_state, $variation_type) {
  $types = module_invoke_all('personalize_elements_variation_types');
  if (!isset($types[$variation_type]['contextual']['formitem'])) {
    $form['error'] = array(
      '#markup' => t('Invalid page variation type.  Please close this message and create a new variation.'),
    );
    return $form;
  }
  $form['variation_type'] = array(
    '#type' => 'value',
    '#value' => $variation_type,
  );
  // These form item will be populated by the front-end code.
  $form['selector'] = array(
    '#type' => 'textfield',
    '#title' => t('Selector'),
    '#required' => TRUE,
    '#maxlength' => 500,
  );
  $form['pages'] = array(
    '#type' => 'hidden',
  );
  $form['agent'] = array(
    '#type' => 'hidden',
  );
  $form['variation_number'] = array(
    '#type' => 'hidden',
  );
  // Now include the details specific to this variation type.
  $form['personalize_elements_content'] = $types[$variation_type]['contextual']['formitem'];
  $form['actions'] = array(
    '#type' => 'actions',
  );
  // Form actions.
  // TRICKY: These must have ids that are different from our CTools loaded
  // modal forms or else their submit buttons will be processed via Drupal ajax
  // rather than the main URL callbacks leading to a 200 response with an
  // empty response body.
  $form['actions']['variation_type_submit_form'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
    '#attributes' => array(
      'class' => array('action-item-primary-active', 'acquia-lift-submit-button'),
    ),
    '#ajax' => array(
      'callback' => 'acquia_lift_page_variation_details_form_ajax_callback',
      'wrapper' => 'acquia-lift-page-variation-details-form',
      'progress' => array(
        'message' => NULL,
        'type' => 'throbber',
      ),
    ),
  );
  // Add a cancel button for AJAX-loaded forms.
  $form['actions']['variation_type_cancel_form'] = array(
    '#type' => 'button',
    '#button_type' => 'cancel',
    '#value' => t('Cancel'),
    '#attributes' => array(
      'class' => array('acquia-lift-cancel-button'),
    ),
    '#ajax' => array(
      'callback' => 'acquia_lift_page_variation_details_form_ajax_callback',
      'wrapper' => 'acquia-lift-page-variation-details-form',
      'progress' => array(
        'message' => NULL,
        'type' => 'throbber',
      ),
    ),
    // Turn off validation.
    '#limit_validation_errors' => array(),
    '#submit' => array(),
  );

  return $form;
}

/**
 * Form validation handler for acquia_lift_page_variation_details_form.
 */
function acquia_lift_page_variation_details_form_validate($form, &$form_state) {
  // Validating a CSS selector is generally beyond what REGEX can handle,
  // and using the Drupal cleaning scripts all update < > to entities
  // so just make sure that there are no tags.
  $re = "/<\\/?[a-z][a-z0-9]*[^<>]*>|<!--.*?-->/im";
  $matched = preg_match_all($re, $form_state['values']['selector']);
  if ($matched > 0) {
    form_set_error('selector', t('The selector cannot contain tags.'));
  }
}

/**
 * Form submit handler for acquia_lift_page_variation_details_form.
 *
 * This creates a page variation set if it does not yet exist and the variation
 * for that page.  For a simple A/B test, there is only a single variation set
 * allowed per campaign and therefore it is named for the campaign/agent.
 *
 * A variation set is similar to a decision name concept and groups together
 * option sets for a particular agent to be considered as a group.
 * A variation is comprised of all of the options of the same index across
 * all option sets in a variation set (decision name).
 * In other words, an option set handles the changes for a single element of
 * the variation and includes how that element is shown for each variation as
 * its options.
 */
function acquia_lift_page_variation_details_form_submit($form, &$form_state) {
  $agent_name = $variation_set_name = $form_state['values']['agent'];
  $option_set = personalize_elements_get_option_set_for_variation($variation_set_name, $agent_name, $form_state['values']['selector'], $form_state['values']['variation_type'], $form_state['values']['pages']);

  $variation_number = NULL;
  if (is_numeric($form_state['values']['variation_number']) && $form_state['values']['variation_number'] >= 0) {
    $variation_number = $form_state['values']['variation_number'];
  }
  $variation_number = acquia_lift_page_variation_create($variation_set_name, $option_set, array('personalize_elements_content' => $form_state['values']['personalize_elements_content']), $variation_number);
  // Save this for the Ajax callback.
  $form_state['storage']['variation_number'] = $variation_number;
}

/**
 * Responds to AJAX submission of a variation type details page.
 */
function acquia_lift_page_variation_details_form_ajax_callback($form, &$form_state) {
  $op = isset($form_state['values']['op']) ? $form_state['values']['op'] : '';
  $commands = array();
  if ($op == t('Save')) {
    $errors = form_get_errors();
    // If validation errors exist, return the form.
    if (!empty($errors)) {
      return $form;
    }
    // Get the updated option sets for the page variation.
    $variation_set_name = $agent_name = $form_state['values']['agent'];
    $option_sets = personalize_option_set_load_multiple(array(), array('decision_name' => $variation_set_name));
    $option_set_settings = array_map('_personalize_convert_option_set_to_js_setting', $option_sets);
    foreach($option_set_settings as $option_set_setting) {
      $settings['personalize']['option_sets'][key($option_set_setting)] = reset($option_set_setting);
    }
    // Also get the updated personalize elements options.
    personalize_elements_limit_option_sets_by_path($option_sets, $form_state['values']['pages']);
    $settings['personalize_elements']['elements'] = personalize_elements_element_settings($option_sets);
    // We need to update the campaigns as well in case a campaign was paused by this action.
    $settings['personalize']['campaigns'] = personalize_agent_get_settings();
    // Note: JavaScript assets for personalize elements are included in
    // personalize_elements_page_build when the module is enabled and therefore
    // do not need to be explicitly added here.
    $commands[] = ajax_command_settings($settings, TRUE);
    // Determine the index to preview.  If creating a new variation, then the
    // first index is 1 because 0 is the control variation.
    $preview_variation_index = (!is_numeric($form_state['storage']['variation_number']) || $form_state['storage']['variation_number'] < 1) ? 1 : $form_state['storage']['variation_number'];
    $commands[] = acquia_lift_command_page_variation_preview($form_state['values']['agent'], $preview_variation_index);
    // Otherwise the form submitted so stop creating a page variation
    // and show a confirmation message.
    $commands[] = acquia_lift_command_page_variation_toggle(FALSE);
    // Show a confirmation message.
    $current_status = personalize_agent_get_status($agent_name);
    if ($current_status === PERSONALIZE_STATUS_NOT_STARTED) {
      if ($settings['personalize']['campaigns'][$agent_name]['verified']) {
        $message = t('Variation added.  You can now start the campaign.');
      }
      else {
        $message = t('The variation has been created. Add one or more goals by clicking Goals &gt; Add goal.');
      }
    }
    else {
      $message = t('The variation has been created.');
    }
    $commands[] = acquia_lift_command_messagebox($message, 10);
    $commands[] = acquia_lift_command_process_queue();
  }
  else if ($op === t('Cancel')) {
    // Turn off edit mode and redirect them.
    $commands[] = acquia_lift_command_page_variation_toggle(FALSE);
  }

  return personalize_ajax_commands_deliver($commands, TRUE);
}

/**
 * Page callback to generate a page variation rename modal window.
 *
 * @param string $agent_name
 *   The name of the campaign to edit.
 * @param string $variation_index
 *   The index of the variation to rename.
 */
function acquia_lift_page_variation_rename_callback($agent_name, $variation_index) {
  acquia_lift_create_ctools_form(
    t('Rename a variation'),
    'acquia_lift_page_variation_rename_form',
    array(
      'agent_name' => $agent_name,
      'variation_index' => $variation_index,
    ),
    'acquia_lift_page_variation_rename_complete_callback'
  );
}

/**
 * Ctools form processing complete handler for the renaming of a page
 * variation.
 * @see acquia_lift_create_ctools_form().
 */
function acquia_lift_page_variation_rename_complete_callback($form, $form_state) {
  $settings = array();
  $option_sets = $option_sets = personalize_option_set_load_multiple(array(), array('decision_name' => $form_state['values']['variation_set_name']));
  $option_set_settings = array_map('_personalize_convert_option_set_to_js_setting', $option_sets);
  foreach($option_set_settings as $option_set_setting) {
    $settings[key($option_set_setting)] = reset($option_set_setting);
  }
  // We need to update the campaigns as well in case a campaign was paused by this action.
  $campaign_settings['personalize']['campaigns'] = personalize_agent_get_settings();
  $commands = array(
    ctools_modal_command_dismiss(),
    acquia_lift_command_messagebox(t('The variation has been renamed.'), 10),
    acquia_lift_command_option_set_updates($settings),
    ajax_command_settings($campaign_settings, TRUE),
    acquia_lift_command_process_queue(),
  );
  return $commands;
}

/**
 * Form handler to generate a form to change a variation name.
 *
 * This form is called within a ctools modal window.
 *
 * @param string $agent_name
 *   The name of the campaign to edit.
 * @param string $variation_index
 *   The index of the variation to rename.
 */
function acquia_lift_page_variation_rename_form($form, &$form_state, $agent_name, $variation_index) {
  $variation_set_name = $agent_name;

  ctools_include('modal');
  ctools_include('ajax');
  ctools_add_js('ajax-responder');

  $form = array();
  $form['agent_name'] = array(
    '#type' => 'value',
    '#value' => $agent_name,
  );
  $form['variation_set_name'] = array(
    '#type' => 'value',
    '#value' => $variation_set_name,
  );
  $form['variation_index'] = array(
    '#type' => 'value',
    '#value' => $variation_index,
  );
  if ($variation_index == 0) {
    $form['message'] = array(
      '#markup' => t('The control variation cannot be renamed.'),
    );
    return $form;
  }
  $form['variation_name'] = array(
    '#type' => 'textfield',
    '#title' => t('New name'),
    '#default_value' => acquia_lift_page_variation_get_name($agent_name, $agent_name, $variation_index),
    '#required' => TRUE,
    '#maxlength' => PERSONALIZE_MACHINE_NAME_MAXLENGTH,
  );
  $form['actions']['submit_form'] = array(
    '#type' => 'submit',
    '#attributes' => array(
      'class' => array('action-item-primary-active', 'acquia-lift-submit-button'),
    ),
    '#value' => t('Rename variation'),
  );
  $form['actions']['reset'] = array(
    '#markup' => ctools_ajax_text_button(t('Cancel'), 'admin/structure/acquia_lift/cancel/nojs', t('Cancel'), 'acquia-lift-cancel-button'),
  );
  return $form;
}

/**
 * Submit handler for acquia_lift_page_variation_rename().
 */
function acquia_lift_page_variation_rename_form_submit($form, &$form_state) {
  acquia_lift_page_variation_rename($form_state['values']['variation_set_name'], $form_state['values']['agent_name'], $form_state['values']['variation_index'], $form_state['values']['variation_name']);
}

/**
 * Page callback to generate a page variation delete confirm modal window.
 *
 * @param string $agent_name
 *   The name of the campaign to edit.
 * @param string $variation_index
 *   The index of the variation to rename.
 */
function acquia_lift_page_variation_delete_callback($agent_name, $variation_index) {
  acquia_lift_create_ctools_form(
    t('Delete a variation'),
    'acquia_lift_page_variation_delete_form',
    array(
      'agent_name' => $agent_name,
      'variation_index' => $variation_index,
    ),
    'acquia_lift_page_variation_delete_complete_callback'
  );
}

/**
 * Ctools form processing complete handler for deletion of a page variation.
 * @see acquia_lift_create_ctools_form().
 */
function acquia_lift_page_variation_delete_complete_callback($form, $form_state) {
  $settings = array();
  $option_sets = personalize_option_set_load_multiple(array(), array('decision_name' => $form_state['values']['variation_set_name']));
  if (empty($option_sets)) {
    $settings['empty'] = $form_state['values']['agent_name'];
  }
  else {
    $option_set_settings = array_map('_personalize_convert_option_set_to_js_setting', $option_sets);
    foreach ($option_set_settings as $option_set_setting) {
      $settings[key($option_set_setting)] = reset($option_set_setting);
    }
  }
  $campaign_settings['personalize']['campaigns'] = personalize_agent_get_settings();
  $commands = array(
    ctools_modal_command_dismiss(),
    acquia_lift_command_messagebox(t('The variation has been deleted.'), 10),
    acquia_lift_command_option_set_updates($settings),
    ajax_command_settings($campaign_settings, TRUE),
    acquia_lift_command_process_queue(),
  );
  return $commands;
}

/**
 * Form handler to generate a confirmation form to delete a variation.
 *
 * This form is called within a ctools modal window.
 *
 * @param string $agent_name
 *   The name of the campaign to edit.
 * @param string $variation_index
 *   The index of the variation to rename.
 */
function acquia_lift_page_variation_delete_form($form, &$form_state, $agent_name, $variation_index) {
  $variation_set_name = $agent_name;
  $variation_name = acquia_lift_page_variation_get_name($variation_set_name, $agent_name, $variation_index);

  ctools_include('modal');
  ctools_include('ajax');
  ctools_add_js('ajax-responder');

  $form = array();
  $form['agent_name'] = array(
    '#type' => 'value',
    '#value' => $agent_name,
  );
  $form['variation_set_name'] = array(
    '#type' => 'value',
    '#value' => $variation_set_name,
  );
  $form['variation_index'] = array(
    '#type' => 'value',
    '#value' => $variation_index,
  );
  if ($variation_index == 0) {
    $form['message'] = array(
      '#markup' => t('The control variation cannot be renamed.'),
    );
    return $form;
  }
  $form['confirm'] = array(
    '#prefix' => '<div>',
    '#suffix' => '</div>',
    '#markup' => t('Are you sure you want to delete %variation?', array(
      '%variation' => $variation_name,
    )),
  );
  $form['actions']['submit_form'] = array(
    '#type' => 'submit',
    '#attributes' => array(
      'class' => array('action-item-primary-active', 'acquia-lift-submit-button'),
    ),
    '#value' => t('Delete variation'),
  );
  $form['actions']['reset'] = array(
    '#markup' => ctools_ajax_text_button(t('Cancel'), 'admin/structure/acquia_lift/cancel/nojs', t('Cancel'), 'acquia-lift-cancel-button'),
  );
  return $form;
}

/**
 * Submit handler for acquia_lift_page_variation_delete_form().
 */
function acquia_lift_page_variation_delete_form_submit($form, &$form_state) {
  acquia_lift_page_variation_delete($form_state['values']['variation_set_name'], $form_state['values']['agent_name'], $form_state['values']['variation_index']);
}

/**
 * Page callback to create a new goal by selecting the type of goal to create.
 *
 * @param bool $ajax
 *   Indicates if the form is called with ajax support.
 */
function acquia_lift_create_goal_callback($ajax) {
  if (!$ajax) {
    drupal_goto('admin/structure/personalize/goals');
    return;
  }

  // Show the goal type selection form in a modal.
  ctools_include('modal');
  ctools_modal_add_js();

  $path = drupal_get_path('module', 'acquia_lift');
  $output = theme('acquia_lift_type_list', array(
    'id' => 'acquia-lift-modal-goal-type-list',
    'items' => array(
      'existing' => array(
        'title' => t('Predefined goal'),
        'description' => t('Visitor actions already defined by Acquia Lift'),
        'url' => 'admin/structure/acquia_lift/goal/add/existing',
        'modal' => TRUE,
        'ctools_style' => 'ctools-modal-acquia-lift-style',
        'logo' => theme('image', array(
          'path' => $path . '/images/goal-type-predefined.png',
          'alt' => t('Pre-existing goal'),
          'title' => t('Select this option to create a goal from a pre-existing visitor action.'),
        )),
      ),
      'element' => array(
        'title' => t('New element goal'),
        'description' => t('Element-level visitor action, such as clicking a link or hovering over an image'),
        'url' => 'admin/structure/visitor_actions',
        'modal' => FALSE,
        'logo' => theme('image', array(
          'path' => $path . '/images/goal-type-element.png',
          'alt' => t('Element goal'),
          'title' => t('Select this option to create an element goal.'),
        ))
      ),
      'page' => array(
        'title' => t('New page goal'),
        'description' => t('Page-level visitor action, such as scrolling to the bottom of the page'),
        'url' => 'admin/structure/acquia_lift/goal/add/page',
        'modal' => TRUE,
        'ctools_style' => 'ctools-modal-acquia-lift-style',
        'logo' => theme('image', array(
          'path' => $path . '/images/goal-type-page.png',
          'alt' => t('Page goal'),
          'title' => t('Select this option to create a page goal.'),
        ))
      ),
    ),
  ));
  return ctools_modal_render(t('Add a goal'), $output);
}

/**
 * Page callback to generate the ctools modal form to create a goal of a
 * specific type.
 *
 * @param $type
 *   The type of goal to create.  Current supported choices: 'existing', 'page'
 *   Note that element is handled via JavaScript.
 * @param $ajax
 *   Indicates if AJAX is supported.
 */
function acquia_lift_create_goal_type_callback($type, $ajax) {
  acquia_lift_create_ctools_form(
    t('Add a goal'),
    'acquia_lift_create_goal_type_form',
    array('type' => $type),
    'acquia_lift_create_goal_type_completed_callback'
  );
}

/**
 * Ctools form processing complete handler for the creation of a new goal by
 * type.
 * @see acquia_lift_create_ctools_form().
 */
function acquia_lift_create_goal_type_completed_callback($form, &$form_state) {
  $agent = $form_state['values']['agent'];
  $settings = personalize_agent_get_settings();
  if ($settings[$agent->machine_name]['verified'] && personalize_agent_get_status($agent->machine_name) === PERSONALIZE_STATUS_NOT_STARTED) {
    $message = t('Goal added.  You can now start the campaign.');
  }
  else {
    $message = t('%goal goal added to campaign.', array('%goal' => $form_state['acquia_lift']['goal_name']));
  }
  $commands = array(
    ctools_modal_command_dismiss(),
    ajax_command_settings(array('personalize' => array(
      'campaigns' => $settings,
    )), TRUE),
    acquia_lift_command_messagebox($message, 10),
    acquia_lift_command_process_queue(),
  );
  return $commands;
}

/**
 * Form builder function to create a new goal in the modal process of a
 * specific type.
 *
 * @param type
 *   The type of goal to create.  Current supported choices: 'page', 'existing'
 */
function acquia_lift_create_goal_type_form($form, &$form_state, $type) {
  ctools_include('modal');
  ctools_include('ajax');
  ctools_add_js('ajax-responder');

  $agent_name = personalize_get_campaign_context();
  $agent = personalize_agent_load($agent_name);
  if (empty($agent)) {
    $form['message'] = array(
      '#markup' => t('You must select a campaign before adding goals.'),
    );
    return $form;
  }

  $form['agent'] = array(
    '#type' => 'value',
    '#value' => $agent,
  );
  $form['goal_type'] = array(
    '#type' => 'value',
    '#value' => $type,
  );
  $change_link = ctools_modal_text_button(t('Change type of goal'), 'admin/structure/acquia_lift/goal/add/nojs', t('Change type of test'), 'ctools-modal-acquia-lift-style');
  // Add individual forms specific to the type of goal being created.
  switch ($type) {
    case 'existing':
      $form['goal_type_change'] = array(
        '#markup' => theme('acquia_lift_create_type_change', array(
          'type' => t('Pre-existing goal'),
          'id' => 'acquia-lift-create-goal-existing',
          'change_link' => $change_link,
        )),
      );
      $form['goal'] = _acquia_lift_create_existing_goal_form($agent);
      break;
    case 'page':
      $form['goal_type_change'] = array(
        '#markup' => theme('acquia_lift_create_type_change', array(
          'type' => t('Page goal'),
          'id' => 'acquia-lift-create-goal-page',
          'change_link' => $change_link,
        )),
      );
      $form['goal'] = _acquia_lift_create_page_goal_form($agent);
      break;
  }
  // Common actions.
  $form['actions']['submit_form'] = array(
    '#type' => 'submit',
    '#attributes' => array(
      'class' => array('action-item-primary-active', 'acquia-lift-submit-button'),
    ),
    '#value' => t('Add goal'),
  );
  $form['actions']['reset'] = array(
    '#markup' => ctools_ajax_text_button(t('Cancel'), 'admin/structure/acquia_lift/cancel/nojs', t('Cancel'), 'acquia-lift-cancel-button'),
  );
  return $form;
}

/**
 * Loads the existing goal selection form for a campaign.
 *
 * @param $agent
 *   The agent data for the campaign that this goal will belong to.
 */
function _acquia_lift_create_existing_goal_form($agent) {
  // Exclude any actions already used in this campaign.
  $exclude = array();
  $goals = personalize_goal_load_by_conditions(array('agent' => $agent->machine_name));
  foreach ($goals as $goal) {
    $exclude[$goal->action] = $goal->action;
  }
  module_load_include('inc', 'personalize', 'personalize.admin');
  $all_goal_options = personalize_goals_options();
  $goal_options = array_diff_key($all_goal_options, $exclude);
  $form['action_name'] = array(
    '#type' => 'select',
    '#title' => t('Goal'),
    '#options' => $goal_options,
    '#empty_option' => t('-Select-'),
    '#required' => TRUE,
  );
  return $form;
}

/**
 * Loads the page goals selection form for a campaign.
 *
 * @param $agent
 *   The agent data for the campaign that this goal will belong to.
 */
function _acquia_lift_create_page_goal_form($agent) {
  $form['actionable_element'] = array(
    '#type' => 'value',
    '#value' => 'page',
  );
  $form['title'] = array(
    '#title' => t('Title'),
    '#description' => t('Name'),
    '#type' => 'textfield',
    '#required' => TRUE,
  );
  // To be filled in on the client-side.
  $form['pages'] = array(
    '#type' => 'hidden',
  );
  // Needed by the visitor actions form for structure.
  $form['event'] = array(
    '#tree' => TRUE
  );
  $form['options'] = array(
    '#tree' => TRUE,
  );

  $form['#validate']['_acquia_lift_create_page_goal_form_validate'];
  module_load_include('inc', 'visitor_actions', 'visitor_actions.admin');
  visitor_actions_get_form_options('page', $form, FALSE);
  return $form;
}

/**
 * Form validation for creating a new page goal simplified form.
 */
function _acquia_lift_create_page_goal_form_validate($form, &$form_state) {
  // Allow the plugin to validate the form values.
  ctools_include('plugins');
  if ($class = ctools_plugin_load_class('visitor_actions', 'actionable_element', $form_state['values']['actionable_element'], 'handler')) {
    call_user_func_array(array($class, 'validate'), array($form_state['values']));
  }
}

/**
 * Submit handler to create a new goal for a campaign.
 */
function acquia_lift_create_goal_type_form_submit($form, &$form_state) {
  $action_name = $goal_name = '';
  $agent_data = $form_state['values']['agent'];

  switch ($form_state['values']['goal_type']) {
    case 'existing':
      $action_name = $form_state['values']['action_name'];
      $goal_name = $form['goal']['action_name']['#options'][$action_name];
      break;
    case 'page':
      $plugin = $form_state['values']['actionable_element'];
      list($type, $event) = explode('::', $form_state['values']['event'][$plugin]);
      $goal_name = $form_state['values']['title'];
      $action_name = personalize_generate_machine_name($goal_name, visitor_actions_action_name_exists, '_');
      $action = array(
        'label' => $goal_name,
        'machine_name' => $action_name,
        'plugin' => $plugin,
        'client_side' => $type == 'client' ? 1 : 0,
        'identifier' => '',
        'event' => $event,
        'pages' => $form_state['values']['pages'],
        // The data array gets populated with the plugin-specific
        // info for the action.
        'data' => isset($form_state['values']['options'][$plugin]) ? $form_state['values']['options'][$plugin] : array(),
      );
      // Allow the plugin to modify the action before saving.
      ctools_include('plugins');
      if ($class = ctools_plugin_load_class('visitor_actions', 'actionable_element', $action['plugin'], 'handler')) {
        $action = call_user_func_array(array($class, 'actionPresave'), array($action));
      }
      visitor_actions_save_action($action);
      break;
  }
  try {
    personalize_goal_save($agent_data->machine_name, $action_name, 1);
  } catch (Exception $e) {
    drupal_set_message($e->getMessage(), 'error');
  }
  $form_state['acquia_lift']['goal_name'] = $goal_name;
}

/**
 * Page callback to generate a start campaign screen.
 */
function acquia_lift_start_campaign_callback($agent_name) {
  $agent_data = personalize_agent_load($agent_name);
  acquia_lift_create_ctools_form(
    t('Start campaign %campaign', array('%campaign' => $agent_data->label)),
    'acquia_lift_start_campaign_form',
    array('agent_data' => $agent_data),
    'acquia_lift_start_campaign_complete_callback'
  );
}

/**
 * Form handler to start a new campaign.
 *
 * @param stdClass $agent_data
 *   The data for the agent to start.
 */
function acquia_lift_start_campaign_form($form, &$form_state, $agent_data) {
  ctools_include('modal');
  ctools_include('ajax');
  ctools_add_js('ajax-responder');

  $form['agent'] = array(
    '#type' => 'value',
    '#value' => $agent_data,
  );
  $form['decision_style'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Automatically determine a winner'),
    '#options' => array(
      'adaptive' => t('Acquia Lift will adapt to visitors to choose the best option over time.'),
    ),
    '#default_value' => array($agent_data->data['decision_style']),
  );
  module_load_include('inc', 'personalize', 'personalize.admin');
  $form['campaign_end'] = personalize_agent_end_date_form($agent_data->machine_name);

  $form['actions']['submit_form'] = array(
    '#type' => 'submit',
    '#attributes' => array(
      'class' => array('action-item-primary-active', 'acquia-lift-submit-button'),
    ),
    '#value' => t('Start campaign'),
  );
  $form['actions']['reset'] = array(
    '#markup' => ctools_ajax_text_button(t('Cancel'), 'admin/structure/acquia_lift/cancel/nojs', t('Cancel'), 'acquia-lift-cancel-button'),
  );  return $form;
}

/**
 * Form validator for acquia_lift_start_campaign().
 */
function acquia_lift_start_campaign_form_validate($form, &$form_state) {
  module_load_include('inc', 'personalize', 'personalize.admin');
  personalize_agent_end_date_form_validate($form_state['values']['campaign_end']);
}

/**
 * Form submit handler for acquia_lift_start_campaign().
 */
function acquia_lift_start_campaign_form_submit($form, &$form_state) {
  // Set the decision style.
  $agent_data = $form_state['values']['agent'];
  $agent_data->data['decision_style'] = empty($form_state['values']['decision_style']) ? 'random' : current($form_state['values']['decision_style']);
  personalize_agent_save($agent_data);

  // Set the end date.
  module_load_include('inc', 'personalize', 'personalize.admin');
  personalize_agent_end_date_form_submit($agent_data->machine_name, $form_state['values']['campaign_end']);

  // Start the campaign.
  $form_state['acquia_lift']['started'] = personalize_agent_set_status($agent_data->machine_name, PERSONALIZE_STATUS_RUNNING);
}

/**
 * Form complete callback for the acquia_lift_start_campaign form.
 * @see acquia_lift_create_ctools_form().
 */
function acquia_lift_start_campaign_complete_callback($form, $form_state) {
  $agent_data = $form_state['values']['agent'];
  if ($form_state['acquia_lift']['started']) {
    $message = t('The %campaign campaign has been started', array(
      '%campaign' => $agent_data->label,
    ));
  }
  else {
    $message = t('There was a problem starting your campaign.');
  }
  return array(
    ctools_modal_command_dismiss(),
    ajax_command_settings(array('personalize' => array(
      'campaigns' => personalize_agent_get_settings(),
    )), TRUE),
    acquia_lift_command_messagebox($message, 10),
  );
}

/**
 * Handles generating a CTools form window for a particular form.

 * @param string $title
 *   The title for the modal window.
 * @param $form_name
 *   The name of the form to retrieve.
 * @param null $args
 *   An array of form arguments to pass to the form.
 * @param null $callback
 *   A callback to call when the form has been executed.
 *   The callback will receive the $form and $form_state arguments and should
 *   return the array of AJAX commands to execute.
 *   If empty then the modal will simply be dismissed.
 */
function acquia_lift_create_ctools_form($title, $form_name, $args = array(), $callback = NULL) {
  ctools_include('modal');
  ctools_include('ajax');
  ctools_modal_add_js();
  ctools_js_load('ajax-responder');

  $form_state = array(
    'ajax' => TRUE,
    'title' => $title,
    'build_info' => array(
      'args' => $args,
    )
  );

  $output = ctools_modal_form_wrapper($form_name, $form_state);

  if ($form_state['executed']) {
    // Form has been processed to generate the output commands.
    if (is_callable($callback)) {
      $output = $callback($output, $form_state);
    } else {
      $output = array(
        ctools_modal_command_dismiss(),
      );
    }
  }
  personalize_ajax_commands_deliver($output);
}

/**
 * AJAX callback to cancel the campaign creation flow.
 *
 * @param $ajax
 *   Indicates if AJAX is supported.
 */
function acquia_lift_campaign_flow_cancel($ajax = NULL) {
  if (!$ajax) {
    // We should never be here out of ajax context.
    return MENU_NOT_FOUND;
  }

  ctools_include('modal');
  ctools_include('ajax');

  $commands = array(ctools_modal_command_dismiss());
  print ajax_render($commands);
}

/**
 * =======================================================================
 *  P A G E  V A R I A T I O N  M A N A G E M E N T
 * =======================================================================
 */

/**
 * Creates an option set option for a page variation based on the passed in option info.
 *
 * @param $variation_set_name
 *   The variation set this belongs to.
 * @param $option_set
 *   The option set object to add the option to
 * @param $option_info
 *   The information about the individual option to be added for this variation.
 * @param $variation_number
 *   The variation number to add this option to. If unspecified it will be created as a
 *   new variation.
 * @return number
 *   The variation number that was created or added to.
 */
function acquia_lift_page_variation_create($variation_set_name, $option_set, $option_info, $variation_number = NULL) {
  // Find all other option sets in this variation set (i.e. with the same
  // decision name and agent.)
  $option_sets = personalize_option_set_load_multiple(array(), array('decision_name' => $variation_set_name, 'agent' => $option_set->agent));

  // To figure out the variation number, check the number of options in the first
  // existing option set and the number of options in the passed in option set and
  // use whichever is higher.
  if (!empty($option_sets)) {
    $first_os = reset($option_sets);
    $max_first = max(array_keys($first_os->options));
  }
  else {
    $max_first = 0;
  }
  // The maximum new variation number must take into account the number of
  // options for both the passed in option set and the original first option set
  // as well as the indexes on those options which may no longer be
  // sequential.
  $max_variation_number  = max(array_merge(array(0, $max_first), array_keys($option_set->options)));
  // The above would give us the maximum existing number so increment for new.
  $max_variation_number++;
  if ($variation_number === NULL || $variation_number > $max_variation_number) {
    $variation_number = $max_variation_number;
  }

  $option_set->decision_name = $variation_set_name;
  $has_control_option = FALSE;
  $new_option_id = 'variation-' . $variation_number;
  $new_option_label = t('Variation #@num', array('@num' => $variation_number));
  $i = 0;
  foreach ($option_set->options as $i => $option) {
    // Check for the control variation.
    if ($option['option_id'] == PERSONALIZE_CONTROL_OPTION_ID) {
      if ($i !== 0) {
        // The control option has to be index 0.
        unset($option_set->options[$i]);
      }
      else {
        $has_control_option = TRUE;
      }
    }
  }
  $next_option = $i + 1;
  // Add any missing options before adding the one for this variation.
  for ($j = $next_option; $j < $variation_number; $j++) {
    $option_label = empty($first_os) || !isset($first_os->options[$j]) ? t('Variation #@num', array('@num' => $j)) : $first_os->options[$j]['option_label'];
    $option_set->options[$j] = personalize_elements_get_option_for_variation('variation-' . $j, $option_label);
  }
  // Now add the option for this variation.
  $option_set->options[$variation_number] = array('option_id' => $new_option_id, 'option_label' => $new_option_label) + $option_info;
  $control_option = personalize_elements_get_option_for_variation(PERSONALIZE_CONTROL_OPTION_ID, PERSONALIZE_CONTROL_OPTION_LABEL);
  if (!$has_control_option) {
    array_unshift($option_set->options, $control_option);
  }
  // If the variation set contains variations with higher numbers than this, make
  // sure options exist in this option set for those variation numbers.
  for ($k = $variation_number + 1; $k < $max_variation_number; $k++) {
    if (!isset($option_set->options[$k])) {
      $option_label = empty($first_os) || !isset($first_os->options[$k]) ? t('Variation #@num', array('@num' => $k)) : $first_os->options[$k]['option_label'];
      $option_set->options[$k] = personalize_elements_get_option_for_variation('variation-' . $k, $option_label);
    }
  }

  $option_set = personalize_option_set_save($option_set);
  // Go through the other option sets in the variation set to update their options.
  foreach ($option_sets as $osid => $os) {
    if (isset($option_set->osid) && $osid == $option_set->osid) {
      continue;
    }
    // Make sure we have a control option.
    if (!isset($os->options[0])) {
      $os->options[0] = array();
    }
    $os->options[0] = array_merge($os->options[0], $control_option);
    // Add empty options for any missing indexes up to and including the variation number.
    for ($i = 1; $i <= $variation_number; $i++) {
      if (!isset($os->options[$i])) {
        $op_id = 'variation-' . $i;
        $op_label = empty($first_os) || !isset($first_os->options[$i]) ? t('Variation #@num', array('@num' => $i)) : $first_os->options[$i]['option_label'];
        $os->options[$i] = personalize_elements_get_option_for_variation($op_id, $op_label);
      }
    }
    personalize_option_set_save($os);
  }
  return $variation_number;
}

/**
 * Rename a page variation.
 *
 * To rename a page variation, re-label all options that match the variation.
 *
 * @param string $variation_set_name
 *   The name of the variation set that this variation belongs to.
 * @param string $agent_name
 *   The name of the campaign/agent for this page variation.
 * @param number $variation_number
 *   The number of variation within the variation set to rename
 * @param string $variation_name
 *   The new variation name.
 */
function acquia_lift_page_variation_rename($variation_set_name, $agent_name, $variation_number, $variation_name) {
  // Cannot rename the control variation.
  if ($variation_number == 0) {
    return;
  }
  $option_sets = personalize_option_set_load_multiple(array(), array('decision_name' => $variation_set_name, 'agent' => $agent_name));
  foreach ($option_sets as $option_set) {
    if (isset($option_set->options[$variation_number])) {
      $option_set->options[$variation_number]['option_label'] = $variation_name;
      personalize_option_set_save($option_set);
    }
  }
}

/**
 * Gets the name of a specific page variation set variation.
 *
 * @param string $variation_set_name
 *   The name of the variation set that this variation belongs to.
 * @param string $agent_name
 *   The name of the campaign/agent for this page variation.
 * @param number $variation_number
 *   The number of variation within the variation set to retrieve.
 */
function acquia_lift_page_variation_get_name($variation_set_name, $agent_name, $variation_number) {
  $option_sets = personalize_option_set_load_multiple(array(), array('decision_name' => $variation_set_name, 'agent' => $agent_name));
  $option_set = current($option_sets);
  if ($option_set) {
    if (isset($option_set->options[$variation_number])) {
      return check_plain($option_set->options[$variation_number]['option_label']);
    }
  }
  return '';
}

/**
 * Gets the page for the variations within a variation set.
 *
 * Assumption: All page variations are on the same path for a single variation set.
 */
function acquia_lift_page_variation_get_path($variation_set_name, $agent_name) {
  $option_sets = personalize_option_set_load_multiple(array(), array('decision_name' => $variation_set_name, 'agent' => $agent_name));
  $option_set = current($option_sets);
  if ($option_set && isset($option_set->data)) {
    return isset($option_set->data['pages']) ? $option_set->data['pages'] : '';
  }
  return '';
}

/**
 * Delete a page variation.
 *
 * @param string $variation_set_name
 *   The name of the variation set that contains the variation to be removed.
 * @param string $agent_name
 *   The name of the campaign/agent for the page variation.
 * @param number $variation_number
 *   The number for the variation to delete.
 */
function acquia_lift_page_variation_delete($variation_set_name, $agent_name, $variation_number) {
  $option_sets = personalize_option_set_load_multiple(array(), array('decision_name' => $variation_set_name, 'agent' => $agent_name));
  foreach ($option_sets as $option_set) {
    if (isset($option_set->options[$variation_number])) {
      unset($option_set->options[$variation_number]);
      personalize_option_set_save($option_set);
    }
  }
  // If the only variation left is the control variation, then delete
  // all of the option sets for this variation.
  if (count($option_set->options) === 1) {
    $last_option = current($option_set->options);
    if ($last_option['option_id'] === PERSONALIZE_CONTROL_OPTION_ID) {
      reset($option_sets);
      foreach ($option_sets as $option_set) {
        personalize_option_set_delete($option_set->osid);
      }
    }
  }
}
