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
  $form['#submit'] = array(
    'acquia_lift_admin_form_submit',
  );
  return system_settings_form($form);
}

/**
 * Simple form for initiating batch syncing of agents to Lift.
 */
function acquia_lift_batch_sync_form($form, &$form_state) {
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Sync all campaigns to Lift')
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

  // Clear the ctools plugin "agent_type" cache for personalize.
  cache_clear_all('plugins:personalize:agent_type', 'cache');
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

  $form['report_filters']['date_filters'] = acquia_lift_report_date_filter(isset($form_state['values']['date_filters']) ? $form_state['values']['date_filters'] : 'all');

  // Default to showing the complete history of the campaign.
  $date_start_report = date('Y-m-d', $agent_data->started);
  $date_end_report = $date_end_targeting = date('Y-m-d');
  if (isset($form_state['values']['date_filters']) && $form_state['values']['date_filters'] === 'today') {
    $date_start_report = date('Y-m-d');
    $date_end_report = NULL;
  }

  $reports = $agent->buildCampaignReports($decision_point, $date_start_report, $date_end_report);

  // Generate mark-up for adaptive style report labels.
  $report_title_additional = '';
  if ($data['decision_style'] === 'adaptive') {
    $report_title_additional = theme('acquia_lift_percentage_label', array(
      'percent_label' => t('Random'),
      'rest_label' => 'Personalized',
      'percent' => $data['explore_rate']
    ));
  }

  // Overview report section.
  $form['overview_report'] = array(
    'overview_report_title' => array(
      '#markup' => '<h2>Overview</h2>',
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

  // Experiment details section.
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
          '#markup' => '<h2>Experiment</h2>',
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
      '#markup' => 'random group',
    );
    $form['experiment_report']['header']['summary']['distribution'] = array(
      '#markup' => $report_title_additional,
      '#theme_wrappers' => array('container'),
    );
  }
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
  if (!isset($reports['targeting'])) {
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
      '#markup' => 'random and personalized groups',
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
          '#markup' => '<h2>Context Stability</h2>',
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
      '#markup' => 'random and personalized groups',
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
    '#title' => '&nbsp;',
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
  $output = theme('acquia_lift_campaign_type_list', array(
    'items' => array(
      'ab' => array(
        'title' => t('A/B test'),
        'description' => t('Create variations of the content on your site to compare against each other.'),
        'url' => 'admin/structure/acquia_lift/add/ab',
        'logo' => theme('image', array(
          'path' => $path . '/images/campaign-type-ab.png',
          'alt' => t('A/B test'),
          'title' => t('Select this option to create an A/B test.'),
        )),
      ),
      'custom' => array(
        'title' => t('Custom Lift campaign'),
        'description' => t('Custom campaigns allow you to create a content targeting or multi-variate campaign using the legacy campaign creation system.'),
        'url' => 'admin/structure/personalize/add/',
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
  ctools_include('modal');
  ctools_include('ajax');
  ctools_modal_add_js();

  $form_state = array(
    'ajax' => TRUE,
    'title' => t('Create a campaign'),
  );

  // ctools_modal_form_wrapper doesn't provide any way to pass parameters to the form builder function.
  $form_state['build_info']['args']['type'] = $type;

  $output = ctools_modal_form_wrapper('acquia_lift_create_campaign_type_form', $form_state);
  // Form has been processed so close it.
  if ($form_state['executed']) {
    $output = array(
      ctools_modal_command_dismiss(),
      ajax_command_settings(array('personalize' => array(
        'activeCampaign' => personalize_get_campaign_context(),
        'campaigns' => personalize_agent_get_settings(),
      )), TRUE),
      acquia_lift_command_messagebox(t('Click on an element in your site to create %variation in %campaign.', array(
        '%campaign' => $form_state['values']['agent_basic_info']['title'],
        '%variation' => 'Variation #1',
      )), 10),
      acquia_lift_command_page_variation_toggle(),
    );
  }
  $page = array('#type' => 'ajax', '#commands' => $output);
  ajax_deliver($page);
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
        '#markup' => theme('acquia_lift_campaign_type_change', array(
          'type' => t('A/B test'),
          'change_link' => $change_link,
        )),
      );
      $form['agent'] = _acquia_lift_create_simplified_campaign_form('acquia_lift_simple_ab');
      break;
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
  $agent = personalize_agent_save($agent_data);
  personalize_set_campaign_context($agent->machine_name);
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
 * Form handler for a page variation details form.
 *
 * @param string $variation_type
 *   The variation type that defines the type of details to include.  This
 *   is a key defined as a personalize_elements variaton.
 * @see hook_personalize_elements_variation_types
 */
function acquia_lift_page_variation_details($form, &$form_state, $variation_type) {
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
    '#type' => 'hidden',
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
  $form['actions']['submit_form'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
    '#attributes' => array(
      'class' => array('action-item-primary-active', 'acquia-lift-submit-button'),
    ),
    '#ajax' => array(
      'callback' => 'acquia_lift_page_variation_details_ajax_callback',
      'wrapper' => 'acquia-lift-page-variation-details',
    ),
  );
  // Add a cancel button for AJAX-loaded forms.
  $form['actions']['cancel_form'] = array(
    '#type' => 'button',
    '#button_type' => 'cancel',
    '#value' => t('Cancel'),
    '#attributes' => array(
      'class' => array('acquia-lift-cancel-button'),
    ),
    '#ajax' => array(
      'callback' => 'acquia_lift_page_variation_details_ajax_callback',
      'wrapper' => 'acquia-lift-page-variation-details',
    ),
    // Turn off validation.
    '#limit_validation_errors' => array(),
    '#submit' => array(),
  );

  return $form;
}

/**
 * Form submit handler for acquia_lift_page_variation_details.
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
function acquia_lift_page_variation_details_submit($form, &$form_state) {
  $agent_name = $variation_set_name = $form_state['values']['agent'];
  $option_set = personalize_elements_get_option_set_for_variation($variation_set_name, $agent_name, $form_state['values']['selector'], $form_state['values']['variation_type'], $form_state['values']['pages']);

  $variation_number = NULL;
  if (is_numeric($form_state['values']['variation_number']) && $form_state['values']['variation_number'] >= 0) {
    $variation_number = $form_state['values']['variation_number'];
  }
  $variation_number = personalize_create_page_variation($variation_set_name, $option_set, array('personalize_elements_content' => $form_state['values']['personalize_elements_content']), $variation_number);
  // Save this for the Ajax callback.
  $form_state['storage']['variation_number'] = $variation_number;
}

/**
 * Responds to AJAX submission of a variation type details page.
 */
function acquia_lift_page_variation_details_ajax_callback($form, &$form_state) {
  $op = isset($form_state['values']['op']) ? $form_state['values']['op'] : '';
  $commands = array();
  if ($op == t('Save')) {
    $errors = form_get_errors();
    // If validation errors exist, return the form.
    if (!empty($errors)) {
      return $form;
    }
    // Otherwise the form submitted so stop creating a page variation
    // and show a confirmation message.
    $commands[] = acquia_lift_command_page_variation_toggle(FALSE);
    $commands[] = acquia_lift_command_messagebox(t('The variation has been created.  Track your conversions by adding goals.'), 10);
    // Get the updated option sets for the page variation.
    $variation_set_name = $form_state['values']['agent'];
    $option_sets = personalize_option_set_load_multiple(array(), array('decision_name' => $variation_set_name));
    $option_set_settings = array_map('_personalize_convert_option_set_to_js_setting', $option_sets);
    foreach($option_set_settings as $option_set_setting) {
      $settings['personalize']['option_sets'][key($option_set_setting)] = reset($option_set_setting);
    }
    // Also get the updated personalize elements options.
    $settings['personalize_elements']['elements'] = personalize_elements_element_settings($option_sets, $form_state['values']['pages']);
    // Note: JavaScript assets for personalize elements are included in
    // personalize_elements_page_build when the module is enabled and therefore
    // do not need to be explicitly added here.
    $commands[] = ajax_command_settings($settings, TRUE);
    // Determine the index to preview.  If creating a new variation, then the
    // first index is 1 because 0 is the control variation.
    $preview_variation_index = (!is_numeric($form_state['storage']['variation_number']) || $form_state['storage']['variation_number'] < 1) ? 1 : $form_state['storage']['variation_number'];
    $commands[] = acquia_lift_command_page_variation_preview($form_state['values']['agent'], $preview_variation_index);
  }
  else if ($op === t('Cancel')) {
    // Turn off edit mode and redirect them.
    $commands[] = acquia_lift_command_page_variation_toggle(FALSE);
  }

  return array('#type' => 'ajax', '#commands' => $commands);
}
