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
  $account_info = acquia_lift_get_account_info();

  $account_info_provided = !empty($account_info['public_key']) && !empty($account_info['private_key']);

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
  }

  $form['acquia_lift_account_info'] = array(
    '#type' => 'fieldset',
    '#title' => 'Acquia Lift Account Settings',
    '#tree' => FALSE,
    '#collapsible' => TRUE,
    '#collapsed' => $account_info_provided
  );
  if ($account_info_provided) {
    $form['acquia_lift_account_info']['msg'] = array(
      '#markup' => t("<p>We automatically obtained your Lift Decision credentials from your Acquia Account. Everything is good to go!</p>", array(
        '!acquialift' => l(t('Acquia Lift'), 'http://www.acquia.com/products-services/website-personalization', array('attributes' => array('target' => '_blank'))),
        '!advocacyemail' => l('advocacy@acquia.com', 'mailto:advocacy@acquia.com')
      )),
    );
    $form['acquia_lift_account_info']['public_key'] = array(
      '#markup' => t("<p>Public Key: <strong>!publickey</strong></p>", array(
        '!publickey' => $account_info['public_key']
      )),
    );
    $form['acquia_lift_account_info']['apiurl'] = array(
      '#markup' => t("<p>API URL: <strong>!apiurl</strong></p>", array(
        '!apiurl' => $account_info['api_url']
      )),
    );
  }
  else {
    $form['acquia_lift_account_info']['msg'] = array(
      '#markup' => t("<p>It doesn't look like we could find your credentials. Have you enabled and configured the Acquia Connector module? Please contact support if you have purchased Acquia Lift and are connected with the Acquia Connector, otherwise, contact !advocacyemail to purchase a subscription to the !acquialift service.</p>", array(
        '!acquialift' => l(t('Acquia Lift'), 'http://www.acquia.com/products-services/website-personalization', array('attributes' => array('target' => '_blank'))),
        '!advocacyemail' => l('advocacy@acquia.com', 'mailto:advocacy@acquia.com')
      )),
    );
  }


  // Add in the JS path.
  $js_path_desc = false;
  $default_path = variable_get('acquia_lift_profiles_js_path', '');
  if (isset($account_info['js_path']) && $account_info['js_path'] != $default_path) {
    $js_path_desc = t("Suggested: !js_path", array('!js_path' => $account_info['js_path']));
    $default_path = $account_info['js_path'];
  }
  // Lift Web account info.
  $form['acquia_lift_account_info']['acquia_lift_profiles_js_path'] = array(
    '#type' => 'textfield',
    '#title' => t('Acquia Lift JavaScript path'),
    '#field_prefix' => 'http(s)://',
    '#default_value' => variable_get('acquia_lift_profiles_js_path', $default_path),
    '#required' => TRUE,
    '#description' => $js_path_desc ? $js_path_desc : false,
  );

  if ($account_info['profiles']['account_name'] != "") {
    $acc_name_desc = t("Short abbreviation of your account name. Suggested: @accountname", array('@accountname' => $account_info['profiles']['account_name']));
  }
  else {
    $acc_name_desc = t("Short abbreviation of your account name, such as MYACCOUNT01.");
  }
  $form['acquia_lift_account_info']['acquia_lift_profiles_account_name'] = array(
    '#type' => 'textfield',
    '#title' => t('Acquia Lift Web Account Name'),
    '#default_value' => variable_get('acquia_lift_profiles_account_name', $account_info['profiles']['account_name']),
    '#description' => $acc_name_desc,
    '#required' => TRUE,
  );

  $form['acquia_lift_account_info']['acquia_lift_profiles_site_name'] = array(
    '#type' => 'textfield',
    '#title' => t('Acquia Lift Web Customer Site'),
    '#default_value' => variable_get('acquia_lift_profiles_site_name', ''),
    '#description' => t("When you are using multiple sites with the same Acquia Lift Web account, use this. Otherwise, leave it empty")
  );

  $form['acquia_lift_account_info']['acquia_lift_profiles_api_url'] = array(
    '#type' => 'textfield',
    '#title' => t('Acquia Lift Web API URL'),
    '#field_prefix' => 'http(s)://',
    '#default_value' => variable_get('acquia_lift_profiles_api_url', $account_info['profiles']['hostname']),
    '#description' => t("Server where your Acquia Lift Web account resides."),
    '#required' => TRUE,
  );

  if ($account_info['profiles']['public_key'] != "" && variable_get('acquia_lift_profiles_access_key', '') == "<reset>") {
    $public_key_desc = t("Suggested: @public_key", array('@public_key' => $account_info['profiles']['public_key']));
  } else {
    $public_key_desc = t("Write %reset in the field to show the access key", array("%reset" => "<reset>"));
  }
  $form['acquia_lift_account_info']['acquia_lift_profiles_access_key'] = array(
    '#type' => 'textfield',
    '#title' => t('Acquia Lift Web API Access Key'),
    '#default_value' => variable_get('acquia_lift_profiles_access_key', $account_info['profiles']['public_key']),
    '#description' => $public_key_desc,
    '#required' => TRUE,
  );

  if ($account_info['profiles']['secret_key'] != "" && variable_get('acquia_lift_profiles_secret_key', '') == "<reset>") {
    $secret_key_desc = t("Suggested: @secret_key", array('@secret_key' => $account_info['profiles']['secret_key']));
  } else {
    $secret_key_desc = t("Write %reset in the field to show the secret key", array("%reset" => "<reset>"));
  }
  $form['acquia_lift_account_info']['acquia_lift_profiles_secret_key'] = array(
    '#type' => 'textfield',
    '#title' => t('Acquia Lift Web API Secret Key'),
    '#default_value' => variable_get('acquia_lift_profiles_secret_key', $account_info['profiles']['secret_key']),
    '#description' => $secret_key_desc,
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
    '#default_value' => variable_get('acquia_lift_report_max_days', ACQUIA_LIFT_DEFAULT_MAX_DAYS),
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
  $account_info = acquia_lift_get_account_info();
  $api = AcquiaLiftAPI::getInstance($account_info);
  if ($api->ping()) {
    drupal_set_message(t('Successfully connected to the Acquia Lift service'));
  }
  else {
    drupal_set_message(t('There was a problem connecting to the Acquia Lift service. Please check your subscription or ask Acquia Support for help.'), 'error');
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
  if (!valid_url($form_state['values']['acquia_lift_profiles_js_path'])) {
    form_set_error('acquia_lift_profiles_js_path', t('You must enter a valid JavaScript path'));
  }
  $form_state['values']['acquia_lift_profiles_js_path'] = preg_replace('/(^[a-z]+:\/\/)/i', '', $form_state['values']['acquia_lift_profiles_js_path']);

  // Validate the profiles api url.
  if (!valid_url($form_state['values']['acquia_lift_profiles_api_url'])) {
    form_set_error('acquia_lift_profiles_api_url', t('You must enter a valid API URL'));
  }
  // Strip any scheme from the API URL.
  $form_state['values']['acquia_lift_profiles_api_url'] = preg_replace('/(^[a-z]+:\/\/)/i', '', $form_state['values']['acquia_lift_profiles_api_url']);

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
function acquia_lift_agent_list($agents = array()) {
  $agent_type_test = t('Test only');
  $agent_type_target = t('Target only');
  $agent_type_both = t('Test and target');
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
    if ($agent->plugin !=  'acquia_lift_target') {
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
    $show_status_change = $status != PERSONALIZE_STATUS_COMPLETED;
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

      // The ability to delete depends on the storage type and on whether the
      // campaign has started or contains option sets.
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

      // Lift target agents show reports if they contain tests.
      $show_report_link = $agent->plugin == 'acquia_lift_target' && !empty($agent->started);

      // Generate operations list.
      $ops = array(
        array(
          'title' => t('Edit'),
          'href' => 'admin/structure/personalize/manage/'. $agent->machine_name . '/variations',
          'attributes' => array(
            'class' => array('acquia-lift-edit'),
            'title' => t('Edit @campaign', array('@campaign' => $agent->label)),
          ),
        )
      );

      if ($show_report_link) {
        $href = 'admin/structure/personalize/manage/'. $agent->machine_name .'/results';
        $ops[] = array(
          'title' => t('View reports'),
          'href' => $href,
          'attributes' => array(
            'class' => array('acquia-lift-report'),
            'title' => t('View reports for @campaign', array('@campaign' => $agent->label)),
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
      // If there are any old tests that used to run for this agent, they will
      // be listed here as well.
      $old_tests = acquia_lift_get_retired_tests($agent->machine_name);
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
  $agent_type = $agent->plugin == ACQUIA_LIFT_TESTING_AGENT ? 'test' : 'personalization';

  return confirm_form($form, t('Are you sure you want to delete the %agent %title?', array('%agent' => $agent_type, '%title' => $agent->label)), 'admin/structure/personalize', '', t('Delete'), t('Cancel'));
}

/**
 * Submit handler for agent deletion form.
 */
function acquia_lift_agent_delete_form_submit($form, &$form_state) {
  $agent_name = $form_state['values']['machine_name'];
  $agent_type = $form_state['values']['plugin'] == ACQUIA_LIFT_TESTING_AGENT ? 'test' : 'personalization';
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

function acquia_lift_report($form, &$form_state, $agent_data) {
  module_load_include('inc', 'acquia_lift', 'includes/AcquiaLiftReportBase');
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
    return array();
  }
  $path = $_GET['q'];
  if ($path == 'system/ajax') {
    $path = $form_state['values']['path'];
  }
  $form['path'] = array(
    '#type' => 'hidden',
    '#value' => $path
  );
  $use_old_stats = isset($_GET['use_old_stats']) || !empty($form_state['values']['use_old_stats']);
  $date_upgraded = variable_get('acquia_lift_report_upgrade_timestamp', 0);
  if ($agent_data->started < $date_upgraded && !$use_old_stats) {
    drupal_set_message(t('Only displaying data starting from @date, which is the date Lift was upgraded to use the new reporting API. Click !here to show the old reports for any tests contained in this personalization', array('@date' => date('Y-m-d', $date_upgraded), '!here' => l('here', $path, array('query' => array('use_old_stats' => 1))))), 'warning');
  }
  $option_set = acquia_lift_get_option_set_for_targeting($agent_data->machine_name);
  $option_ids = array_map(function($option) {return $option['option_id'];}, $option_set->options);
  // First get the overview report, i.e. total decisions and conversions across
  // all audiences.
  $account_info = acquia_lift_get_account_info();
  $lift_api = AcquiaLiftAPI::getInstance($account_info);
  try {
    $overview_stats = $lift_api->getPersonalizationOverviewReport($agent_data->machine_name, $option_ids);
  }
  catch (AcquiaLiftException $e) {
    $overview_stats = array();
  }
  list($start_time, $end_time) = acquia_lift_get_report_dates_for_agent($agent_data, FALSE);
  $from_datetime = date_create();
  date_timestamp_set($from_datetime, $start_time);
  $to_datetime = date_create();
  date_timestamp_set($to_datetime, $end_time);
  $interval = date_diff($from_datetime, $to_datetime);
  $form['overview_report'] = array(
    'overview_report_title' => array(
      '#markup' => '<h2>' . t('Personalization Overview') . '</h2>',
    ),
    '#theme_wrappers' => array('container'),
    '#attributes' => array(
      'id' => 'acquia-lift-overview-report',
      'class' => array('acquia-lift-report-section', 'clearfix'),
    ),
  );
  $decision_count = $conversion_count = 0;
  $variation_counts = array();
  if (isset($overview_stats['error'])) {
    drupal_set_message(t("There was a problem retrieving the reports for your personalization - please try again later."), 'error');
    return array();
  }
  foreach ($overview_stats as $variation => $stats) {
    $decision_count += $stats['decision_count'];
    $conversion_count += $stats['conversion_count'];
    $variation_counts[$stats['variation_id']] = array(
      'option_label' => _get_variation_label($stats['variation_id'], $agent_data->machine_name),
      'decision_count' => $stats['decision_count'],
      'goal_count' => $stats['conversion_count'],
    );
  }
  $overview_report = _build_overview_report($decision_count, $conversion_count, $interval);
  $form['overview_report']['report'] = array(
    '#markup' => drupal_render($overview_report),
    '#theme_wrappers' => array('container'),
    '#id' => 'acquia-lift-overview-report-data',
  );
  $variation_breakdown = _build_variation_breakdown($variation_counts);
  $form['variation_breakdown'] = array(
    '#markup' => drupal_render($variation_breakdown),
    '#theme_wrappers' => array('container'),
    '#id' => 'acquia-lift-variation-breakdown',
  );

  if (empty($option_set->targeting)) {
    return array();
  }
  // Display a report for the specified audience.
  if (isset($form_state['values']['audience_filter'])) {
    $audience = $form_state['values']['audience_filter'];
  }
  else {
    $audience = isset($_GET['targeting_audience']) ? 'active|' . $_GET['targeting_audience'] : NULL;
  }
  // Get audience options.
  $audience_options = array();
  foreach($option_set->targeting as $audience_name => $targeting) {
    $audience_options['active|' . $audience_name] = $targeting['label'];
  }
  // Get retired tests as well.
  $old_tests = acquia_lift_get_retired_tests($agent_data->machine_name);
  foreach($old_tests as $retired_test) {
    $label = $retired_test->label;
    // The label for the retired test includes the personalization name. Strip
    // it off as it is redundant here and makes the option name far too long.
    $prefix = $agent_data->label . ': ';
    if (strpos($label, $prefix) === 0) {
      $label = substr($label, strlen($prefix));
    }
    $retired_test_audience_option = 'retired|' . $retired_test->machine_name;
    $audience_options[$retired_test_audience_option] = $label;
    if (isset($_GET['lift_retired_test']) && $_GET['lift_retired_test'] == $retired_test->machine_name) {
      $audience = $retired_test_audience_option;
    }
  }
  if (empty($audience_options)) {
    return array(
      'no_report' => array(
        '#markup' => t('This personalization does not contain tests, no reports to show.')
      )
    );
  }

  // If no audience specified, or one that does not have a test, use the first
  // audience option.
  if (empty($audience)) {
    $audience = key($audience_options);
  }
  list($audience_status, $name) = explode('|', $audience);
  $audience_type = 'target';
  if ($audience_status == 'retired') {
    $audience_type = 'test';
    $nested_agent = personalize_agent_load($name);
    $test_label = $nested_agent->label;
    if ($nested_agent->data['lift_retired'] < $date_upgraded) {
      $use_old_stats = TRUE;
    }
  }
  else {
    if (isset($option_set->targeting[$name]['osid'])) {
      $audience_type = 'test';
    }
    $test_label = $option_set->targeting[$name]['label'];
  }

  $form['use_old_stats'] = array(
    '#type' => 'hidden',
    '#value' => $use_old_stats
  );
  // Show the audience filter.
  $form['audience_container'] = array(
    'audience_overview_report_title' => array(
      '#markup' => '<h2>' . t('Audience Overview') . '</h2>',
    ),
    '#type' => 'container',
    '#attributes' => array(
      'id' => 'audience-report-container',
      'class' => array('container-inline'),
    ),
    '#tree' => FALSE,
  );
  if (count($audience_options) == 1) {
    $form['audience_container']['audience_label'] = array(
      '#markup' => '<p>' . t('%audience audience', array('%audience' => $test_label)) . '</p>'
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

  $form['report'] = acquia_lift_report_audience($form_state, $agent_data, $option_set, $name, $lift_api, $start_time, $end_time, $use_old_stats, $audience_status == 'retired');
  if ($audience_status == 'active' && $audience_type == 'test' && personalize_agent_get_status($agent_data->machine_name) != PERSONALIZE_STATUS_COMPLETED) {
    $form['complete_audience'] = array(
      '#type' => 'link',
      '#title' => t('End test and choose winner'),
      '#href' => 'admin/structure/personalize/manage/' . $agent_data->machine_name . '/audience/' . $name . '/complete',
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
  return $form;
}

/**
 * Builds the audience-specific report.
 * @param $form_state
 * @param stdClass $personalization_data
 *   Object reprsenting the personalization
 * @param stdClass $targeting_option_set
 *   Object representing the targeting option set for hte personalization
 * @param string $audience
 *   The audience to pull a report for
 * @param \AcquiaLiftAPI $lift_api
 *   The lift API client wrapper.
 * @param $from
 *   Start date for the report
 * @param $to
 *   End date for the report
 * @param bool $use_old_stats
 *   Whether to use the legacy reports (i.e. direct from the decision engine)
 * @param bool $retired
 *   Whether this is a report for a retired test
 * @return mixed
 */
function acquia_lift_report_audience($form_state, $personalization_data, $targeting_option_set, $audience, AcquiaLiftAPI $lift_api, $from, $to, $use_old_stats = FALSE, $retired = FALSE) {
  $audience_type = isset($targeting_option_set->targeting[$audience]['osid']) ? 'test' : 'target';
  module_load_include('inc', 'acquia_lift', 'includes/AcquiaLiftLearnReport');
  $variations = acquia_lift_get_structure_from_targeting($targeting_option_set);
  if ($retired) {
    $audience_type = 'test';
    $tests = acquia_lift_get_retired_tests($personalization_data->machine_name);
    foreach ($tests as $test) {
      // If it's a retired test, then the $audience param is actually the machine
      // name of the test.
      if ($test->machine_name == $audience) {
        $audience = $test->data['lift_audience'];
        $test_os = personalize_option_set_load_by_agent($test->machine_name);
        $test_os = reset($test_os);
        $variations[$audience] = array_map(function($option) { return $option['option_id'];}, $test_os->options);
        $retired_test = $test; // We'll need this if using the old reporting.
        break;
      }
    }
  }
  // Conversion report filters.
  $selected_goal = empty($form_state['values']['goal']) ? NULL : $form_state['values']['goal'];
  $selected_metric = empty($form_state['values']['metric']) ? 'rate' : $form_state['values']['metric'];
  $session_count = $goal_count = $confidence = 0;
  $winner = NULL;
  $daily_data = $experiment_results = array();
  $nested_agent = NULL;
  if ($audience_type == 'test') {
    if (isset($retired_test)) {
      $nested_agent = $retired_test;
    }
    else {
      $nested_os = personalize_option_set_load($targeting_option_set->targeting[$audience]['osid']);
      $nested_agent = personalize_agent_load($nested_os->agent);
    }
  }
  if ($audience_type == 'test' && $use_old_stats) {
    // Get our test results the old way, i.e. pre-reporting-API
    $agent_instance = personalize_agent_load_agent($nested_agent->machine_name);
    if ($agent_instance instanceof AcquiaLiftLearn) {
      $options = array(
        'start_time' => $from,
        'end_time' => $to,
        'goal' => $selected_goal,
      );
      $learn_report = AcquiaLiftReportFactory::create($agent_instance, $lift_api, $options);
      $session_count = $learn_report->getSessionCount();
      $goal_count = $learn_report->getGoalCount();
      $daily_data = $learn_report->getDailyData();
      $experiment_results = $learn_report->getAggregatedData();
    }
  }
  else {
    // Pull the stats directly from the reporting API.
    try {
      $audience_report = $lift_api->getAudienceReport($personalization_data->machine_name, $audience, $variations[$audience], $audience_type, $from, $to);
      if (!empty($audience_report['overall_stats']) && !empty($audience_report['daily_stats'])) {
        foreach ($audience_report['overall_stats'] as $variation => $stats) {
          $session_count += $stats['decision_count'];
          $goal_count += $stats['conversion_count'];
        }
        $daily_data = _extract_daily_data($audience_report['daily_stats'], $personalization_data->machine_name);
        if (isset($audience_report['test_results'])) {
          $experiment_results = _extract_experiment_results($audience_report['test_results']);
          $confidence = !empty($audience_report['overall_confidence']) ? $audience_report['overall_confidence'] : 0;
          $winner = !empty($audience_report['winner']) ? $audience_report['winner'] : null;
        }
      }
      else {
        return array(
          '#markup' => t('No data available for this audience.')
        );
      }
    }
    catch (AcquiaLiftException $e) {
      drupal_set_message(t('There was a problem retrieving the reporting data from the API'), 'error');
    }
  }

  // Audience Overview report section.
  $form['audience_overview_report'] = array(
    '#theme_wrappers' => array('container'),
    '#attributes' => array(
      'id' => 'acquia-lift-audience-overview-report',
      'class' => array('acquia-lift-report-section', 'clearfix'),
    ),
  );

  $audience_overview_report = _build_overview_report($session_count, $goal_count);
  $form['audience_overview_report']['report'] = array(
    '#markup' => drupal_render($audience_overview_report),
    '#theme_wrappers' => array('container'),
    '#id' => 'acquia-lift-audience-overview-report-data',
  );

  $daily_report = _build_daily_report($daily_data, $personalization_data->machine_name, $audience);
  $form['daily_report'] = array(
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
          '#markup' => '<h2>' . t('Daily conversion report') . '</h2>',
        ),
      ),
    ),
    '#attributes' => array(
      'id' => 'acquia-lift-daily-report',
      'class' => array('lift-statistics'),
    ),
  );
  $form['daily_report']['header']['options'] = array(
    '#type' => 'container',
    '#attributes' => array(
      'class' => array('acquia-lift-report-section-options'),
    ),
    '#tree' => FALSE,

    'metric' => acquia_lift_report_experiment_metric_dropdown($selected_metric),
    'submit' => array(
      '#type' => 'submit',
      '#value' => t('Filter'),
    ),
  );
  // We only support per-goal reporting for the new reports.
  if (!$use_old_stats) {
    $form['daily_report']['header']['options']['goal'] = acquia_lift_report_goal_dropdown($personalization_data->machine_name, $selected_goal);
  }
  $form['daily_report']['report'] = array(
    '#markup' => drupal_render($daily_report),
    '#theme_wrappers' => array('container'),
    '#id' => 'acquia-lift-daily-report-data',
  );
  if ($audience_type == 'test') {
   $experiment_report = _build_experiment_report($experiment_results, $confidence, $winner, $personalization_data->machine_name, $audience, variable_get('acquia_lift_confidence_measure', 95));
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
    if ($nested_agent->data['decision_style'] == 'adaptive') {
      $percentage = $nested_agent->data['explore_rate'];
      $form['experiment_report']['header']['title']['groups'] = array(
        '#markup' => t('(@percentage%)', array('@percentage' => $percentage)),
      );
      // Also add help text to the heading to explain what this is.
      $form['experiment_report']['header']['title']['#attributes']['data-help-tooltip'] = t('The portion of visitors who were shown a random variation, as opposed to the optimized variation. The data below are for the experiment group only.');
    }

    $form['experiment_report']['header']['summary']['report_summary'] = array(
      '#theme_wrappers' => array('container'),
      '#markup' => t('See which content variations are winning.'),
      '#attributes' => array(
        'class' => array('acquia-lift-report-summary'),
      ),
    );
    $form['experiment_report']['report'] = array(
      '#markup' => drupal_render($experiment_report),
      '#theme_wrappers' => array('container'),
      '#id' => 'acquia-lift-experiment-report-data',
    );
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
 * Extracts daily stats from the reporting API into results
 * that can be displayed.
 */
function _extract_daily_data($daily_data, $personalization_name) {
  $daily_results = array();
  $i = 0;
  $counts = $goals = $vals = array();
  foreach ($daily_data as $variation_results) {
    $variation = $variation_results['variation_id'];
    foreach ($variation_results['results'] as $stats) {
      $day = $stats['timestamp'];
      $results = $stats['result'];
      if (!isset($counts[$variation])) {
        $counts[$variation] = $goals[$variation] =  $vals[$variation] = 0;
      }
      $counts[$variation] += $results['decision_count'];
      $goals[$variation] += $results['conversion_count'];

      $vals[$variation] += $results['conversion_value'];
      $rate = $counts[$variation] > 0 ? ($goals[$variation] / $counts[$variation]) : 0;
      $mean = $counts[$variation] > 0 ? ($vals[$variation] / $counts[$variation]) : 0;
      $goal_value = $goals[$variation] ? floor($vals[$variation] / $goals[$variation]) : 1;
      // Calculate confidence bounds for conversion rate.
      $sd = $counts[$variation] > 0 ? sqrt(($rate*(1-$rate))/$counts[$variation]) : 0;
      // We want a 90% confidence interval, which means we need the 95th
      // quantile, given the two tails of the distribution.
      module_load_include('inc', 'acquia_lift','includes/AcquiaLiftLearnReport');
      $quantile = AcquiaLiftLearnReport::$normal_quantiles[4];
      $upper = $goal_value * ($rate + ($quantile * $sd));
      $lower = $goal_value * ($rate - ($quantile * $sd));
      $daily_results[] = array(
        'option_id' => $variation,
        'option_label' => _get_variation_label($variation, $personalization_name),
        'goals' => $results['conversion_count'],
        'count' => $results['decision_count'],
        'date' => $day,
        'timestamp' => strtotime($day),
        'conversion' => _format_report_percentage($rate),
        'conversion_value' => _format_report_number($mean),
        'estimated_value' => _format_report_number($mean, TRUE, 4),
        'margin_error' => _format_report_number(($upper - $lower) / 2, TRUE, 4),
        'counter' => $i,
        'control' => $i === 0,
      );
    }
    $i++;
  }
  return $daily_results;
}

/**
 * Extracts aggregated stats from the reporting API into results
 * that can be displayed.
 */
function _extract_experiment_results($aggregated_data) {
  $aggregated_results = array();
  $i = 0;
  foreach ($aggregated_data as $result) {
    $variation = $result['variation_id'];
    $rate = $result['decision_count'] > 0 ? ($result['conversion_count'] / $result['decision_count']) : 0;
    $aggregated_results[$variation] = array(
      'counter' => $i,
      'option_id' => $variation,
      'option_label' => $variation,
      'goals' => $result['conversion_count'],
      'count' => $result['decision_count'],
      'conversion' => _format_report_percentage($rate),
      'estimated_value' => _format_report_number($result['mean'], TRUE, 4),
      'control' => $i === 0,
      'confidence' => _format_report_percentage($result['confidence']),
      'lift_default' =>  _format_report_number($result['lift_default']) . '%',
      'lift_random' => $result['lift_random'],
    );
    $i++;
  }
  return $aggregated_results;
}

/**
 * Builds a limited report for a test directly from the test stats.
 *
 * Used for tests that were running before data was being pushed to Lift Web.
 *
 * @param stdClass $agent_data
 *   The agent data for this test report.
 */
function _acquia_lift_get_subreport(&$form_state, $agent_data) {

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

  // Check for Rickshaw and D3 libraries and alert users if not exist.
  if (_acquia_lift_missing_library_warning(array('rickshaw', 'd3'), t('The following libraries are required in order to view the Acquia Lift reports:'))) {
    return array();
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
  $os = reset($option_sets);
  $decision_point = $os->decision_point;

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
  $form['audience_overview_report'] = array(
    'audience_overview_report_title' => array(
      '#markup' => '<h2>' . t('Overview') . '</h2>',
    ),
    '#theme_wrappers' => array('container'),
    '#attributes' => array(
      'id' => 'acquia-lift-overview-report',
      'class' => array('acquia-lift-report-section', 'clearfix'),
    ),
  );
  $form['audience_overview_report']['report'] = array(
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
  $form['experiment_report']['header']['options'] = array(
    '#type' => 'container',
    '#attributes' => array(
      'class' => array('acquia-lift-report-section-options'),
    ),
    '#tree' => FALSE,
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

  return $form;
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
function acquia_lift_get_report_dates_for_agent($agent_data, $as_date_strings = TRUE) {
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
  if (!$as_date_strings) {
    return array($start, $end);
  }
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
  $max_days = variable_get('acquia_lift_report_max_days', ACQUIA_LIFT_DEFAULT_MAX_DAYS);
  if ($num_days <= $max_days) {
    return $start;
  }
  // Move the start date forward so that we are within the max.
  $num_seconds = $max_days * 86400;
  return $end - $num_seconds;
}

/**
 * AJAX callback to return the daily conversion report.
 *
 * The following parameters are supplied within the query string:
 *   - personalization: the machine name of the personalization
 *   - audience: the audience to get data for
 *   - goal: (optional) a goal name to limit results (defaults to all goals)
 */
function acquia_lift_report_daily_conversions() {
  $params = drupal_get_query_parameters();
  $report = array(
    '#markup' => t('No report available for this personalization.'),
  );
  if (empty($params['personalization']) || empty($params['audience'])) {
    drupal_json_output(drupal_render($report));
    return;
  }
  $personalization_name = personalize_sanitize_string($params['personalization']);
  $audience = personalize_sanitize_string($params['audience']);
  $goal = empty($params['goal']) ? NULL : personalize_sanitize_string($params['goal']);

  $personalization = personalize_agent_load($personalization_name);

  if ($personalization->plugin  == "acquia_lift_target") {
    $account_info = acquia_lift_get_account_info();
    $api = AcquiaLiftAPI::getInstance($account_info);

    $targeting_option_set = acquia_lift_get_option_set_for_targeting($personalization_name, FALSE);
    $targeting_structure = acquia_lift_get_structure_from_targeting($targeting_option_set, FALSE);
    if (!isset($targeting_structure[$audience])) {
      drupal_json_output(drupal_render($report));
      return;
    }
    $audience_type = isset($targeting_option_set->targeting[$audience]['osid']) ? 'test' : 'target';
    list($start_time, $end_time) = acquia_lift_get_report_dates_for_agent($personalization, FALSE);

    try {
      $audience_report = $api->getAudienceReport($personalization_name, $audience, $targeting_structure[$audience], $audience_type, $start_time, $end_time, $goal);
      if (!empty($audience_report['daily_stats'])) {
        $daily_data = _extract_daily_data($audience_report['daily_stats'], $personalization_name);
        $daily_report = _build_daily_report($daily_data, $personalization_name, $audience);
        drupal_json_output('<div id="acquia-lift-daily-report-data">' . drupal_render($daily_report) . "</div>");
        return;
      }
    }
    catch (AcquiaLiftException $e) {

    }
  }

  drupal_json_output(drupal_render($report));
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
  if (empty($machine_name)) {
    if (is_numeric($label)) {
      $label = "Audience " . $label;
    }
    $machine_name = personalize_generate_machine_name($label, NULL, '-');
  }
  elseif (is_numeric($machine_name) && !isset($option_set->targeting[$machine_name])) {
    $machine_name = personalize_generate_machine_name("audience-" . $machine_name, NULL, '-');
  }

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

function acquia_lift_target_audience_copy($option_set, $audience_to_copy, $new_machine_name = NULL, $new_label = NULL, $weight = NULL) {
  // Create a function for checking unique qudience names within an option set.
  $all_audience_names = array_keys($option_set->targeting);
  $name_check_callback = function($name) use ($all_audience_names) {
    return in_array($name, $all_audience_names);
  };
  $label = empty($new_label) ? $option_set->targeting[$audience_to_copy]['label'] : $new_label;
  $machine_name = empty($new_machine_name) ? personalize_generate_machine_name($audience_to_copy, $name_check_callback, '-') : $new_machine_name;
  // Copy the necessary info from the existing audience.
  $option_set->targeting[$machine_name] = array(
    'label' => $label,
    'weight' => empty($weight) ? $option_set->targeting[$audience_to_copy]['weight'] : $weight,
    'targeting_features' => $option_set->targeting[$audience_to_copy]['targeting_features'],
    'targeting_rules' => $option_set->targeting[$audience_to_copy]['targeting_rules'],
    'targeting_strategy' => $option_set->targeting[$audience_to_copy]['targeting_strategy'],
  );
  personalize_option_set_save($option_set);
  return $machine_name;
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
  $existing_structure = acquia_lift_get_structure_from_targeting($targeting_option_set, FALSE);
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
  $rules = $new_tests = $new_options = $remove_audiences = array();
  foreach ($agent->data['lift_targeting'] as $new_audience => $variations) {
    $audience_name = $new_audience;
    if (count($variations) == 1) {
      if (isset($existing_structure[$audience_name]) && count($existing_structure[$audience_name]) > 1) {
        // This used to be a test - create a new audience so that the new
        // reporting data won't interfere with the reporting data for the test.
        $new_name = acquia_lift_target_audience_copy($targeting_option_set, $audience_name);
        // Reload the option set.
        $targeting_option_set = personalize_option_set_load($targeting_option_set->osid, TRUE);
        // Mark the old audience for removal
        $remove_audiences[] = $audience_name;
        $audience_name = $new_name;
      }
      $new_options[$audience_name] = reset($variations);
      continue;
    }
    if (isset($existing_structure[$audience_name])) {
      if ($existing_structure[$audience_name] != $variations) {
        // This audience is getting a different test from what it had before -
        // change its name so that the old test report will still make sense.
        $new_name = acquia_lift_target_audience_copy($targeting_option_set, $audience_name);
        // Reload the option set.
        $targeting_option_set = personalize_option_set_load($targeting_option_set->osid, TRUE);
        // Mark the original audience for removal.
        $remove_audiences[] = $audience_name;
        $audience_name = $new_name;
      }
      else {
        $rules[$audience_name] = $audience_name;
      }
    }

    // If we didn't find an existing test corresponding to this combination,
    // we'll need to create a new one.
    if (!isset($rules[$audience_name])) {
      $new_tests[$audience_name] = $variations;
    }
  }
  // Keep track of nested agents that get deleted.
  foreach (array_keys($existing_tests) as $audience) {
    if (!in_array($audience, $rules)) {
      // Delete the old test if it's no longer being used.
      acquia_lift_remove_nested_test($targeting_option_set, $audience, $agent);
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

  // Remove any audiences that are now defunct.
  foreach ($remove_audiences as $audience_name) {
    unset($targeting_option_set->targeting[$audience_name]);
  }
  // Refresh the reference to teh targeting option set, in case it got reloaed.
  $all_option_sets[$targeting_option_set->osid] = $targeting_option_set;
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
  $agent->plugin = ACQUIA_LIFT_TESTING_AGENT;
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
  $test->data['lift_retired'] = time();
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
function acquia_lift_get_structure_from_targeting($option_set, $include_empty_audiences = TRUE) {
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
    if (!$include_empty_audiences && empty($targeting_structure[$name])) {
      unset($targeting_structure[$name]);
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
    $nested_agent->data = array_merge($nested_agent->data, $agent_data);
    personalize_agent_save($nested_agent);
  }
}

/**
 * Validate that an agent has the minimum targeting required and add it if it
 * is not available.
 *
 * Targeting agents should have a default "Everyone else".
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
      watchdog('Acquia Lift', $e->getMessage());
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
  $mvt_agent->plugin = ACQUIA_LIFT_TESTING_AGENT;
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
    $test_agents = personalize_agent_load_by_type(ACQUIA_LIFT_TESTING_AGENT);
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

/**
 * =======================================================================
 *  R E P O R T  O U T P U T  F U N C T I O N S
 * =======================================================================
 */


/**
 * Returns a render array representing the overview report for the given dates.
 *
 * @param DateInterval $interval
 *   The length of the period this report represents, as a DateInterval object.
 * @param string $type
 *   THe type of personalization, e.g. 'A/B test'
 * @param int $session_count
 *   The total number of sessions.
 * @param int $goal_count
 *   The total number of goals received.
 * @return array
 *   A render array representing the overview report.
 */
function _build_overview_report($session_count, $goal_count, DateInterval $interval = null, $type = null) {
  // Create report renderable.
  $build = array();
  $is_audience_report = empty($interval);
  if (!empty($type)) {
    $build['test_type'] = array(
      '#type' => 'container',
      '#theme' => 'acquia_lift_report_overview',
      '#title' => $type,
      '#description' => t('test type'),
      '#attributes' => array(
        'id' => 'acquia-lift-overview-type',
      )
    );
  }
  if (!empty($interval)) {
    $build['total_running'] = array(
      '#type' => 'container',
      '#theme' => 'acquia_lift_report_overview',
      '#attributes' => array(
        'id' => 'acquia-lift-overview-running',
      ),
      '#title' => $interval->format('%d days, %h hours'),
      '#description' => t('total time running'),
    );
  }
  $formatted_session_count = _format_report_number($session_count, FALSE, 0);
  $build['shown'] = array(
    '#type' => 'container',
    '#theme' => 'acquia_lift_report_overview',
    '#attributes' => array(
      'id' => $is_audience_report ? 'acquia-lift-audience-overview-shown' : 'acquia-lift-overview-shown',
    ),
    '#title' => $formatted_session_count,
    '#description' => format_plural($session_count, 'time shown', 'times shown'),
  );
  $formatted_goal_count = _format_report_number($goal_count, FALSE, 0);
  $build['goals'] = array(
    '#type' => 'container',
    '#theme' => 'acquia_lift_report_overview',
    '#attributes' => array(
      'id' => $is_audience_report ? 'acquia-lift-audience-overview-goals' : 'acquia-lift-overview-goals',
    ),
    '#title' => $formatted_goal_count,
    '#description' => t('goals met'),
  );
  if ($goal_count > 0) {
    $build['goals']['#attributes']['class'] = array('acquia-lift-report-positive');
  }
  return $build;
}

/**
 * Builds the render array for the metrics portion of the report.
 *
 * @param array $daily_data
 *   The per-variation results broken down by day (shown in the graph)
 * @param array
 *   A render array for the report.
 */
function _build_daily_report($daily_data, $personalization_name, $audience_name) {
  $headers = array(
    t('Date'),
    t('Content variation'),
    array(
      'data' => t('Conversion rate (%)'),
      'data-conversion-metric' => 'rate',
    ),
    array(
      'data' => t('Conversion value'),
      'data-conversion-metric' => 'value',
    ),
    t('Margin of error'),
  );
  $build = $rows = array();
  foreach ($daily_data as $choice_data) {
    $rows[] = array(
      'data' => array(
        array(
          'data' => $choice_data['timestamp'],
        ),
        array(
          'data' => $choice_data['option_label'],
          'data-acquia-lift-variation-label' => _get_variation_label_abbreviated($choice_data['counter'], $choice_data['control']),
        ),
        array(
          'data' => $choice_data['conversion'],
        ),
        array(
          'data' => $choice_data['conversion_value'],
        ),
        array(
          'data' => $choice_data['margin_error'],
        ),
      ),
      'no_striping' => TRUE,
    );
  }
  if (!empty($rows)) {
    $build['metric_table'] = array(
      '#theme' => 'table',
      '#header' => $headers,
      '#rows' => $rows,
      '#sticky' => FALSE,
      '#attributes' => array(
        'data-lift-statistics' => '',
        'data-liftGraph-columnName' => '2',
        'data-liftGraph-columnX' => '1',
        'data-liftGraph-renderer' => 'line',
        'data-liftgraph-excluded' => '5',
        'data-acquia-lift-personalization' => $personalization_name,
        'data-acquia-lift-audience' => $audience_name,
      )
    );
  }
  return $build;
}

  /**
   * Builds the render array for the summary portion of the report.
   *
   * @param array $aggregated_data
   *   The per-variation results aggregated across all dates
   * @param bool $confidence
   *   Whether the report has confidence.
   * @param bool $winner
   *   The id of the winning option or NULL if there's no winner
   * @param array
   *   A render array for the report.
   */
function _build_experiment_report($aggregated_data, $confidence, $winner, $personalization_name, $audience_name, $confidence_measure) {
  $headers = array(
    t('Variation'),
    array(
      'data' => t('Times shown'),
      'data-help-tooltip' => t('Number of times this variation was shown to the experimental group.'),
    ),
    array(
      'data' => t('Goals met'),
      'data-help-tooltip' => t('Number of times visitors in the experiement group completed a goal after viewing the variation.'),
    ),
    array(
      'data' => t('Conversion rate'),
      'data-help-tooltip' => t('Percentage of goals met for each display of the variation.'),
    ),
    array(
      'data' => t('Chance to beat control'),
      'data-help-tooltip' => t('Confidence that this variation will perform better than the control.'),
    ),
    array(
      'data' => t('Lift'),
      'data-help-tooltip' => t('Estimated increase in conversions if this variation were to be shown all the time, versus the control.'),
    ),
    array(
      'data' => t('Winner'),
      'data-help-tooltip' => t('Most effective variation for visitors based on a @confidence% confidence level.', array(
        '@confidence' => $confidence_measure,
      )),
    ),
  );
  $confidence_message_shown = FALSE;
  $rows = array();
  $num_rows = count($aggregated_data);
  foreach ($aggregated_data as $choice_id => $choice_data) {
    $row_data = array(
      array(
        'data' => $choice_data['option_label'],
        'data-acquia-lift-variation-label' => _get_variation_label($choice_id, $personalization_name),
      ),
      array(
        'data' => $choice_data['count'],
      ),
      array(
        'data' => $choice_data['goals'],
      ),
      array(
        'data' => $choice_data['conversion'],
      ),
      array(
        'data' => $choice_data['confidence'],
      ),
      array(
        'data' => $choice_data['lift_default'],
      ),
    );
    // Add the winner column data.
    if (empty($rows) && !$confidence) {
      // If there is low confidence then show the message throughout the
      // winner column.
      $row_data[] = array(
        'data' => _get_low_confidence_message($confidence_measure),
        'rowspan' => $num_rows,
        'class' => array('acquia-lift-ab-winner'),
      );
      $confidence_message_shown = TRUE;
    }
    else if (!$confidence_message_shown) {
      // Show the winner indicator if this is the winning variation.
      $row_data[] = $confidence && ($winner === $choice_id) ? '<span class="lift-winner">' . t('Winner') . '</span>' : '';
    }
    $rows[] = array(
      'data' => $row_data,
      'no_striping' => TRUE,
    );

  }
  if (empty($rows)) {
    return array();
  }
  $build['summary_holder'] = array(
    '#type' => 'container',
    '#attributes' => array(
      'class' => array('lift-graph-result'),
    ),
  );
  $build['summary_holder']['summary_table'] = array(
    '#theme' => 'table',
    '#header' => $headers,
    '#rows' => $rows,
    '#sticky' => FALSE,
    '#attributes' => array(
      'class' => array('lift-graph-result-data'),
      'data-acquia-lift-personalization' => $personalization_name,
      'data-acquia-lift-audience' => $audience_name,
    ),
    '#attached' => array(
      'library' => array(
        array('acquia_lift', 'acquia_lift.help'),
      ),
    ),
  );
  return $build;
}


/**
 * Builds the render array for the breakdown per variation.
 */
function _build_variation_breakdown($variation_counts) {
  $headers = array(
    t('Variation'),
    array(
      'data' => t('Times shown'),
      'data-help-tooltip' => t('Number of times this variation was shown across all audiences.'),
    ),
    array(
      'data' => t('Goals met'),
      'data-help-tooltip' => t('Number of times visitors across all audiences completed a goal after viewing the variation.'),
    ),
  );
  $rows = array();
  foreach ($variation_counts as $choice_id => $info) {
    $row_data = array(
      array(
        'data' => $info['option_label'],
      ),
      array(
        'data' => $info['decision_count'],
      ),
      array(
        'data' => $info['goal_count'],
      ),
    );

    $rows[] = array(
      'data' => $row_data,
      'no_striping' => TRUE,
    );

  }
  if (empty($rows)) {
    return array();
  }
  $build['summary_holder'] = array(
    '#type' => 'container',
    '#attributes' => array(
      'class' => array('lift-variation-breakdown'),
    ),
  );
  $build['summary_holder']['summary_table'] = array(
    '#theme' => 'table',
    '#header' => $headers,
    '#rows' => $rows,
    '#sticky' => FALSE,
  );

  return $build;
}

/**
 * Generates a message to show when there is insufficient confidence in the
 * test results.
 *
 * @return string
 */
function _get_low_confidence_message($confidence_measure) {
  return t('There is not enough data to declare a winner with @confidence% confidence. Consider letting the test run longer before using the results.', array(
    '@confidence' => $confidence_measure,
  ));
}

  /**
   * Generates the variation abbreviated label.
   *
   * @param $counter
   *   Indicates the number for the variation.
   * @param $is_control
   *   True if this is the control option.
   */
function _get_variation_label_abbreviated($counter, $is_control) {
  if ($is_control) {
    return t('Control');
  }
  else {
    if (!$counter) {
      $counter = 1;
    }
    return t('V@num', array('@num' => $counter));
  }
}
function _get_variation_label($variation_id, $personalization_name) {
  $os = acquia_lift_get_option_set_for_targeting($personalization_name, FALSE);
  foreach ($os->options as $option) {
    if ($option['option_id'] == $variation_id) {
      return $option['option_label'];
    }
  }
  return $variation_id;
}

/**
 * Formats a percentage value for use in reports.
 *
 * @param $value
 *   The number to show as a percentage.
 * @param bool $include_sign
 *   True to include positive/negative sign indicators.
 * @param $trim
 *   Boolean indicating whether the number should be trimmed of trailing 0s.
 * @param $decimals
 *   The number of decimal places to display.
 * @param $padding
 *   The total number of characters (including decimal) for padding of the
 *   final number.  This allows numbers to align properly  in column views.
 *   This will have no effect if trim is set to true.
 * @return string
 *   The formatted number to display.
 */
function _format_report_percentage($value, $include_sign = FALSE, $trim = TRUE, $decimals = 2, $padding = 1) {
  $percent = (float) $value * 100;
  if ($percent > 0 && $include_sign) {
    return '+' . _format_report_number($percent, $trim, $decimals, $padding) . '%';
  }
  return _format_report_number($percent, $trim, $decimals, $padding) . '%';
}

  /**
   * Formats a number value for use in reports.
   *
   * @param $value
   *   The number of format (or an empty value).
   * @param $trim
   *   Boolean indicating whether the number should be trimmed of trailing 0s.
   * @param $decimals
   *   The number of decimal places to display.
   * @param $padding
   *   The total number of characters to pad to the left of the decimal point.
   * @return string
   *   The formatted number to display.
   */
function _format_report_number($value, $trim = TRUE, $decimals = 2, $padding = 1) {
  if (is_numeric($value)) {
    $value = number_format($value, $decimals);
    if ($trim && strpos('.', $value) !== FALSE) {
      $value = rtrim(rtrim($value, '0'), '.');
    }
    if ($padding > 0) {
      $value = str_pad($value, $padding, '0', STR_PAD_LEFT);
    }
  }
  if (empty($value)) {
    $value = 0;

  }
  return $value;
}