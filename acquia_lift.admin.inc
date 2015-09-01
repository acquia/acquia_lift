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

  $form['acquia_lift_profiles_js_path'] = array(
    '#type' => 'textfield',
    '#title' => t('Acquia Lift JavaScript path'),
    '#field_prefix' => 'http(s)://',
    '#default_value' => variable_get('acquia_lift_profiles_js_path', ''),
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
  $form['acquia_lift_report_max_days'] = array(
    '#type' => 'textfield',
    '#title' => t('Reporting history duration'),
    '#size' => 3,
    '#required' => TRUE,
    '#default_value' => variable_get('acquia_lift_report_max_days', 14),
    '#field_suffix' => t(' days'),
    '#description' => t('The maximum number of days for which to show reporting history.'),
    '#element_validate' => array('element_validate_integer_positive'),
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
  acquia_lift_batch_sync_all();
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
    form_set_error('acquia_lift_account_info][api_url', t('You must enter a valid API URL'));
  }
  if (!valid_url($form_state['values']['acquia_lift_profiles_js_path'])) {
    form_set_error('acquia_lift_profiles_js_path', t('You must enter a valid JavaScript path'));
  }
  // Strip any scheme from the URLs.
  $form_state['values']['acquia_lift_account_info']['api_url'] = preg_replace('/(^[a-z]+:\/\/)/i', '', $form_state['values']['acquia_lift_account_info']['api_url']);
  $form_state['values']['acquia_lift_profiles_js_path'] = preg_replace('/(^[a-z]+:\/\/)/i', '', $form_state['values']['acquia_lift_profiles_js_path']);

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
 * Page callback for full campaign listings.
 */
function acquia_lift_agent_list($agents = array(), $legacy = FALSE) {
  $agent_type_test = t('Test only');
  $agent_type_target = t('Target only');
  $agent_type_both = t('Test and target');
  $agent_type_legacy = t('Legacy');
  $build = array();
  $build['#attached']['library'][] = array('personalize', 'admin.campaign.list');
  $build['#attached']['css'][] = drupal_get_path('module', 'acquia_lift') . '/css/acquia_lift.admin.css';
  $build['#attached']['js'][] = drupal_get_path('module', 'acquia_lift') . '/js/acquia_lift.admin.js';

  $status_map = personalize_get_agent_status_map();
  if (empty($agents)) {
    $agents = personalize_agent_load_multiple(array(), array(), FALSE, TRUE, 'acquia_lift_sort_agent_started');
  }

  $sorted_agents = array();
  foreach ($agents as $agent) {
    // If this is a nested agent, it should not be listed.
    if (!$legacy && in_array($agent->plugin, array(ACQUIA_LIFT_TESTING_AGENT_V1, ACQUIA_LIFT_TESTING_AGENT_V2))) {
      continue;
    }
    $agent_status = personalize_agent_get_status($agent->machine_name);
    $sorted_agents[$agent_status][] = $agent;
  }

  if (empty($sorted_agents)) {
    $no_agents_intro = '<p>' . t('Acquia Lift Target uses personalizations to control how different sets of content variations are displayed to your website visitors.') . '</p>';
    $no_agents_intro .= '<p>' . t('When you create a new personalization, Acquia Lift Target will walk you through the process, helping you to add the set of variations that you want to display, create the goals that determine which variation is most effective, determine the visitor audience segments, and finally control when you want the personalization to run.') . '</p>';
    $no_agents_intro .= '<p>' . t('To create your first new personalization, click the above Add Personalization link.') . '</p>';

    return array(
      'no_agents' => array(
        '#markup' => $no_agents_intro,
      ),
    );
  }

  $build['personalizations_list'] = array(
    '#type' => 'container',
    '#attributes' => array(
      'id' => 'personalize-personalizations-list',
    ),
  );
  $navigation_bar = array();
  ksort($sorted_agents);
  foreach ($sorted_agents as $status => $agents) {
    // Notice the 'href', 'fragment', and 'external' here are made specific to theme an anchor link.
    $navigation_bar['personalization-group-' . $status] = array(
      'title' => theme('html_tag', array(
        'element' => array(
          '#tag' => 'h3',
          '#value' => $status_map[$status],
        ),
      )),
      'href' => '',
      'fragment' => 'personalization-group-' . $status,
      'external' => TRUE,
      'html' => TRUE,
    );
    $build['personalizations_list']['navigation_bar'] = array(
      '#markup' => theme('links', array('links' => $navigation_bar)),
    );

    $description = '';
    switch($status) {
      case PERSONALIZE_STATUS_NOT_STARTED:
        $description = t('These personalizations have not yet been manually started.');
        break;
      case PERSONALIZE_STATUS_SCHEDULED:
        $description = t('These personalizations are scheduled to start in the future.');
        break;
      case PERSONALIZE_STATUS_RUNNING:
        $description = t('These personalizations are currently displaying different content variations to visitors.');
        break;
      case PERSONALIZE_STATUS_PAUSED:
        $description = t('These personalizations are displaying the fallback/winner for each variation set, using JavaScript.');
        break;
      case PERSONALIZE_STATUS_COMPLETED:
        $description = t('These personalizations are displaying the fallback/winner for each variation set, using PHP whenever possible, which results in a faster display than Paused personalizations.');
        break;
    }
    $build['personalizations_list']['personalization_group_' . $status] = array(
      '#type' => 'container',
      '#attributes' => array(
        'id' => 'personalization-group-' . $status,
      ),
    );
    $build['personalizations_list']['personalization_group_' . $status]['description'] = array(
      '#markup' => theme('html_tag', array(
        'element' => array(
          '#tag' => 'p',
          '#value' => $description,
        )
      )),
    );

    // Build the personalizations table.
    $show_status_change = !$legacy && ($status != PERSONALIZE_STATUS_COMPLETED);
    $header = array();
    $header[] = array('data' => t('Name'));
    $header[] = array('data' => t('Start date'));
    $header[] = array('data' => t('Type'));
    if ($show_status_change) {
      $header[] = array('data' => t('Change status'));
    }
    $header[] = array('data' => t('Operations'));

    $rows = array();
    foreach ($agents as $agent) {
      $targeting = array();

      // Determine the agent type.
      $agent_type = $legacy ? $agent_type_legacy : $agent->plugin;
      if ($agent->plugin == 'acquia_lift_target') {
        // Load targeting audiences
        $targeting_os = acquia_lift_get_option_set_for_targeting($agent->machine_name);
        if (!empty($agent->data['lift_targeting'])) {
          // Use the currently saved targeting even if not yet implemented.
          $targeting = $agent->data['lift_targeting'];
        }
        else {
          // Use the stored targeting that is currently implemented.
          $targeting = acquia_lift_get_structure_from_targeting($targeting_os);
        }
        $agent_type = $agent_type_target;
        if (count($targeting) == 1) {
          $only_audience = reset($targeting);
          if (count($only_audience) > 1) {
            $agent_type = $agent_type_test;
          }
        }
        else if (count($targeting) > 1) {
          foreach ($targeting as $audience => $variations) {
            if (count($variations) > 1) {
              $agent_type = $agent_type_both;
              break;
            }
          }
        }
      }

      // The ability to delete depends on the storage type and on whether the campaign has started
      // or contains option sets.
      $can_delete = acquia_lift_agent_delete_access($agent);

      $delete_link = '';
      // Determine storage
      if ($can_delete) {
        switch ($agent->export_type) {
          case EXPORT_IN_DATABASE | EXPORT_IN_CODE:
            $delete_link = array(
              'title' => t('Revert'),
              'href' => 'admin/structure/personalize/manage/'. $agent->machine_name .'/delete-all',
              'attributes' => array(
                'class' => 'acquia-lift-delete',
                'title' => t('Delete @campaign', array('@campaign' => $agent->label)),
              ),
            );
            break;
          case EXPORT_IN_DATABASE:
            $delete_link = array(
              'title' => t('Delete'),
              'href' => 'admin/structure/personalize/manage/'. $agent->machine_name .'/delete-all',
              'attributes' => array(
                'class' => 'acquia-lift-delete',
                'title' => t('Delete @campaign', array('@campaign' => $agent->label)),
              ),
            );
            break;
        }
      }

      // Determine the start date to show (either schedule or actual).
      switch ($status) {
        case PERSONALIZE_STATUS_SCHEDULED:
        case PERSONALIZE_STATUS_NOT_STARTED:
          $start_date = personalize_agent_get_start_date($agent->machine_name);
          break;
        default:
          $start_date = isset($agent->started) ? $agent->started : 0;
          break;
      }

      // If there are any old tests that used to run for this agent, they will
      // be listed here as well.
      $old_tests = acquia_lift_get_retired_tests($agent->machine_name);
      // Lift target agents show reports if they contain tests.
      $show_report_link = $legacy || !empty($old_tests) || ($agent->plugin == 'acquia_lift_target' && !empty($agent->started) && in_array($agent_type, array($agent_type_test, $agent_type_both)));

      // Generate operations list.
      $ops = array();
      if (!$legacy) {
        $ops[] = array(
          'title' => t('Edit'),
          'href' => 'admin/structure/personalize/manage/'. $agent->machine_name . '/variations',
          'attributes' => array(
            'class' => array('acquia-lift-edit'),
            'title' => t('Edit @campaign', array('@campaign' => $agent->label)),
          ),
        );
      }
      if ($show_report_link) {
        $href = $legacy ? 'admin/structure/personalize/legacy/'. $agent->machine_name .'/results' :
          'admin/structure/personalize/manage/'. $agent->machine_name .'/results';
        $ops[] = array(
          'title' => t('View reports'),
          'href' => $href,
          'attributes' => array(
            'class' => array('acquia-lift-report'),
            'title' => t('View reports for @campaign', array('@campaign' => $agent->label)),
          ),
        );
      }
      if ($legacy && $status != PERSONALIZE_STATUS_COMPLETED) {
        $ops[] = array(
          'title' => t('Complete'),
          'href' => 'admin/structure/personalize/manage/'. $agent->machine_name .'/complete',
          'query' => array('destination' => current_path() . '#personalization-group-' . PERSONALIZE_STATUS_COMPLETED),
          'attributes' => array(
            'class' => array('acquia-lift-complete'),
            'title' => t('Complete @campaign (WARNING: action is final)', array('@campaign' => $agent->label)),
          ),
        );
      }
      if (!empty($delete_link)) {
        $delete_link['query'] = array('destination' => current_path());
        $ops[] = $delete_link;
      }

      // Generate actual table row data.
      $tablerow = array();
      $tablerow[] = array('data' => check_plain($agent->label), 'class' => array('acquia-lift-campaign-title'));
      $tablerow[] = array('data' => $start_date > 0 ? format_date($start_date, 'custom', 'M d, Y') : '');
      $tablerow[] = array('data' => $agent_type);
      if ($show_status_change) {
        $status_change_form = drupal_get_form("personalize_change_status_{$agent->machine_name}_form", $agent);
        $tablerow[] = array('data' => drupal_render($status_change_form));
      }
      $tablerow[] = array('data' => theme('links', array('links' => $ops, 'attributes' => array('class' => array('acquia-lift-list-operations')))));

      $campaign_class = $agent->plugin == 'acquia_lift_target' && !empty($targeting_os->targeting) ? array('acquia-lift-personalize-list-campaign') : array();
      $rows[] = array(
        'data' => $tablerow,
        'no_striping' => TRUE,
        'class' => $campaign_class,
      );

      // Add rows for targeting.
      if (!empty($targeting_os->targeting)) {
        foreach ($targeting_os->targeting as $audience_id => $audience) {
          if (empty($targeting[$audience_id])) {
            continue;
          }
          $variations = $targeting[$audience_id];
          $tablerow = array();
          $tablerow[] = array('data' => !empty($audience['label']) ? check_plain($audience['label']) : $audience_id, 'class' => 'acquia-lift-campaign-title');
          $tablerow[] =  '';
          $tablerow[] = count($variations) > 1 ? t('Test') : t('Target');
          if ($show_status_change) {
            $tablerow[] = '';
          }

          $ops = array();
          if ($show_report_link && count($variations) > 1) {
            $ops[] = array(
              'title' => t('View reports'),
              'href' => 'admin/structure/personalize/manage/' . $agent->machine_name . '/results',
              'query' => array(
                'targeting_audience' => $audience_id,
              ),
              'attributes' => array(
                'class' => array('acquia-lift-report'),
                'title' => t('View reports for @campaign: @audience', array(
                  '@campaign' => $agent->label,
                  '@audience' => empty($audience['label']) ? $audience_id : $audience['label'],
                )),
              ),
            );
          }
          if (count($variations) > 1 && !empty($agent->started) && $status != PERSONALIZE_STATUS_COMPLETED) {
            $ops[] = array(
              'title' => t('End test and choose winner'),
              'href' => 'admin/structure/personalize/manage/' . $agent->machine_name . '/audience/' . $audience_id . '/complete',
              'attributes' => array(
                'class' => array('acquia-lift-complete', 'ctools-use-modal', 'ctools-modal-acquia-lift-style'),
                'title' => t('End test and choose winner'),
              ),
            );
          }
          $tablerow[] = array('data' => theme('links', array('links' => $ops, 'attributes' => array('class' => array('acquia-lift-list-operations')))));;
          $rows[] = array(
            'data' => $tablerow,
            'no_striping' => TRUE,
            'class' => array('acquia-lift-personalize-list-audience', 'element-hidden'),
          );
        }
      }
      // List retired tests.
      foreach ($old_tests as $test) {
        $tablerow = array();
        $tablerow[] = array('data' => t('(retired test) ') . $test->label, 'class' => 'acquia-lift-campaign-title');
        $tablerow[] =  '';
        $tablerow[] = t('Retired test');
        if ($show_status_change) {
          $tablerow[] = '';
        }
        $ops = array(
          // All retired tests have reports.
          array(
            'title' => t('View reports'),
            'href' => 'admin/structure/personalize/manage/' . $agent->machine_name . '/results',
            'query' => array(
              'lift_retired_test' => $test->machine_name,
            ),
            'attributes' => array(
              'class' => array('acquia-lift-report'),
              'title' => t('View reports for retired test @test', array('@test' => $test->label)),
            )
          ),
          // All retired tests can be deleted.
          array(
            'title' => t('Delete'),
            'href' => 'admin/structure/personalize/manage/'. $test->machine_name .'/delete-all',
            'attributes' => array(
              'class' => 'acquia-lift-delete',
              'title' => t('Delete retired test @test', array('@test' => $test->label)),
            ),
          )
        );

        $tablerow[] = array('data' => theme('links', array('links' => $ops, 'attributes' => array('class' => array('acquia-lift-list-operations')))));

        $rows[] = array(
          'data' => $tablerow,
          'no_striping' => TRUE,
          'class' => array('acquia-lift-personalize-list-audience', 'element-hidden'),
        );
      }
    }

    $build['personalizations_list']['personalization_group_' . $status]['table'] = array(
      '#theme' => 'table',
      '#header' => $header,
      '#rows' => $rows,
      '#attributes' => array(
        'id' => 'personalize'
      ),
    );
  }
  return $build;
}

/**
 * Sort function for agents by start time and then label.
 */
function acquia_lift_sort_agent_started($a, $b) {
  if (!isset($a->started) || !isset($b->started) || $a->started == $b->started) {
    return strcmp(drupal_strtolower($a->label), drupal_strtolower($b->label));
  }
  return $a->started < $b->started ? -1 : 1;
}

/**
 * Form for deleting an agent and all its variation sets.
 */
function acquia_lift_agent_delete_form($form, $form_state, $agent) {
  $form['machine_name'] = array('#type' => 'hidden', '#value' => $agent->machine_name);
  $form['title'] = array('#type' => 'hidden', '#value' => $agent->label);
  $form['plugin'] = array('#type' => 'hidden', '#value' => $agent->plugin);
  $agent_type = in_array($agent->plugin, array(ACQUIA_LIFT_TESTING_AGENT_V1, ACQUIA_LIFT_TESTING_AGENT_V2)) ? 'test' : 'personalization';

  return confirm_form($form, t('Are you sure you want to delete the %agent %title?', array('%agent' => $agent_type, '%title' => $agent->label)), 'admin/structure/personalize', '', t('Delete'), t('Cancel'));
}

/**
 * Submit handler for agent deletion form.
 */
function acquia_lift_agent_delete_form_submit($form, &$form_state) {
  $agent_name = $form_state['values']['machine_name'];
  $agent_type = in_array($form_state['values']['plugin'], array(ACQUIA_LIFT_TESTING_AGENT_V1, ACQUIA_LIFT_TESTING_AGENT_V2)) ? 'test' : 'personalization';
  // First delete all option sets belonging to the agent.
  $option_sets = personalize_option_set_load_by_agent($agent_name);
  foreach ($option_sets as $option_set) {
    personalize_option_set_delete($option_set->osid);
  }
  personalize_agent_delete($form_state['values']['machine_name']);
  drupal_set_message(t('The %agent %name has been deleted.', array('%agent' => $agent_type, '%name' => $form_state['values']['title'])));
  $form_state['redirect'] = 'admin/structure/personalize';
}

/**
 * =======================================================================
 *  A C Q U I A  L I F T  A G E N T  R E P O R T I N G
 * =======================================================================
 */

/**
 * Menu callback for a report to be displayed within the wizard.
 *
 * @param $agent_data
 *   The loaded agent to show a report for.
 * @return array
 *   A render array for the report page.
 */
function acquia_lift_report_wizard($agent_data) {
  module_load_include('inc', 'personalize', 'personalize.admin.campaign');
  $build = array(
    'wizard' => drupal_get_form('personalize_campaign_wizard', $agent_data, 'results'),
    'report' => drupal_get_form('acquia_lift_report', $agent_data)
  );
  return $build;
}

/**
 * Form build function for the Acquia Lift report, which has filters.
 *
 * @param stdClass $agent_data
 *   The campaign agent data for this report.
 * @param stdClass $option_set
 *   (optional) The content variation to show in the default view.
 */
function acquia_lift_report($form, &$form_state, $agent_data) {
  $form = array(
    '#prefix' => '<div id="acquia-lift-reports">',
    '#suffix' => '</div>',
    '#attached' => array(
      'library' => array(
        array('acquia_lift', 'acquia_lift.reports'),
      ),
    ),
  );
  if (!acquia_lift_is_targeting_agent($agent_data)) {
    $legacy_campaigns = variable_get('acquia_lift_legacy_agents', array());
    if (in_array($agent_data->machine_name, $legacy_campaigns)) {
      $form['report'] = acquia_lift_report_custom($form_state, $agent_data, TRUE);
      return $form;
    }
    else {
      return array();
    }
  }
  if (isset($_GET['lift_retired_test'])) {
    if ($nested_agent = personalize_agent_load($_GET['lift_retired_test'])) {
      $form['report'] = acquia_lift_report_custom($form_state, $nested_agent, FALSE, $agent_data);
      return $form;
    }
  }
  $option_set = acquia_lift_get_option_set_for_targeting($agent_data->machine_name);

  if (empty($option_set->targeting)) {
    return array();
  }
  // Display a report for the specified audience.
  if (isset($form_state['values']['audience_filter'])) {
    $audience = $form_state['values']['audience_filter'];
  }
  else {
    $audience = isset($_GET['targeting_audience']) ? 'audience|' . $_GET['targeting_audience'] : NULL;
  }
  // Get audience options.
  $audience_options = array();
  foreach($option_set->targeting as $audience_name => $targeting) {
    if (isset($targeting['osid'])) {
      $audience_options['audience|' . $audience_name] = $targeting['label'];
    }
  }
  // Get retired tests as well.
  $old_tests = acquia_lift_get_retired_tests($agent_data->machine_name);
  foreach($old_tests as $retired_test) {
    $audience_options['retired|' . $retired_test->machine_name] = $retired_test->label;
  }

  // If no audience specified, or one that does not have a test, use the first
  // audience option if there is one.
  if (!empty($audience)) {
    list($test_type, $test_id) = explode('|', $audience);
  }
  if (empty($audience) || ($test_type == 'audience' && !isset($option_set->targeting[$test_id]['osid']))) {
    if (empty($audience_options)) {
      return array(
        'no_report' => array(
          '#markup' => t('This personalization does not contain tests, no reports to show.')
        )
      );
    }
    $audience = key($audience_options);
    list($test_type, $test_id) = explode('|', $audience);
  }
  if ($test_type == 'retired') {
    $nested_agent = personalize_agent_load($test_id);
    $test_label = $nested_agent->label;
  }
  else {
    $test_label = $option_set->targeting[$test_id]['label'];
  }

  // Show the audience filter.
  $form['audience_container'] = array(
    '#type' => 'container',
    '#attributes' => array(
      'class' => array('container-inline'),
    ),
    '#tree' => FALSE,
  );
  if (count($audience_options) == 1) {
    $form['audience_container']['audience_label'] = array(
      '#markup' => '<h2>' . t('Test for %audience audience', array('%audience' => $test_label)) . '</h2>'
    );
    $form['audience_container']['audience_filter'] = array(
      '#type' => 'value',
      '#value' => $audience
    );
  }
  else {
    $form['audience_container']['audience_filter'] = array(
      '#title' => t('Audience'),
      '#type' => 'select',
      '#options' => $audience_options,
      '#default_value' => $audience,
      '#ajax' => array(
        'callback' => "acquia_lift_report_ajax_callback",
        'wrapper' => "acquia-lift-reports",
      ),
      '#id' => 'acquia-lift-report-audience-filter',
    );
  }
  if ($test_type == 'audience' && personalize_agent_get_status($agent_data->machine_name) != PERSONALIZE_STATUS_COMPLETED) {
    $form['audience_container']['complete'] = array(
      '#type' => 'link',
      '#title' => t('End test and choose winner'),
      '#href' => 'admin/structure/personalize/manage/' . $agent_data->machine_name . '/audience/' . $test_id . '/complete',
      '#attributes' => array(
        'class' => array(
          'ctools-use-modal',
          'ctools-modal-acquia-lift-style',
          'action-item-primary-active',
          'acquia-lift-submit-button',
          'acquia-lift-complete-audience',
        )
      )
    );
  }

  if ($test_type == 'retired') {
    $nested_agent = personalize_agent_load($test_id);
    $form['report'] = acquia_lift_report_custom($form_state, $nested_agent, FALSE, $agent_data);
    return $form;
  }
  if ($nested_os = personalize_option_set_load($option_set->targeting[$test_id]['osid'])) {
    $nested_agent = personalize_agent_load($nested_os->agent);
    if (!empty($nested_agent)) {
      $form['report'] = acquia_lift_report_custom($form_state, $nested_agent, FALSE, $agent_data);
    }
  }

  return $form;
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
 * @param stdClass $agent_data
 *   The campaign agent data for this report.
 * @param stdClass $option_set
 *   (optional) The content variation to show in the default view.
 * @param stdClass $parent_agent
 *   (optional) The parent agent if this is a nested test.
 */
function acquia_lift_report_custom(&$form_state, $agent_data, $legacy = FALSE, $parent_agent = NULL) {

  if ($agent_data->started == 0) {
    return array(
      'no_report' => array(
        '#markup'=> t('This agent has not started running yet, no reports to show.')
      )
    );
  }
  // Instantiate the agent class to perform some further checks before trying to
  // show reports for it.
  if (!$agent = personalize_agent_load_agent($agent_data->machine_name)) {
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

  // If this isn't a learning agent, no reports to show..
  if (!$agent instanceof AcquiaLiftLearningAgentInterface) {
    return array(
      'no_report' => array(
        '#markup' => t('This agent does not support reporting.')
      )
    );
  }

  // Check for Rickshaw and D3 libraries and alert users if not exist.
  if (_acquia_lift_missing_library_warning(array('rickshaw', 'd3'), t('The following libraries are required in order to view the Acquia Lift reports:'))) {
    return array();
  }
  if ($legacy) {
    // The file containing the legacy report class needs to be explicitly loaded.
    module_load_include('inc', 'acquia_lift', 'includes/AcquiaLiftReportLegacy');
  }
  $agent_name = $agent->getMachineName();
  // Generate report filters.
  $data = $agent->getData();

  $form['report_filters'] = array(
    '#type' => 'container',
    '#tree' => FALSE,
    '#attributes' => array(
      'class' => array('acquia-lift-report-filters', 'clearfix'),
    ),
  );

  // Get the decision points for this agent so we can provide a filter on this.
  $option_sets = personalize_option_set_load_by_agent($agent_name);
  if ($legacy) {
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
      $decision_point = key($decisions);
    }
    $form['report_filters']['decision_point'] = acquia_lift_report_decision_point_dropdown($decision_options, $decision_point, $agent_name);
  }
  else {
    $os = reset($option_sets);
    $decision_point = $os->decision_point;
  }

  list($date_start_report, $date_end_report) = acquia_lift_get_report_dates_for_agent($agent_data);

  // Conversion report filters.
  $selected_goal = empty($form_state['values']['goal']) ? NULL : $form_state['values']['goal'];
  $selected_metric = empty($form_state['values']['metric']) ? 'rate' : $form_state['values']['metric'];

  $options = array(
    'decision' => $decision_point,
    'start' => $date_start_report,
    'end' => $date_end_report,
    'goal' => $selected_goal,
    'conversion_metric' => $selected_metric,
    'legacy' => $legacy
  );
  $reports = $agent->getCampaignReports($options);
  if (!($reports['#has_data'])) {
    // This campaign hasn't been shown yet, so there is no data for reporting.
    return array(
      'no_report' => array(
        '#markup' => t('This personalization does not yet contain information about your visitors\' website interactions. This situation generally occurs for personalizations that have either just been started or do not generate much website traffic.'),
      ),
    );
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
  // Show the test (explore)  percentage if in adaptive mode.
  if ($data['decision_style'] == 'adaptive') {
    $percentage = $data['explore_rate'];
    $form['experiment_report']['header']['title']['groups'] = array(
      '#markup' => t('(@percentage%)', array('@percentage' => $percentage)),
    );
    // Also add help text to the heading to explain what this is.
    $form['experiment_report']['header']['title']['#attributes']['data-help-tooltip'] = t('The portion of visitors who were shown a random variation, as opposed to the optimized variation. The data below are for the experiment group only.');
  }

  // Get the conversion report options.
  $goals_agent = !empty($parent_agent) ? $parent_agent->machine_name : $agent->getMachineName();
  $form['experiment_report']['header']['options'] = array(
    '#type' => 'container',
    '#attributes' => array(
      'class' => array('acquia-lift-report-section-options'),
    ),
    '#tree' => FALSE,
    'goal' => acquia_lift_report_goal_dropdown($goals_agent, $selected_goal),
    'metric' => acquia_lift_report_experiment_metric_dropdown($selected_metric),
    'submit' => array(
      '#type' => 'submit',
      '#value' => t('Filter'),
    ),
  );
  $form['experiment_report']['header']['summary']['report_summary'] = array(
    '#theme_wrappers' => array('container'),
    '#markup' => t('See which content variations are winning.'),
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
    acquia_lift_legacy_report_form($form, $form_state, $reports);
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
      'callback' => 'acquia_lift_report_ajax_callback',
      'wrapper' => 'acquia-lift-reports',
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
function acquia_lift_report_experiment_metric_dropdown($selected) {
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
 * Returns an array with start date and end date in Y-m-d format.
 *
 * @param $agent_data
 *   The agent to get report dates for
 * @return array
 *   A two-element array, the first element of which is the start date in Y-m-d
 *   format, the second of which is the end date in Y-m-d format.
 */
function acquia_lift_get_report_dates_for_agent($agent_data) {
  // Default to showing the complete history of the campaign.
  $start = $agent_data->started;
  $end = time();
  // If the campaign's status is "completed" then we need use the date it ended.
  $status = variable_get(_personalize_agent_get_status_variable($agent_data->machine_name));
  if ($status == PERSONALIZE_STATUS_COMPLETED && $end_time = variable_get(_personalize_agent_get_stoptime_variable($agent_data->machine_name), 0)) {
    $end = $end_time;
  }
  // Ensure our time span is within the max number of days.
  $start = acquia_lift_adjust_report_start_date($start, $end);
  $date_start_report = date('Y-m-d', $start);
  $date_end_report = date('Y-m-d', $end);
  return array($date_start_report, $date_end_report);
}

/**
 * Adjusts the start time so that the time interval is within the max.
 *
 * @param int $start
 *   Timestamp representing the start date for the report.
 * @param int $end
 *   Timestamp representing the end date for the report.
 * @return int
 *   The adjusted start date as a timestamp.
 */
function acquia_lift_adjust_report_start_date($start, $end) {
  $num_days = floor(($end - $start) / 86400);
  $max_days = variable_get('acquia_lift_report_max_days', 14);
  if ($num_days <= $max_days) {
    return $start;
  }
  // Move the start date forward so that we are within the max.
  $num_seconds = $max_days * 86400;
  return $end - $num_seconds;
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
function acquia_lift_report_experiment() {
  $params = drupal_get_query_parameters();
  $agent_name = empty($params['campaign']) ? NULL : personalize_sanitize_string($params['campaign']);
  $options = array();
  $options['decision'] = empty($params['decision']) ? NULL : personalize_sanitize_string($params['decision']);
  $options['goal'] = empty($params['goal']) ? NULL : personalize_sanitize_string($params['goal']);
  $options['start'] = empty($params['start']) ? NULL : check_plain($params['start']);
  $options['end'] = empty($params['end']) ? NULL : check_plain($params['end']);

  if (is_numeric($options['start'])) {
    $options['start'] = date('Y-m-d', $options['start']);
  }
  if (is_numeric($options['end'])) {
    $options['end'] = date('Y-m-d', $options['end']);
  }

  $plugin = personalize_agent_load_agent($agent_name);
  if ($plugin instanceof AcquiaLiftLearningAgentInterface) {
    $reports = $plugin->getExperimentReports($options);
    $reports['#type'] = 'ajax';
    drupal_json_output(drupal_render($reports));
  }
  else {
    $report = array(
      '#markup' => t('No report available for this personalization.')
    );
    drupal_json_output(drupal_render($report));
  }

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
    acquia_lift_implement_mvt($agent_data);
  }
  else {
    acquia_lift_implement_targeting($agent_data);
  }
  // Once we implement the structure for a personalization, we do not allow
  // adding or removing variation sets.
  // @todo This can be revisited later, it is simply too cumbersome to support
  //   such post-implementation changes at this time and the use-case of multiple
  //   variation sets so is small that it does not warrant such effort.
  $agent_data->data['locked_for_variation_sets'] = TRUE;
  personalize_agent_save($agent_data);
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
    // Just update the test options for any embedded tests and return an empty
    // array.
    acquia_lift_save_nested_test_options($agent);
    return;
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
  foreach (array_keys($existing_tests) as $audience) {
    if (!in_array($audience, $rules)) {
      // Delete the old test if it's no longer being used.
      acquia_lift_remove_nested_test($targeting_option_set, $audience, $agent);
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
  $targeting_option_set->options = array_values($targeting_option_set->options);
  foreach ($all_option_sets as $os) {
    if ($os->osid != $targeting_option_set->osid) {
      $os->targeting = $targeting_option_set->targeting;
      $os->options = array_values($os->options);
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
 * Removes a nested test from an audience.
 *
 * This does not actually delete the test - it is kept around so that reports
 * can be viewed, but the test is removed from the audience.
 *
 * @param $option_set
 *   The option set whose targeting is being changed.
 * @param $audience
 *   THe name of the target audience the test was assigned to.
 * @throws \PersonalizeException
 */
function acquia_lift_remove_nested_test(&$option_set, $audience, $parent_agent) {
  // Find the nested option set for the specified targeting rule.
  if (isset($option_set->targeting[$audience]) && isset($option_set->targeting[$audience]['osid'])) {
    $nested_os = personalize_option_set_load($option_set->targeting[$audience]['osid']);
    // Retire the test.
    $nested_agent = personalize_agent_load($nested_os->agent);
    $audience_label = isset($option_set->targeting[$audience]['label']) ? $option_set->targeting[$audience]['label'] : $audience;
    acquia_lift_retire_test($nested_agent, $parent_agent->machine_name, $parent_agent->label, $audience, $audience_label);
    unset($option_set->targeting[$audience]['osid']);
  }
}

/**
 * Retires a nested test, preserving information about what it had been used for.
 *
 * @param $test
 * @param $parent_name
 * @param $parent_label
 * @param $audience_name
 * @param $audience_label
 */
function acquia_lift_retire_test($test, $parent_name, $parent_label, $audience_name, $audience_label) {
  $test->data['lift_retired'] = 1;
  $test->data['lift_parent'] = $parent_name;
  $test->data['lift_audience'] = $audience_name;
  personalize_agent_set_status($test->machine_name, PERSONALIZE_STATUS_COMPLETED);
  $test->label = $parent_label . ': ' . $audience_label . t(" (Completed @date)", array('@date' => date("Y-m-d")));
  personalize_agent_save($test);
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
      if (empty($nested_option_set)) {
        // @todo: Ensure that any deleted option sets are removed from nested
        // targeting.
        continue;
      }
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
  $tests = acquia_lift_get_nested_tests($parent_agent);
  if (empty($tests)) {
    return;
  }
  $explore_rate = 100;
  $decision_style = 'random';
  if (isset($parent_agent->data['decision_style']) && $parent_agent->data['decision_style'] == 'adaptive') {
    $decision_style = 'adaptive';
    $explore_rate = isset($parent_agent->data['explore_rate']) ? $parent_agent->data['explore_rate'] : 20;
  }
  $agent_data = array(
    'control_rate' => isset($parent_agent->data['control_rate']) ? $parent_agent->data['control_rate'] : 0,
    'explore_rate' => $explore_rate,
    'decision_style' => $decision_style,
  );

  foreach ($tests as $test) {
    $nested_agent = personalize_agent_load($test);
    $nested_agent->data = $agent_data;
    personalize_agent_save($nested_agent);
  }
}

/**
 * Validate that an agent has the minimum targeting required and add it if it
 * is not available.
 *
 * Targeting agents should have a default "Everyone else" audience that has all
 * unassigned variations assigned.
 *
 * @param stdClass $agent
 *   The object representing the agent that owns the targeting option set.
 */
function acquia_lift_validate_minimum_targeting($agent, $option_set) {
  // Check and create the "Everyone else" audience if it doesn't exist.
  $targeting_option_set = acquia_lift_get_option_set_for_targeting($agent->machine_name);
  if ($targeting_option_set->osid == $option_set->osid) {
    $targeting_option_set = $option_set;
  }
  if (empty($targeting_option_set->targeting)) {
    $audience = _acquia_lift_personalize_campaign_wizard_everyone_else_audience();
    acquia_lift_target_audience_save($audience['name'], $agent->machine_name, $audience['contexts'], $audience['strategy'], $audience['weight'], $audience['id']);
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
      acquia_lift_implement_test_structure($agent_data);
      acquia_lift_batch_sync_tests_for_agent($agent_data, $next_status);
      // Unset the started property which will have been changed during
      // status update and we don't want it getting clobbered by other
      // saves of the agent.
      unset($agent_data->started);
      return TRUE;

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
    // @todo Should it be deleted from Lift?
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
    'name' => t('Everyone'),
    'contexts' => array(),
    'weight' => 1,
    'strategy' => 'OR',
    'id' => ACQUIA_LIFT_TARGETING_EVERYONE_ELSE,
  );
}

/**
 * Stops a test for the specified audience of the specified targeting agent.
 *
 * @param $agent
 *   The targeting agent.
 * @param $audience
 *   The audience to stop the test for.
 * @param $winner
 *   (optional) The winning variation id to assign to the audience in place of
 *   the test.
 * @throws \AcquiaLiftException
 * @throws \PersonalizeException
 */
function acquia_lift_stop_test_for_audience($agent, $audience, $winner = NULL) {
  $option_set = acquia_lift_get_option_set_for_targeting($agent->machine_name);
  if (empty($option_set) || !isset($option_set->targeting[$audience]) || !isset($option_set->targeting[$audience]['osid'])) {
    throw new AcquiaLiftException('There is no test for this audience.');
  }
  $nested_os = personalize_option_set_load($option_set->targeting[$audience]['osid']);
  // Assign the "winning" option to the audience. If no winner provided we use
  // the first one.
  if (empty($winner)) {
    $winner = $nested_os->options[0]['option_id'];
  }
  unset($option_set->targeting[$audience]['osid']);
  $option_set->targeting[$audience]['option_id'] = $winner;
  try {
    personalize_option_set_save($option_set);
  }
  catch (PersonalizeException $e) {
    drupal_set_message(t('There was a problem updating the targeting for this personalization.'));
    // Don't retire the test, something's amiss.
    return;
  }
  if (isset($agent->data['lift_targeting'][$audience])) {
    $agent->data['lift_targeting'][$audience] = array($winner);
    personalize_agent_save($agent);
  }
  $nested_agent = personalize_agent_load($nested_os->agent);
  $audience_label = isset($option_set->targeting[$audience]['label']) ? $option_set->targeting[$audience]['label'] : $audience;
  acquia_lift_retire_test($nested_agent, $agent->machine_name, $agent->label, $audience, $audience_label);
}

/**
 * Returns any retired tests that exist for the specified agent.
 *
 * @param $agent_name
 *   The agent to get retired tests for.
 * @return array
 *   An array of test objects for the specified agent, or if no agent was specified
 *   an associative array whose keys are parent agents and whose values are
 *   arrays of test objects.
 */
function acquia_lift_get_retired_tests($agent_name = NULL) {
  $old_tests = &drupal_static(__FUNCTION__, NULL);
  if ($old_tests == NULL) {
    $old_tests = array();
    $test_agents = personalize_agent_load_by_type(ACQUIA_LIFT_TESTING_AGENT_V1);
    $test_agents = array_merge($test_agents, personalize_agent_load_by_type(ACQUIA_LIFT_TESTING_AGENT_V2));
    foreach ($test_agents as $name => $agent) {
      if (isset($agent->data['lift_parent'])) {
        if (!isset($old_tests[$agent->data['lift_parent']])) {
          $old_tests[$agent->data['lift_parent']] = array();
        }
        $old_tests[$agent->data['lift_parent']][] = $agent;
      }
    }
  }
  if (!empty($agent_name)) {
    return isset($old_tests[$agent_name]) ? $old_tests[$agent_name] : array();
  }
  return $old_tests;
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
  if (empty($agents)) {
    drupal_goto('admin/structure/personalize');
  }
  return acquia_lift_agent_list($agents, TRUE);
}

/**
 * Page callback to show the form to stop a test for a particular audience
 * in a CTools modal window.
 *
 * @param stdClass $agent_data
 *   The data for the campaign.
 * @param string $audience_id
 *   The id of the audience to complete.
 */
function acquia_lift_target_complete_audience_modal_callback($agent_data, $audience_id) {
  module_load_include('inc', 'acquia_lift', 'acquia_lift.admin.unibar');
  acquia_lift_create_ctools_form(
    t('Choose test winner'),
    'acquia_lift_target_complete_audience_form',
    array(
      'agent_data' => $agent_data,
      'audience_id' => $audience_id,
    ),
    'acquia_lift_target_complete_audience_complete_callback'
  );
}

/**
 * Form handler to complete an audience within a campaign.
 *
 * This form is called within a ctools modal window.
 *
 * @param stdClass $agent_data
 *   The data for the campaign.
 * @param string $audience_id
 *   The id of the audience to complete.
 */
function acquia_lift_target_complete_audience_form($form, &$form_state, $agent_data, $audience_id) {
  ctools_include('modal');
  ctools_include('ajax');
  ctools_add_js('ajax-responder');

  // As this is just a small modal form without validation, don't show any
  // dsm messages as they would be repetitive of what is on the main page.
  drupal_get_messages();

  $option_set = acquia_lift_get_option_set_for_targeting($agent_data->machine_name);
  $audience_label = isset($option_set->targeting[$audience_id]['label']) ? $option_set->targeting[$audience_id]['label'] : $audience_id;
  if (!isset($option_set->targeting[$audience_id])) {
    $form['error'] = array(
      '#markup' => t('The specified audience does not exist in the %name personalization.  Close this window and refresh in order to see current options.', array(
        '%name' => $agent_data->label,
      )),
    );
    return $form;
  }
  else if (!isset($option_set->targeting[$audience_id]['osid'])) {
    $form['error'] = array(
      '#markup' => t('The %audience audience does not include a test.  Close this window and refresh in order to see current tests.', array(
        '%audience' => $audience_label,
      )),
    );
  }
  // Get all the display variations for the option set.
  $option_sets = personalize_option_set_load_by_agent($agent_data->machine_name);

  module_load_include('inc', 'acquia_lift', 'acquia_lift.admin.wizard');
  $all_variations = _acquia_lift_personalize_campaign_wizard_variation_displays($option_sets, isset($agent_data->data['variation_set_handling']) ? $agent_data->data['variation_set_handling'] : NULL, FALSE);

  $has_multiple = count($option_sets) > 1;
  foreach($all_variations as $displays) {
    $variation_displays = array();
    $targeting_option_id = '';
    foreach ($displays as $option) {
      if ($has_multiple && is_array($option) && isset($option['option_set_label'])) {
        $variation_displays[] = theme('acquia_lift_single_variation', array('option' => $option['option_label'], 'option_set' => $option['option_set_label']));
      }
      else {
        $variation_displays[] = is_string($option) ? $option : $option['option_label'];
      }
      if (!$has_multiple || !empty($option['targeting'])) {
        $targeting_option_id = $option['option_id'];
      }
    }
    // The targeting option id could remain empty if there are invalid
    // combinations of options resulting in a smaller number of variations in
    // the option set used for targeting.
    if (!empty($targeting_option_id)) {
      $variation_options[$targeting_option_id] = implode(', ', $variation_displays);
    }
  }

  $existing_targeting = acquia_lift_get_structure_from_targeting($option_set);
  $options = array();
  foreach ($existing_targeting[$audience_id] as $option_id) {
    $options[$option_id] = $variation_options[$option_id];
  }

  $instructions = t('Choose a winner for this test for the %audience audience.', array(
    '%audience' => $audience_label,
  ));
  $instructions .= '<br />';
  $instructions .=   t('After you select a winner, the test will end.');
  $instructions .= '<br /><span class="acquia-lift-alert"><em>';
  $instructions .= t('This action cannot be undone!');
  $instructions .= '</em></span>';
  $form = array();
  $form['audience_id'] = array(
    '#type' => 'value',
    '#value' => $audience_id,
  );
  $form['audience_label'] = array(
    '#type' => 'value',
    '#value' => $audience_label,
  );
  $form['agent_data'] = array(
    '#type' => 'value',
    '#value' => $agent_data,
  );
  $form['instructions'] = array(
    '#markup' => $instructions,
  );
  $form['winner'] = array(
    '#type' => 'radios',
    '#options' => $options,
    '#required' => TRUE,
    '#title' => t('Winning variation'),
    '#title_display' => 'invisible',
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Complete test'),
    '#attributes' => array(
      'class' => array('action-item-primary-active acquia-lift-submit-button'),
    ),
  );
  return $form;
}

/**
 * Submit handler for acquia_lift_target_complete_audience_form().
 */
function acquia_lift_target_complete_audience_form_submit($form, &$form_state) {
  try {
    acquia_lift_stop_test_for_audience($form_state['values']['agent_data'], $form_state['values']['audience_id'], $form_state['values']['winner']);
  } catch (Exception $e) {
    drupal_set_message($e->getMessage(), 'error');
    return;
  }
  drupal_set_message(t('The winner has been set for the %audience audience.', array(
    '%audience' => $form_state['values']['audience_label'],
  )));
}

/**
 * Ctools form processing complete handler for the completing of an audience
 * test.
 *
 * @see acquia_lift_create_ctools_form().
 */
function acquia_lift_target_complete_audience_complete_callback($form, $form_state) {
  $commands = array();
  $commands[] = ctools_ajax_command_reload();
  return $commands;
}
