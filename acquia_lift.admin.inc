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
  $form['decision_point'] = acquia_lift_report_decision_point_dropdown('confidence', $decision_options, $decision_point);

  // Date buttons filters.
  // Default to showing the complete history of the campaign.
  $date_start_report = date('Y-m-d', $agent_data->started);
  $date_end_report = $date_end_targeting = date('Y-m-d');

  $form['date_filters'] = array(
    '#prefix' => '<div class="acquia-lift-report-date-filters">',
    '#suffix' => '</div>',
  );
  $today_only = FALSE;
  if (isset($form_state['report_start_date'])) {
    if ($form_state['report_start_date'] == 'today') {
      $today_only = TRUE;
      $date_start_report = date('Y-m-d');
      $date_end_report = NULL;
    }
  }
  $form['date_all'] = acquia_lift_report_date_filter_button(t('Complete history'), 'all', !$today_only);
  $form['date_today'] = acquia_lift_report_date_filter_button(t('Today only'), 'today', $today_only);


  // Confidence report section.
  $form['confidence_report'] = array(
    '#prefix' => '<div id="acquia-lift-confidence-report-replace">',
    '#suffix' => '</div>',
    '#tree' => TRUE,
    'confidence_report_title' => array(
      '#markup' => '<h2>Lift Report</h2>',
    ),
  );
  // Targeting report section.
  $form['targeting_report'] = array(
    '#prefix' => '<div id="acquia-lift-targeting-report-replace">',
    '#suffix' => '</div>',
    '#tree' => TRUE,
    'targeting_report_title' => array(
      '#markup' => '<h2>Context relevance</h2>',
    ),
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
      'callback' => 'acquia_lift_confidence_report_ajax_callback',
      'wrapper' => 'acquia-lift-confidence-report-replace',
    ),
  );
  // Get the confidence report.
  $confidence_report = $agent->buildConfidenceReport($date_start_report, $date_end_report, $decision_point, $selected_value);
  $form['confidence_report']['report'] = array(
    '#markup' => drupal_render($confidence_report)
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
    '#markup' => drupal_render($targeting_report)
  );
  // We have to specify the include file so as not to lose it during rendering from ajax.
  // @see drupal_retrieve_form():734
  $form_state['build_info']['files'] = array(
    drupal_get_path('module', 'acquia_lift') . '/acquia_lift.admin.inc',
  );
  return $form;
}


/**
 * Menu callback for pulling reports via ajax (from filter links).
 *
 * @param $agent_name
 *   The name of the agent this report is for.
 * @param $report_name
 *   The name of the report.
 * @param $date
 *   The date to use for the report (either "today" or a start date for all dates
 *   from the start date up to today).
 * @param null $feature
 *   A feature to filter the report by.
 */
function acquia_lift_report_ajax($agent_name, $report_name, $date, $feature = NULL) {
  if ($date !== 'today') {
    $date_start = date('Y-m-d', $date);
    $date_end = date('Y-m-d');
  }
  else {
    $date_start = date('Y-m-d');
    $date_end = NULL;
  }
  $agent = personalize_agent_load_agent($agent_name);
  $confidence_report = $agent->buildConfidenceReport($date_start, $date_end, $feature);

  $selector = "#acquia-lift-$report_name-report .acquia-lift-tabbed-report";
  $commands = array(
    ajax_command_replace($selector, drupal_render($confidence_report)),
  );
  $page = array('#type' => 'ajax', '#commands' => $commands);
  ajax_deliver($page);
}

/**
 * Returns a button for changing the timeframe of the report.
 *
 * @param $title
 *   The text to display on the button
 * @param $type
 *   Either 'all' or 'today'
 * @param $disabled
 *   Whether the button should be disabled or not.
 * @return array
 *   An array representing the button element.
 */
function acquia_lift_report_date_filter_button($title, $type, $disabled) {
  return array(
    '#type' => 'submit',
    '#tag' => 'button',
    '#text' => $title,
    '#value' => $type,
    '#theme_wrappers' => array('personalize_html_tag'),
    '#attributes' => array(
      // The ID is necessary for the AJAX replace to function correctly.
      'id' => "edit-report-date-$type",
      'class' => array('acquia-lift-report-date-filter', 'form-submit',),
      'title' => $title,
    ),
    '#disabled' => $disabled,
    '#submit' => array('acquia_lift_report_date_filter_submit'),
    '#ajax' => array(
      'callback' => "acquia_lift_report_ajax_callback",
      'wrapper' => "acquia-lift-reports",
    )
  );
}

/**
 * Returns a dropdown for filtering by decision point.
 *
 * @param string $report_type
 *   The type of report this is for, e.g. 'confidence' or 'targeting'.
 * @param array $options
 *   The different decision point options.
 * @param $selected
 *   The decision point to set as the default value.
 * @return array
 *   An array representing a dropdown select list.
 */
function acquia_lift_report_decision_point_dropdown($report_type, $options, $selected) {
  return array(
    '#prefix' => '<div class="acquia-lift-report-decision-point-filter">',
    '#suffix' => '</div>',
    '#title' => t('Variation set: '),
    '#type' => 'select',
    '#options' => $options,
    '#default_value' => $selected,
    '#ajax' => array(
      'callback' => "acquia_lift_report_ajax_callback",
      'wrapper' => "acquia-lift-reports",
    )
  );
}

/**
 * Submit callback for the confidence report's date filter.
 */
function acquia_lift_report_date_filter_submit($form, &$form_state) {
  switch($form_state['values']['op']) {
    case 'today':
      $form_state['report_start_date'] = 'today';
      break;
    case 'all':
      $form_state['report_start_date'] = 'all';
      break;
  }
  $form_state['rebuild'] = TRUE;
}

/**
 * Ajax callback for the decision point dropdown in the targeting report.
 */
function acquia_lift_report_ajax_callback($form, &$form_state) {
  return $form;
}
