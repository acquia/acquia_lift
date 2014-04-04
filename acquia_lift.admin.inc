<?php

/**
 * @file acquia_lift.admin.inc
 * Provides functions needed for the admin UI.
 */

/**
 * Admin form for configuring personalization backends.
 */
function acquia_lift_admin_form($form, &$form_state) {
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
    '#markup' => t("<p>This information is used to link your Acquia Lift account to Drupal.</p><p>Contact Acquia to get set up with Acquia Lift credentials.</p>"),
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

  $form['#submit'] = array(
    'acquia_lift_admin_form_submit',
  );
  return system_settings_form($form);
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
}

/**
 * Submit handler for the Acquia Lift admin form.
 *
 * Creates a default Acquia Lift agent if one does not yet exist.
 */
function acquia_lift_admin_form_submit($form, &$form_state) {
  acquia_lift_ensure_default_agent($form_state['values']['acquia_lift_account_info']);
}

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
  $data = $agent->getData();

  $form = array(
    '#prefix' => '<div id="acquia-lift-reports">',
    '#suffix' => '</div>',
    '#attached' => array(
      'css' => array(
        drupal_get_path('module', 'acquia_lift') . '/css/acquia_lift.admin.css',
      )
    ),
  );

  $form['filters'] = array(
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
    $decision_point = !empty($option_set) ? $option_set->decision_name : key($decisions);
  }
  $form['filters']['decision_point'] = acquia_lift_report_decision_point_dropdown($decision_options, $decision_point);

  $form['filters']['date_filters'] = acquia_lift_report_date_filter(isset($form_state['values']['date_filters']) ? $form_state['values']['date_filters'] : 'all');

  // Default to showing the complete history of the campaign.
  $date_start_report = date('Y-m-d', $agent_data->started);
  $date_end_report = $date_end_targeting = date('Y-m-d');
  if (isset($form_state['date_filters']) && $form_state['date_filters'] === 'today') {
    $date_start_report = date('Y-m-d');
    $date_end_report = NULL;
  }

  // Overview report section.
  $form['overview_report'] = array(
    'overview_report_title' => array(
      '#markup' => '<h2>Overview</h2>',
    ),
    '#theme_wrappers' => array('container'),
    '#id' => 'acquia-lift-overview-report',
    '#attributes' => array(
      'class' => array('clearfix'),
    ),
  );

  // Confidence report section.
  $form['confidence_report'] = array(
    '#tree' => TRUE,
    'confidence_report_title' => array(
      '#markup' => '<h2>Lift Report</h2>',
    ),
    '#theme_wrappers' => array('container'),
    '#id' => 'acquia-lift-confidence-report',
  );
  // Targeting report section.
  $form['targeting_report'] = array(
    '#tree' => TRUE,
    'targeting_report_title' => array(
      '#markup' => '<h2>Context relevance</h2>',
    ),
    '#theme_wrappers' => array('container'),
    '#id' => 'acquia-lift-targeting-report',
  );

  $overview_report = $agent->buildOverviewReport($date_start_report, $date_end_report, (isset($form_state['values']['date_filter']) && $form_state['values']['date_filter'] === 'today'));
  $form['overview_report']['report'] = array(
    '#markup' => drupal_render($overview_report),
    '#theme_wrappers' => array('container'),
    '#id' => 'acquia-lift-overview-report-data',
  );

  // Features dropdown for confidence report.
  $context_values = array();
  try {
    // Get the possible feature values to filter by.
    $result = AcquiaLiftAPI::getInstance(variable_get('acquia_lift_account_info', array()))->getPotentialTargetingValues($agent_data->machine_name);
    if (isset($result['data']['potential']['features']) && !empty($result['data']['potential']['features'])) {
      foreach ($result['data']['potential']['features'] as $feature) {
        $context_values[$feature['code']] = $feature['name'];
      }
    }
  }
  catch (AcquiaLiftException $e) {
    drupal_set_message($e->getMessage(), 'error');
  }
  // Context value filter.
  $selected_value = isset($form_state['values']['confidence_report']['feature_links']) ? $form_state['values']['confidence_report']['feature_links'] : key($context_values);
  $form['confidence_report']['feature_links'] = array(
    '#title' => t('Filter by visitor context: '),
    '#type' => 'select',
    '#options' => $context_values,
    '#default_value' => $selected_value,
    '#ajax' => array(
      'callback' => 'acquia_lift_report_ajax_callback',
      'wrapper' => 'acquia-lift-confidence-report',
    ),
    '#return_element' => 'confidence_report',
  );
  // Get the confidence report.
  $confidence_report = $agent->buildConfidenceReport($date_start_report, $date_end_report, $decision_point, $selected_value);
  $form['confidence_report']['report'] = array(
    '#markup' => drupal_render($confidence_report),
    '#theme_wrappers' => array('container'),
    '#id' => 'acquia-lift-confidence-report-data',
  );

  // Decision point dropdown for targeting report.
  if (isset($form_state['values']['targeting_report']['decision_point'])) {
    $decision_point_targeting_report = $form_state['values']['targeting_report']['decision_point'];
  }
  else {
    $decision_point_targeting_report = !empty($option_set) ? $option_set->decision_name : key($decisions);
  }
  $form['targeting_report']['decision_point'] = acquia_lift_report_decision_point_dropdown('targeting', $decision_options, $decision_point_targeting_report);

  // Date buttons for targeting report.
  $form['targeting_report']['date_filters'] = array(
    '#prefix' => '<div class="acquia-lift-report-date-filters">',
    '#suffix' => '</div>',
  );
  $today_only_targeting = FALSE;
  if (isset($form_state['targeting_report_start_date'])) {
    if ($form_state['targeting_report_start_date'] == 'today') {
      $today_only_targeting = TRUE;
      $date_start_targeting = date('Y-m-d');
      $date_end_targeting = NULL;
    }
  }
  $form['targeting_report']['date_all'] = acquia_lift_report_date_filter_button(t('Complete history'), 'all', 'targeting', !$today_only_targeting);
  $form['targeting_report']['date_today'] = acquia_lift_report_date_filter_button(t('Today only'), 'today', 'targeting', $today_only_targeting);

  // Get the targeting report.
  $targeting_report = $agent->buildTargetingFeaturesReport($date_start_report, $date_end_report, $decision_point);
  $form['targeting_report']['report'] = array(
    '#markup' => drupal_render($targeting_report),
    '#theme_wrappers' => array('container'),
    '#id' => 'acquia-lift-targeting-report-data',
  );
  // We have to specify the include file so as not to lose it during rendering from ajax.
  // @see drupal_retrieve_form():734
  $form_state['build_info']['files'] = array(
    drupal_get_path('module', 'acquia_lift') . '/acquia_lift.admin.inc',
  );
  return $form;
}

/**
 * Returns a date filter for the reports.
 *
 * @param $selected
 *   The selected date option.
 * @return array
 *   An array representing the form input.
 */
function acquia_lift_report_date_filter($selected = 'all') {
  return array(
    '#type' => 'select',
    '#options' => array(
      'today' => t('Today only'),
      'all' => t('All dates'),
    ),
    '#default_value' => $selected,
    '#ajax' => array(
      'callback' => "acquia_lift_report_ajax_callback",
      'wrapper' => "acquia-lift-reports",
    ),
    '#title_display' => 'invisible',
  );
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
 * Ajax callback for filtering options.
 */
function acquia_lift_report_ajax_callback($form, &$form_state) {
  return $form;
}
