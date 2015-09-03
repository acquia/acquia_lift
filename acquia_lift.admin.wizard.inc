<?php
/**
 * @file acquia_lift.admin.wizard.inc Functions specific to the Acquia Lift
 * alteration of the campaign creation wizard.
 */

/**
 * Basic handler for altering any part of the campaign wizard form.
 *
 * @param array $form
 *   The existing campaign form structure to alter.
 * @param array $form_state
 *   The form state.
 * @param stdClass $agent_data
 *   The agent that is being altered (optional if creating).  This will
 *   always be a targeting agent.
 */
function acquia_lift_personalize_campaign_wizard_alter(&$form, &$form_state, $agent_data = NULL) {
  $form['#attached']['library'][] = array('acquia_lift', 'acquia_lift.targeting_admin');
  $form['#attached']['library'][] = array('acquia_lift', 'acquia_lift.help');

  // We have to specify the include file so as not to lose it during rendering from ajax.
  // @see personalize_agent_form_ajax_submit()
  // @see drupal_retrieve_form():734
  $form_state['build_info']['files'][] = drupal_get_path('module', 'acquia_lift') . '/acquia_lift.admin.wizard.inc';

  module_load_include('inc', 'acquia_lift', 'acquia_lift.admin');
  $editable = acquia_lift_target_definition_changes_allowed($agent_data);


  // Call the alter hooks for the generic form parts.
  call_user_func_array('acquia_lift_personalize_campaign_wizard_process_bar_alter', array(&$form, &$form_state, $agent_data, $editable));
  call_user_func_array('acquia_lift_personalize_campaign_wizard_base_alter', array(&$form, &$form_state, $agent_data, $editable));

  // Add in any section help if available.
  $function = 'acquia_lift_personalize_campaign_wizard_' . $form_state['storage']['step'] . '_help';
  if (function_exists($function)) {
    $form['section_help'] = call_user_func_array($function, array(&$form, &$form_state, $agent_data, $editable));
    $form['header']['#weight'] = -40;
    $form['agent_basic_info']['#weight'] = -30;
    $form['process_bar']['#weight'] = -20;
    $form['section_help']['#weight'] = -10;
  }

  // Now call any alter hooks that are specific to the storage step.
  $function = 'acquia_lift_personalize_campaign_wizard_' . $form_state['storage']['step'] . '_alter';
  if (function_exists($function)) {
    call_user_func_array($function, array(&$form, &$form_state, $agent_data, $editable));
  }

  // Update primary submit button.
  if (!empty($form['actions']['submit'])) {
    $form['actions']['submit']['#attributes']['class'][] = 'acquia-lift-submit-button';
  }
}

/**
 ********************************************************************
 *
 * S U B F O R M S
 *
 ********************************************************************
 */

/**
 * Alter hook for the process bar on the campaign wizard form.
 */
function acquia_lift_personalize_campaign_wizard_process_bar_alter(&$form, &$form_state, $agent_data, $editable) {
  // For each status form, handle the status change along with any
  // nested agent implementations.
  foreach (element_children($form['process_bar']['actions']['status']['status_wrapper']) as $form_element) {
    $form['process_bar']['actions']['status']['status_wrapper'][$form_element]['#submit'] = array('acquia_lift_personalize_campaign_wizard_status_submit');
  }

  if (empty($agent_data->started)) {
    $form['process_bar']['navigation']['#options']['results']['disabled'] = TRUE;
  }
}

/**
 * Alter hook for the base campaign wizard form.
 */
function acquia_lift_personalize_campaign_wizard_base_alter(&$form, &$form_state, $agent_data, $editable) {
  module_load_include('inc', 'acquia_lift', 'acquia_lift.admin');
  if ($editable) {
    // Make the campaign name a self-revealing text input.
    $form['agent_basic_info']['title']['#theme_wrappers'][] = 'acquia_lift_revealing_input';
  }
  else {
    $form['agent_basic_info']['title']['#disabled'] = TRUE;
  }
}

/**
 * Alter hook for the variations portions of the campaign wizard.
 */
function acquia_lift_personalize_campaign_wizard_variations_alter(&$form, &$form_state, $agent_data, $editable) {
  global $base_url;

  // Rebuild the variations form to show the customized Lift approach.
  unset($form['variations']['title']['summary']);
  unset($form['variations']['option_sets']);
  $form['variations']['#tree'] = TRUE;

  // Take over the form validation and submit handling.
  $form['#validate'] = array('acquia_lift_personalize_campaign_wizard_validate');
  $form['#submit'] = array('acquia_lift_personalize_campaign_wizard_submit');

  if (empty($agent_data->machine_name)) {
    return;
  }

  if (!$editable) {
    $form['variations']['#disabled'] = TRUE;
  }

  $variation_set_handling = ACQUIA_LIFT_DECISION_LOCKSTEP;
  /**
   * Temporarily setting all variation set handling to lock step.
   * See https://www.drupal.org/node/2505247
  $variation_set_handling = '';
  if (!empty($form_state['values']['variations']['add_variation']['variation_set_handling'])) {
    $variation_set_handling = $form_state['values']['variations']['add_variation']['variation_set_handling'];
    // If the variation set was just determined, save it so that it can persist
    // across other AJAX actions.
    $form_state['storage']['acquia_lift_variation_set_handling'] = $form_state['values']['variations']['add_variation']['variation_set_handling'];
  }
  else if (isset($agent_data->data['variation_set_handling'])) {
    $variation_set_handling = $agent_data->data['variation_set_handling'];
  }*/
  // Set the option set type into storage to allow persistence across
  // multiple form call-backs.

  if (isset($form_state['triggering_element']) && end($form_state['triggering_element']['#parents']) == 'option_set_type') {
    $delta = _acquia_lift_personalize_campaign_wizard_next_element($form_state, 'new');
    $form_state['new_option_sets'][$delta] = $form_state['values']['variations']['editing']['new'][$delta]['option_set_type'];
    $form_state['expanded_option_set'] = $delta;
  }
  else if (isset($_GET['create']) && in_array($_GET['create'], array('block', 'element'))) {
    // Has to be the first new option set because the base is just loading.
    $form_state['new_option_sets'][0] = $_GET['create'];
    $form_state['expanded_option_set'] = 0;
  }

  $option_sets = personalize_option_set_load_by_agent($agent_data->machine_name);
  if ($variation_set_handling == ACQUIA_LIFT_DECISION_MULTIVARIATE) {
    // If this is an MVT then the option sets get saved directly on the testing
    // agent.
    $mvt_name = acquia_lift_get_mvt_name_for_agent($agent_data->machine_name);
    if ($mvt = personalize_agent_load($mvt_name)) {
      $option_sets = personalize_option_set_load_by_agent($mvt_name);
    }
  }

  // If this campaign already handles multiple variation sets then indicate
  // the type of handling in the section title.
  if (count($option_sets) > 1 && !empty($variation_set_handling)) {
    $section_title = $form['variations']['title']['#title'];
    if ($variation_set_handling == ACQUIA_LIFT_DECISION_LOCKSTEP) {
      $section_title = '<span data-help-tooltip="' . t('Each variation set should have the same number of variations so they can be evenly locked as combinations. For example: AA, BB, CC.') . '">' . $section_title . ' (' . t('Lock step') . ')' . '</span>';
    }
    else {
      $section_title .= ' (' . t('Multi-variate') . ')';
    }
    $form['variations']['title']['#title'] = $section_title;
  }

  // Keep track of all variations for display.
  $all_variations = array();
  $form['variations']['editing'] = array(
    '#type' => 'container',
    '#attributes' => array(
      'class' => array('personalize-wizard-column'),
    ),
  );

  // Make a container to hold existing option sets.
  if (!empty($option_sets)) {
    $form['variations']['editing']['option_sets'] = array(
      '#type' => 'container',
      '#attributes' => array(
        'class' => array('personalize-wizard-variation-sets'),
      ),
    );
  }
  // Add an option set edit card for each option set.
  foreach ($option_sets as $option_set) {
    $option_set_plugin = personalize_get_option_set_type($option_set->plugin);
    $form['variations']['editing']['option_sets']['option_set_' . $option_set->osid] = array(
      '#tree' => TRUE,
      '#type' => 'container',
      '#theme' => 'acquia_lift_card',
      '#collapsible' => TRUE,
      '#collapsed' => count($option_sets) > 1,
      '#title' => $option_set->label,
      '#attributes' => array(
        'class' => array('personalize-option-set'),
      ),
    );
    $form['variations']['editing']['option_sets']['option_set_' . $option_set->osid]['option_set'] = array(
      '#type' => 'value',
      '#value' => $option_set,
    );
    $form['variations']['editing']['option_sets']['option_set_' . $option_set->osid]['header'] = array(
      '#type' => 'container',
    );
    // Variation sets can be deleted only if the structure has not already been
    // implemented.
    // @see acquia_lift_implement_test_structure().
    if ($editable) {
      $delete_link = module_hook($option_set_plugin['module'], 'personalize_delete_link') ? module_invoke($option_set_plugin['module'], 'personalize_delete_link', $option_set) : '';
      if (!empty($delete_link)) {
        $form['variations']['editing']['option_sets']['option_set_' . $option_set->osid]['header']['delete'] = array(
          '#markup' => l(t('Delete'), $delete_link, array(
            'attributes' => array(
              'class' => array('acquia-lift-delete')
            ),
            'query' => array(
              'destination' => current_path(),
            )
          )),
        );
      }
    }
    switch ($option_set->plugin) {
      case 'block':
        $form['variations']['editing']['option_sets']['option_set_' . $option_set->osid]['content'] = _acquia_lift_personalize_campaign_wizard_variations_block($form, $form_state, $agent_data, array('variations', 'editing', 'option_sets', 'option_set_' . $option_set->osid, 'content'), $option_set);
        break;
      case 'elements':
        $form['variations']['editing']['option_sets']['option_set_' . $option_set->osid]['content'] = _acquia_lift_personalize_campaign_wizard_variations_element($form, $form_state, $agent_data, $option_set);
        break;
      case 'fields':
        $form['variations']['editing']['option_sets']['option_set_' . $option_set->osid]['content'] = _acquia_lift_personalize_campaign_wizard_variations_fields($form, $form_state, $agent_data, $option_set);
        break;
      // Unsupported display - get an edit link if provided by the plugin.
      default:
        $form['variations']['editing']['option_sets']['option_set_' . $option_set->osid]['content'] = _acquia_lift_personalize_campaign_wizard_variations_general_edit($agent_data, $option_set);
        $edit_link = module_hook($option_set_plugin['module'], 'personalize_edit_link') ? module_invoke($option_set_plugin['module'], 'personalize_edit_link', $option_set) : '';
        if (!empty($edit_link)) {
          $markup = l(t('Edit this variation set'), $edit_link);
        }
        else {
          $markup = t('Variation set type ' . $option_set->plugin . ' does not provide a link for editing.');
        }
        $form['variations']['editing']['option_sets']['option_set_' . $option_set->osid]['content']['edit'] = array(
          '#markup' => $markup,
        );
    }
    // Add the advanced settings form.
    $form['variations']['editing']['option_sets']['option_set_' . $option_set->osid]['advanced'] = personalize_campaign_wizard_variations_advanced($form, $form_state, $option_set);
    // Option set label is edited in a different spot so don't show it twice.
    $form['variations']['editing']['option_sets']['option_set_' . $option_set->osid]['advanced']['label']['#access'] = FALSE;
    // Decision name is handled based on the type of tests created.
    $form['variations']['editing']['option_sets']['option_set_' . $option_set->osid]['advanced']['decision_name']['#access'] = FALSE;

    // Add the pages options to the settings form for element variations.
    if ($option_set->plugin == 'elements') {
      $form['variations']['editing']['option_sets']['option_set_' . $option_set->osid]['advanced']['pages_all'] = array(
        '#type' => 'radios',
        '#title' => t('Execute on'),
        '#options' => array(
          '0' => t('Selected pages'),
          '1' => t('All pages'),
        ),
        '#default_value' => isset($option_set->data['pages']) && empty($option_set->data['pages']) ? 1 : 0,
      );

      $form['variations']['editing']['option_sets']['option_set_' . $option_set->osid]['advanced']['pages'] = array(
        '#type' => 'textarea',
        '#title' => t('Pages'),
        '#title_display' => 'invisible',
        '#field_prefix' => $base_url . base_path(),
        '#default_value' => isset($option_set->data['pages']) ? $option_set->data['pages'] : '',
        '#description' => t("Specify pages by using specifying one path per line. The '*' character is a wildcard. Example paths are %blog for the blog page and %blog-wildcard for every personal blog. %front is the front page.", array('%blog' => 'blog', '%blog-wildcard' => 'blog/*', '%front' => '<front>')),
        '#states' => array(
          'disabled' => array(
            ':input[name="variations[editing][option_sets][option_set_' . $option_set->osid . '][advanced][pages_all]"]' => array('value' => '1'),
          ),
        ),
        // Specify the requirements for personalize_form_element_path_validate.
        '#allow_dynamic' => TRUE,
        '#allow_external' => FALSE,
      );
    }

    // Show save button specific to this option set.
    $form['variations']['editing']['option_sets']['option_set_' . $option_set->osid]['save'] = array(
      '#name' => $option_set->plugin . '_existing_' . $option_set->osid,
      '#type' => 'submit',
      '#value' => t('Save'),
      '#submit' => array('acquia_lift_personalize_campaign_wizard_variations_single_submit'),
      '#limit_validation_errors' => array(array('variations', 'editing', 'option_sets', 'option_set_' . $option_set->osid)),
    );
  }
  // Hide the agent advanced options which will be shown on the review screen.
  $form['variations']['advanced']['#access'] = FALSE;

  $new_option_sets_id = 'acquia-lift-option-sets-new';
  $form['variations']['editing']['new'] = array(
    '#type' => 'container',
    '#attributes' => array(
      'id' => $new_option_sets_id,
    ),
    '#access' => $editable,
  );

  if (!empty($form_state['new_option_sets'])) {
    // Get the option set type UI display details.
    module_load_include('inc', 'acquia_lift', 'acquia_lift.ui');
    $option_set_types = acquia_lift_option_set_types_ui();
    $option_set_options = array();
    foreach ($option_set_types as $type => $details) {
      $option_set_options[$type] = $details['title'];
    }

    foreach ($form_state['new_option_sets'] as $delta => $new_option_set_type) {
      $new_option_set_details_id = 'acquia-lift-new-option-set-' . $delta;
      $new_option_set_type = $form_state['new_option_sets'][$delta] ?: '';
      $expanded = (isset($form_state['expanded_option_set']) && $form_state['expanded_option_set'] == $delta) || count($form_state['new_option_sets']) == 1;
      $form['variations']['editing']['new'][$delta] = array(
        '#type' => 'container',
        '#title' => t('New variation set #@num', array('@num' => ($delta + 1))),
        '#theme' => 'acquia_lift_card',
        '#collapsed' => !$expanded,
        '#access' => $editable,
        '#attributes' => array(
          'id' => $new_option_set_details_id,
        ),
      );
      if (empty($new_option_set_type)) {
        // Show option set type selection.
        $form['variations']['editing']['new'][$delta]['option_set_type'] = array(
          '#type' => 'radios',
          '#options' => $option_set_options,
          '#theme' => 'acquia_lift_radio_list',
          '#ajax' => array(
            'wrapper' => $new_option_set_details_id,
            'callback' => 'acquia_lift_personalize_campaign_wizard_variations_ajax_delta',
          ),
          '#attributes' => array(
            'autocomplete' => 'off',
          ),
        );
        foreach ($option_set_types as $type => $details) {
          $form['variations']['editing']['new'][$delta]['option_set_type'][$type]['#description'] = $details['description'];
          $form['variations']['editing']['new'][$delta]['option_set_type'][$type]['#image'] = $details['logo'];
        }
        $form['variations']['editing']['new'][$delta]['cancel'] = array(
          '#type' => 'submit',
          '#name' => 'cancel_option_set_' . $delta,
          '#value' => t('Cancel'),
          '#submit' => array('acquia_lift_personalize_campaign_wizard_variations_ajax_cancel'),
          '#limit_validation_errors' => array(),
          '#ajax' => array(
            'callback' => 'acquia_lift_personalize_campaign_wizard_variations_ajax',
            'wrapper' => $new_option_sets_id,
          ),
        );
      }
      else {
        // Display a option set type specific form for the new option set.
        $selected_option_set_type = $option_set_types[$new_option_set_type]['title'];
        $change_type_options = array(
          '#submit' => array('acquia_lift_personalize_campaign_wizard_variations_ajax_change'),
          '#ajax' => array(
            'callback' => 'acquia_lift_personalize_campaign_wizard_variations_ajax_delta',
            'wrapper' => $new_option_set_details_id,
          ),
        );
        switch ($new_option_set_type) {
          case 'block':
            $form['variations']['editing']['new'][$delta] += _acquia_lift_personalize_campaign_wizard_change_type_form($selected_option_set_type, $change_type_options);
            $form['variations']['editing']['new'][$delta]['block'] = array(
              '#type' => 'container',
              '#attributes' => array(
                'class' => array('acquia-lift-block-variation-set'),
              ),
            );
            $form['variations']['editing']['new'][$delta]['block']['content'] = _acquia_lift_personalize_campaign_wizard_variations_block($form, $form_state, $agent_data, array('variations', 'editing', 'new', $delta, 'block', 'content'));
            // TRICKY: Name must be specified or else it will default to "op" and
            // conflict with the main process bar save button.
            $form['variations']['editing']['new'][$delta]['block']['save'] = array(
              '#name' => 'block_new_' . $delta,
              '#type' => 'submit',
              '#value' => t('Save'),
              '#submit' => array('acquia_lift_personalize_campaign_wizard_variations_single_submit'),
              '#limit_validation_errors' => array(array('variations', 'editing', 'new', $delta)),
            );
            $form['variations']['editing']['new'][$delta]['block']['cancel'] = array(
              '#type' => 'submit',
              '#name' => 'cancel_block_option_set_' . $delta,
              '#value' => t('Cancel'),
              '#submit' => array('acquia_lift_personalize_campaign_wizard_variations_ajax_cancel'),
              '#limit_validation_errors' => array(),
              '#ajax' => array(
                'callback' => 'acquia_lift_personalize_campaign_wizard_variations_ajax',
                'wrapper' => $new_option_sets_id,
              ),
            );
            break;

          case 'element':
            $form['variations']['editing']['new'][$delta] += _acquia_lift_personalize_campaign_wizard_change_type_form($selected_option_set_type, $change_type_options);
            $form['variations']['editing']['new'][$delta]['element'] = array(
              '#type' => 'container',
              '#attributes' => array(
                'class' => array('acquia-lift-element-variation-set'),
              ),
            );
            $form['variations']['editing']['new'][$delta]['element']['content'] = _acquia_lift_personalize_campaign_wizard_variations_element($form, $form_state, $agent_data);
            $form['variations']['editing']['new'][$delta]['element']['cancel'] = array(
              '#type' => 'submit',
              '#name' => 'cancel_element_option_set_' . $delta,
              '#value' => t('Cancel'),
              '#submit' => array('acquia_lift_personalize_campaign_wizard_variations_ajax_cancel'),
              '#limit_validation_errors' => array(),
              '#ajax' => array(
                'callback' => 'acquia_lift_personalize_campaign_wizard_variations_ajax',
                'wrapper' => $new_option_sets_id,
              ),
            );
            break;
        }
      }
    }
  }
  // The button to add a new option set.
  $form['variations']['editing']['new']['add'] = array(
    '#type' => 'submit',
    '#value' => t('Add variation set'),
    '#theme_wrappers' => array('acquia_lift_add_card_button'),
    '#access' => $editable,
    '#submit' => array('acquia_lift_personalize_campaign_wizard_variations_ajax_add'),
    '#limit_validation_errors' => array(),
    '#ajax' => array(
      'callback' => 'acquia_lift_personalize_campaign_wizard_variations_ajax',
      'wrapper' => $new_option_sets_id,
    ),
  );

  // The rest of the display is for displaying current variations.
  if (count($option_sets) == 0) {
    return;
  }

  // Display all the current variation options.
  $form['variations']['display'] = array(
    '#type' => 'container',
    '#attributes' => array(
      'class' => array('personalize-wizard-column'),
    ),
  );

  // If there are any errors in the multiple variation set handling for this
  // agent then show them here while the user can do something about it.
  if (count($option_sets) > 1) {
    if (!acquia_lift_target_validate_lock_step($option_sets)) {
      $form['variations']['display']['errors'] = array(
        '#type' => 'container',
        '#theme' => 'acquia_lift_card',
        '#title' => t('Let\'s fix this'),
        '#collapsible' => FALSE,
      );
      $form['variations']['display']['errors']['details'] = array(
        '#markup' => theme('item_list', array(
          'items' => array(
            t('Variation sets must contain an equal number of variations.'),
          ),
          'attributes' => array(
            'class' => array('personalize-wizard-review-warnings'),
          ),
        )),
      );
    }
  }
  $form['variations']['display']['options'] = array(
    '#type' => 'container',
    '#theme' => 'acquia_lift_card',
    '#title' => t('All variations'),
    '#collapsible' => FALSE,
    '#attributes' => array(
      'class' => array('acquia-lift-informational'),
    ),
  );
  $all_variations = _acquia_lift_personalize_campaign_wizard_variation_displays($option_sets, $variation_set_handling);
  $form['variations']['display']['options']['variations'] = array(
    '#markup' => theme('acquia_lift_variations_list', array(
      'items' => $all_variations,
    )),
  );
}

/**
 * Alter the goals form.
 */
function acquia_lift_personalize_campaign_wizard_goals_alter(&$form, &$form_state, $agent_data, $editable) {
  unset($form['goals']['title']['title_summary']);
  $form['goals']['#tree'] = TRUE;

  // Take over form validation and submit handling.
  $form['#validate'] = array('acquia_lift_personalize_campaign_wizard_validate');
  $form['#submit'] = array('acquia_lift_personalize_campaign_wizard_submit');

  if (!$editable) {
    $form['goals']['#disabled'] = TRUE;
  }

  // Alter the existing goals list form items.
  $goal_deltas = element_children($form['goals']['all_goals']);
  if (count($goal_deltas) === 1) {
    $only_goal_delta = current($goal_deltas);
    if (empty($form['goals']['all_goals'][$only_goal_delta]['goal_id']['#value'])) {
      // The only goal form element is a placeholder for a new goal.
      unset($form['goals']['all_goals'][$only_goal_delta]);
      $goal_deltas = array();
    }
  }

  $params = drupal_get_query_parameters();
  $all_actions = visitor_actions_get_actions();
  foreach ($goal_deltas as $delta) {
    $goal_id = $form['goals']['all_goals'][$delta]['goal_id']['#value'];
    $expand = FALSE;

    if ($editable) {
      // Add delete button to the header.
      // We cannot make this an AJAX callback because the form values cannot
      // be carried forward with an updated form build due to the storing of
      // goal values by array index order.
      $form['goals']['all_goals'][$delta]['header'] = array(
        '#type' => 'container',
        'delete' => array(
          '#name' => 'delete_goal_' . $delta,
          '#type' => 'submit',
          '#value' => t('Delete'),
          '#attributes' => array(
            'data-acquia-lift-personalize-goal-id' => $goal_id,
            'title' => t('Delete the goal.'),
            'class' => array('acquia-lift-delete'),
          ),
          '#submit' => array('acquia_lift_personalize_campaign_wizard_goals_submit_delete'),
          '#limit_validation_errors' => array(),
        ),
      );

      // Determine the current action selected.
      $edit_action = $form['goals']['all_goals'][$delta]['action_name']['#default_value'];
      if (isset($form_state['triggering_element']) && end($form_state['triggering_element']['#array_parents']) == 'action_name') {
        $trigger_delta = _acquia_lift_personalize_campaign_wizard_next_element($form_state, 'all_goals');
        if ($trigger_delta === $delta) {
          $edit_action = $form_state['triggering_element']['#value'];
          $expand = TRUE;
        }
      }
      $visitor_action = $all_actions[$edit_action];

      // If this was an automatically created goal then add an explanation.
      if (!empty($visitor_action['data']['auto_created'])) {
        $form['goals']['all_goals'][$delta]['#title_attributes']['data-help-tooltip'] = t('This goal was automatically created for you. Use it if you want to track any click within any variation in this variation set. If you do not want to track clicks in this variation set, feel free to delete this goal.');
      }

      if (!empty($visitor_action['plugin'])) {
        // Show a link to edit the associated action within the label.
        $edit_action_label = t('Visitor action') . ' ' . l(
            t('Edit'),
            'admin/structure/acquia_lift/visitor_action/' . $edit_action,
            array(
              'attributes' => array(
                'class' => array(
                  'ctools-use-modal',
                  'ctools-modal-acquia-lift-style',
                  'acquia-lift-configure',
                ),
                'id' => 'edit-visitor-action-' . $edit_action,
              ),
            )
          );
        $form['goals']['all_goals'][$delta]['action_name']['#title'] = $edit_action_label;
      }
      else {
        // Not a custom action and therefore is not editable.
        $form['goals']['all_goals'][$delta]['action_name']['#title'] = t('Visitor action');
      }
    }
    // Make sure the action label/link gets updated when the action
    // selection changes.
    $card_id = 'goals-all-goals-' . $delta;
    $form['goals']['all_goals'][$delta]['#attributes']['id'] = $card_id;
    $form['goals']['all_goals'][$delta]['action_name']['#ajax'] = array(
      'callback' => 'acquia_lift_personalize_campaign_wizard_goals_ajax_existing',
      'wrapper' => $card_id,
    );

    // Convert the goals display into a card.
    $expand = $expand || count(element_children($form['goals']['all_goals'])) === 1 || (isset($params['goal']) && $params['goal'] == $goal_id);
    $form['goals']['all_goals'][$delta]['#type'] = 'container';
    $form['goals']['all_goals'][$delta]['#theme'] = 'acquia_lift_card';
    $form['goals']['all_goals'][$delta]['#collapsible'] = TRUE;
    $form['goals']['all_goals'][$delta]['#collapsed'] = !$expand;
    unset($form['goals']['all_goals'][$delta]['remove']);
  }

  // If the triggering element is the goal type selection, then set it to
  // the form_state for the selected goal delta.
  if (isset($form_state['triggering_element']) && end($form_state['triggering_element']['#parents']) == 'goal_type') {
    $delta = _acquia_lift_personalize_campaign_wizard_next_element($form_state, 'new');
    $form_state['new_goals'][$delta] = $form_state['values']['goals']['new'][$delta]['goal_type'];
    $form_state['expanded_goal'] = $delta;
  }

  // Show the card to add a new goal last.
  $new_goals_id = 'acquia-lift-goals-new';
  $form['goals']['new'] = array(
    '#type' => 'container',
    '#attributes' => array(
      'id' => $new_goals_id,
    ),
  );
  if (!empty($form_state['new_goals'])) {
    // Get the goal type UI display details.
    module_load_include('inc', 'acquia_lift', 'acquia_lift.ui');
    $goal_types = acquia_lift_goal_types_ui();
    $goal_type_options = array();
    foreach ($goal_types as $type => $details) {
      $goal_type_options[$type] = $details['title'];
    };

    foreach($form_state['new_goals'] as $delta => $new_goal_details) {
      $new_goal_type = $form_state['new_goals'][$delta] ?: '';

      // Generate the ID to use when replacing the individual new goal form.
      $new_goal_details_id = 'acquia-lift-new-' . $delta;
      $expanded = (isset($form_state['expanded_goal']) && $form_state['expanded_goal'] == $delta) || count($form_state['new_goals']) == 1;

      $form['goals']['new'][$delta] = array(
        '#type' => 'container',
        '#theme' => 'acquia_lift_card',
        '#collapsed' => !$expanded,
        '#title' => t('New goal #@num', array('@num' => ($delta + 1))),
        '#collapsible' => TRUE,
        '#access' => $editable,
        '#attributes' => array(
          'id' => $new_goal_details_id,
        ),
      );
      if (empty($new_goal_type)) {
        $form['goals']['new'][$delta]['goal_type'] = array(
          '#type' => 'radios',
          '#options' => $goal_type_options,
          '#theme' => 'acquia_lift_radio_list',
          '#ajax' => array(
            'callback' => 'acquia_lift_personalize_campaign_wizard_goals_ajax_delta',
            'wrapper' => $new_goal_details_id,
            'progress' => array(
              'type' => 'throbber',
              'message' => '',
            ),
          ),
          '#attributes' => array(
            'autocomplete' => 'off',
          ),
        );
        foreach ($goal_types as $type => $details) {
          $form['goals']['new'][$delta]['goal_type'][$type]['#description'] = $details['description'];
          $form['goals']['new'][$delta]['goal_type'][$type]['#image'] = $details['logo'];
        }
        $form['goals']['new'][$delta]['cancel'] = array(
          '#type' => 'submit',
          '#name' => 'cancel_goal_' . $delta,
          '#value' => t('Cancel'),
          '#submit' => array('acquia_lift_personalize_campaign_wizard_goals_ajax_cancel'),
          '#limit_validation_errors' => array(),
          '#ajax' => array(
            'callback' => 'acquia_lift_personalize_campaign_wizard_goals_ajax',
            'wrapper' => $new_goals_id,
          ),
        );
      } else {
        // Display a goal-type specific form for the new goal.
        module_load_include('inc', 'acquia_lift', 'acquia_lift.admin.unibar');
        // Add the form to change the type of goal selected.
        $selected_goal_type = $goal_types[$new_goal_type]['title'];
        $change_type_options = array(
          '#submit' => array('acquia_lift_personalize_campaign_wizard_goals_ajax_change'),
          '#ajax' => array(
            'callback' => 'acquia_lift_personalize_campaign_wizard_goals_ajax_delta',
            'wrapper' => $new_goal_details_id,
          ),
        );
        $form['goals']['new'][$delta] += _acquia_lift_personalize_campaign_wizard_change_type_form($selected_goal_type, $change_type_options);
        switch ($new_goal_type) {
          case 'existing':
            $form['goals']['new'][$delta]['details'] = _acquia_lift_existing_goal_create_form($agent_data);
            $form['goals']['new'][$delta]['save'] = array(
              '#name' => 'single_save_' . $delta,
              '#type' => 'submit',
              '#value' => t('Save'),
              '#limit_validation_errors' => array(array('goals', 'new', $delta)),
              '#submit' => array('acquia_lift_personalize_campaign_wizard_goals_single_submit'),
            );
            break;
          case 'page':
            $form['goals']['new'][$delta]['details'] = _acquia_lift_page_goal_create_form($agent_data);
            // Update the states properties for each page option.
            foreach (element_children($form['goals']['new'][$delta]['details']['options']['page']) as $page_option) {
              // Assumes that there is a single value being checked in
              // the event input list.
              $visible_states = $form['goals']['new'][$delta]['details']['options']['page'][$page_option]['#states']['visible'][':input[name="event[page]"]'];
              $form['goals']['new'][$delta]['details']['options']['page'][$page_option]['#states']['visible'] = array(
                ':input[name="goals[new][' . $delta . '][details][event][page]"]' => $visible_states,
              );
            }
            // Clear the element validation - instead it will be handled
            // in the form validator.
            unset($form['goals']['new'][$delta]['details']['#element_validate']);
            // Include the page for the validation callback.
            $form_state['build_info']['files'][] = drupal_get_path('module', 'acquia_lift') . '/acquia_lift.admin.unibar.inc';
            // The pages field needs to be manually populated.
            $form['goals']['new'][$delta]['details']['pages'] = array(
              '#type' => 'textarea',
              '#element_validate' => array('personalize_form_element_path_validate'),
              '#title' => t('Pages'),
              '#description' => t("Specify pages by using specifying one path per line. The '*' character is a wildcard. Example paths are blog for the blog page and blog/* for every personal blog. &lt;front&gt; is the front page."),
              '#allow_dynamic' => TRUE,
              '#allow_external' => FALSE,
              '#required' => TRUE,
              '#weight' => 10,
            );
            $form['goals']['new'][$delta]['save'] = array(
              '#name' => 'single_save_' . $delta,
              '#type' => 'submit',
              '#value' => t('Save'),
              '#limit_validation_errors' => array(array('goals', 'new', $delta)),
              '#submit' => array('acquia_lift_personalize_campaign_wizard_goals_single_submit'),
            );
            break;
          case 'element':
            $form['goals']['new'][$delta]['details'] = _acquia_lift_personalize_campaign_wizard_goal_element_form();
            break;
        }
        $form['goals']['new'][$delta]['cancel'] = array(
          '#type' => 'submit',
          '#name' => 'cancel_goal_' . $delta,
          '#value' => t('Cancel'),
          '#submit' => array('acquia_lift_personalize_campaign_wizard_goals_ajax_cancel'),
          '#limit_validation_errors' => array(),
          '#ajax' => array(
            'callback' => 'acquia_lift_personalize_campaign_wizard_goals_ajax',
            'wrapper' => $new_goals_id,
          ),
        );
      }
    }
  }
  // The button to add a new goal.
  $form['goals']['new']['add'] = array(
    '#type' => 'submit',
    '#value' => t('Add goal'),
    '#theme_wrappers' => array('acquia_lift_add_card_button'),
    '#access' => $editable,
    '#submit' => array('acquia_lift_personalize_campaign_wizard_goals_ajax_add'),
    '#limit_validation_errors' => array(),
    '#ajax' => array(
      'callback' => 'acquia_lift_personalize_campaign_wizard_goals_ajax',
      'wrapper' => $new_goals_id,
    ),
  );
}

/**
 * Alter hook for the targeting portion of the campaign wizard.
 *
 * Acquia Lift Target agents allow for audience designation to be used across
 * the agent.
 */
function acquia_lift_personalize_campaign_wizard_targeting_alter(&$form, &$form_state, $agent_data, $editable) {
  // If targeting was already found to be unsupported, then do not add.
  if (isset($form['unsupported'])) {
    return;
  }
  // Explicit targeting is not allowed for multivariate tests.
  $variation_set_handling = isset($agent_data->data['variation_set_handlng']) ? $agent_data->data['variation_set_handling'] : ACQUIA_LIFT_DECISION_LOCKSTEP;
  if ($variation_set_handling == ACQUIA_LIFT_DECISION_MULTIVARIATE) {
    unset($form['targeting'], $form['actions']);
    $form['main'] = array(
      '#markup' => t('Targeting is not supported for multivariate tests.')
    );
    return;
  }
  if (!$editable) {
    $form['targeting']['#disabled'] = TRUE;
  }
  $option_set = acquia_lift_get_option_set_for_targeting($agent_data->machine_name);

  if (empty($option_set)) {
    return;
  }
  // Take over the form validation and submit handling.
  $form['#validate'] = array('acquia_lift_personalize_campaign_wizard_validate');
  $form['#submit'] = array('acquia_lift_personalize_campaign_wizard_submit');

  unset($form['targeting']['option_sets']);

  $variation_options = $saved_targeting = array();
  $option_sets = personalize_option_set_load_by_agent($agent_data->machine_name);
  $all_variations = _acquia_lift_personalize_campaign_wizard_variation_displays($option_sets, $variation_set_handling, FALSE);

  // Convert the variations into an associative array of displays keyed by
  // targeting option id that can be used as select input options.
  $display_attributes = array();
  $has_multiple = count($option_sets) > 1;
  if ($has_multiple && $variation_set_handling == ACQUIA_LIFT_DECISION_LOCKSTEP) {
    $display_attributes['class'] = array('acquia-lift-variations-lockstep');
  }
  foreach($all_variations as $displays) {
    $variation_displays = array();
    $targeting_option_id = '';
    foreach ($displays as $option) {
      if (is_string($option)) {
        $variation_displays[] = $option;
      }
      else {
        $display_vars['option'] = $option['option_label'];
        if (isset($option['option_set_label'])) {
          $display_vars['option_set'] = $option['option_set_label'];
        }
        $variation_displays[] = theme('acquia_lift_single_variation', $display_vars);
      }
      if (!$has_multiple || !empty($option['targeting'])) {
        $targeting_option_id = $option['option_id'];
      }
    }
    // The targeting option id could remain empty if there are invalid
    // combinations of options resulting in a smaller number of variations in
    // the option set used for targeting.
    if (!empty($targeting_option_id)) {
      $variation_options[$targeting_option_id] = '<div ' . drupal_attributes($display_attributes) . '>' . implode('<br />', $variation_displays) . '</div>';
    }
  }

  // Load the targeting that is currently in use for a campaign.
  $existing_targeting = acquia_lift_get_structure_from_targeting($option_set);
  // Track whether an option to revert targeting changes should be shown.
  $show_revert = FALSE;
  $status = personalize_agent_get_status($agent_data->machine_name);
  // The settings reflected in the form will either be what is currently running
  // or any saved overrides from when this form was last submitted (if the
  // review step was never completed to apply them.)
  if (!empty($agent_data->data['lift_targeting'])) {
    // Get the current state of targeting work within the UI that has not
    // yet been saved to the agent.
    $saved_targeting = $agent_data->data['lift_targeting'];
    if ($editable && $saved_targeting != $existing_targeting && $status != PERSONALIZE_STATUS_NOT_STARTED) {
      drupal_set_message(t('The targeting settings shown here do not match what is currently implemented for this personalization.'), 'warning');
      $show_revert = TRUE;
    }
  }
  else {
    if ($editable && $status != PERSONALIZE_STATUS_NOT_STARTED) {
      // The current agent is running and there have been no UI changes made
      // since it was started.
      drupal_set_message(t('The targeting settings shown here represent what is currently implemented for this personalization.'));
    }
    // Set the saved targeting to be the same as existing targeting for
    // presentation within this form.
    $saved_targeting = $existing_targeting;
  }

  // Begin the audience form.
  $form['targeting']['audiences'] = array(
    '#type' => 'container',
    '#tree' => TRUE,
    '#attributes' => array(
      'id' => 'acquia-lift-targeting-audiences',
      'class' => array('personalize-wizard-column'),
    ),
  );

  // Add a new audience rule.
  $new_audiences_id = 'acquia-lift-targeting-audiences-new';
  $form['targeting']['audiences']['new'] = array(
    '#type' => 'container',
    '#attributes' => array(
      'id' => $new_audiences_id,
    ),
  );
  $form['targeting']['audiences']['new']['add'] = array(
    '#type' => 'submit',
    '#value' => t('Add target audience'),
    '#theme_wrappers' => array('acquia_lift_add_card_button'),
    '#access' => $editable,
    '#submit' => array('acquia_lift_personalize_campaign_wizard_targeting_audience_add'),
    '#limit_validation_errors' => array(),
    '#ajax' => array(
      'callback' => 'acquia_lift_personalize_campaign_wizard_targeting_audience_ajax',
      'wrapper' => $new_audiences_id,
      'effect' => 'fade',
    )
  );
  if (isset($form_state['new_audiences'])) {
    foreach($form_state['new_audiences'] as $delta => $details) {
      $form['targeting']['audiences']['new'][$delta] = array(
        '#type' => 'container',
        '#theme' => 'acquia_lift_card',
        '#collapsible' => TRUE,
        '#collapsed' => !isset($form_state['expanded_audience']) || $form_state['expanded_audience'] != $delta,
        '#title' => t('New target audience #@num', array('@num' => ($delta + 1))),
        '#attributes' => array(
          'class' => array('acquia-lift-campaign-add'),
        ),
        '#access' => $editable,
      );
      $form['targeting']['audiences']['new'][$delta]['details'] = _acquia_lift_personalize_campaign_wizard_targeting_audience($form, $form_state, $agent_data, 'new', $delta);
      // TRICKY: Name must be specified or else it will default to "op" and
      // conflict with the main process bar save button.
      $form['targeting']['audiences']['new'][$delta]['save_audience'] = array(
        '#name' => 'save_new_audience_' . $delta,
        '#type' => 'submit',
        '#value' => t('Save'),
      );
      $form['targeting']['audiences']['new'][$delta]['cancel'] = array(
        '#type' => 'submit',
        '#name' => 'cancel_audience_' . $delta,
        '#value' => t('Cancel'),
        '#limit_validation_errors' => array(),
        '#submit' => array('acquia_lift_personalize_campaign_wizard_targeting_audience_cancel'),
        '#ajax' => array(
          'callback' => 'acquia_lift_personalize_campaign_wizard_targeting_audience_ajax',
          'wrapper' => $new_audiences_id,
          'effect' => 'fade',
        ),
      );
    }
  }

  // Load all existing audiences for the agent.
  if (isset($option_set->targeting)) {
    // Sort the audiences by weight for display.
    uasort($option_set->targeting, function ($a, $b) {
      if ($a['weight'] === $b['weight']) {
        return 0;
      }
      else {
        return $a['weight'] < $b['weight'] ? -1 : 1;
      }
    });
    $form['targeting']['audiences']['existing'] = array(
      '#type' => 'container',
    );
    if (count($option_set->targeting) > 2 && $editable) {
      // Show instructions regarding re-ordering audiences.  This only applies
      // when there are at least two audiences beyond the "Everyone" audience.
      $form['targeting']['audiences']['existing']['priority'] = array(
        '#markup' => '<div class="acquia-lift-audience-priority"><h4>' . t('Audience Priority') . '</h4><p>' . t('Drag and drop audiences to change the order in which they are targeted.  Audiences at the top of the list will be targeted before audiences below them.') . '</p></div>',
      );
    }
    foreach ($option_set->targeting as $audience_id => $audience) {
      $form['targeting']['audiences']['existing'][$audience_id] = array(
        '#type' => 'container',
        '#theme' => 'acquia_lift_card',
        '#title' => isset($audience['label']) ? $audience['label'] : 'Audience: ' . $audience_id,
        '#collapsible' => FALSE,
        '#sortable' => $audience_id !== ACQUIA_LIFT_TARGETING_EVERYONE_ELSE && count($option_set->targeting) > 2,
        '#flag' => t('Test'),
        '#flag_visible' => isset($saved_targeting[$audience_id]) && count($saved_targeting[$audience_id]) > 1,
      );
      $form['targeting']['audiences']['existing'][$audience_id]['header'] = array(
        '#type' => 'container'
      );
      if ($editable) {
        if ($audience_id === ACQUIA_LIFT_TARGETING_EVERYONE_ELSE) {
          if (count($option_set->targeting) == 1) {
            $help = t('This audience is composed of all visitors who have not been selected to view a specific variation. Create a new audience above to assign specific variations to groups of visitors.');
          }
          else {
            $help = t('This audience is composed of all visitors who have not been selected to view a specific variation. Variations in the Everyone else audience will be shown to any person that does not match the audiences you have listed here.');
          }
          $form['targeting']['audiences']['existing'][$audience_id]['#title_attributes']['data-help-tooltip'] = $help;
        }
        else {
          $form['targeting']['audiences']['existing'][$audience_id]['header']['edit'] = array(
            '#type' => 'checkbox',
            '#title' => t('Edit'),
            '#attributes' => array(
              'class' => array('acquia-lift-edit'),
            ),
          );
          $form['targeting']['audiences']['existing'][$audience_id]['header']['delete'] = array(
            '#markup' => l(t('Delete'), 'admin/structure/personalize/manage/' . $agent_data->machine_name . '/audience/' . $audience_id . '/delete', array(
              'attributes' => array(
                'class' => array('acquia-lift-delete')
              ),
            )),
          );
          // Show the form to edit the audience segment when edit is selected.
          $form['targeting']['audiences']['existing'][$audience_id]['details'] = _acquia_lift_personalize_campaign_wizard_targeting_audience($form, $form_state, $agent_data, 'existing', $audience_id, $audience);
          $form['targeting']['audiences']['existing'][$audience_id]['details']['#states'] = array(
            'visible' => array(
              ':input[name="audiences[existing][' . $audience_id . '][header][edit]"]' => array('checked' => TRUE),
            )
          );
        }
      }
      // Show the form to assign variations when not in edit mode.
      $classes = array('acquia-lift-targeting-assignment');
      if ($audience_id === ACQUIA_LIFT_TARGETING_EVERYONE_ELSE) {
        $classes[] = 'acquia-lift-targeting-everyone-else';
      }
      // This message will only be shown when there two or more variations
      // assigned to an audience.
      $form['targeting']['audiences']['existing'][$audience_id]['message'] = array(
        '#type' => 'container',
        '#attributes' => array(
          'class' => array('acquia-lift-test-message'),
        ),
        'tests' => array(
          '#markup' => t('These variations will be displayed as a test, with the first variation in the list as the control.'),
        ),
      );
      $form['targeting']['audiences']['existing'][$audience_id]['assignment'] = array(
        '#type' => 'select',
        '#title' => t('Add display option'),
        '#multiple' => TRUE,
        '#options' => $variation_options,
        '#default_value' => isset($saved_targeting[$audience_id]) ? $saved_targeting[$audience_id] : '',
        '#empty_option' => t('Select...'),
        '#states' => array(
          'visible' => array(
            ':input[name="audiences[existing][' . $audience_id . '][header][edit]"]' => array('checked' => FALSE),
          )
        ),
        '#attributes' => array(
          'class' => $classes,
          'data-acquia-lift-targeting-droppable' => TRUE,
          'data-acquia-lift-targeting-allow-move' => TRUE,
          'data-acquia-lift-targeting-allow-remove' => TRUE,
        ),
      );
      // Keep track of the assignment order of options within the audience.
      $form['targeting']['audiences']['existing'][$audience_id]['assignment_order'] = array(
        '#type' => 'textfield',
        '#title' => t('Assignment order'),
        '#default_value' => isset($saved_targeting[$audience_id]) ? implode(',', $saved_targeting[$audience_id]) : '',
        '#attributes' => array(
          'class' => array('acquia-lift-targeting-assignment-order'),
        ),
      );
      // Show the ability to select a winner for an audience if:
      // - the campaign has run
      // - the campaign is not completed
      // - the audience was used in targeting
      // - the audience has a test currently implemented in Lift
      // - the audience contents has not changed from what was implemented
      $status = personalize_agent_get_status($agent_data->machine_name);
      if (!empty($agent_data->started) &&
        $status != PERSONALIZE_STATUS_COMPLETED &&
        isset($existing_targeting[$audience_id]) &&
        count($existing_targeting[$audience_id]) > 1 &&
        isset($saved_targeting[$audience_id]) &&
        $existing_targeting[$audience_id] == $saved_targeting[$audience_id]) {
        // Add this button to the footer so that it can still be available
        // even when the card is disabled.  This allows availability even
        // when the campaign is running.
        $form['targeting']['audiences']['existing'][$audience_id]['footer'] = array(
          '#type' => 'container',
        );
        $form['targeting']['audiences']['existing'][$audience_id]['footer']['complete_url'] = array(
          '#type' => 'hidden',
          // Per ctools modals the class name is the #id of the triggering
          // ajax button with "-url" suffix.
          '#attributes' => array('class' => array('acquia-lift-complete-' . $audience_id . '-url')),
          '#value' => url('admin/structure/personalize/manage/' . $agent_data->machine_name . '/audience/' . $audience_id . '/complete'),
        );
        $form['targeting']['audiences']['existing'][$audience_id]['footer']['complete'] = array(
          '#type' => 'button',
          '#value' => t('End test and choose winner'),
          '#attributes' => array(
            'id' => 'acquia-lift-complete-' . $audience_id,
            'class' => array(
              'ctools-use-modal',
              'ctools-modal-acquia-lift-style',
              'action-item-primary-active',
              'acquia-lift-submit-button',
              'acquia-lift-complete-audience',
            ),
            'data-acquia-lift-audience-id' => $audience_id,
          ),
          '#disabled' => FALSE,
        );
      }
    }

    // Show advanced settings for personalization.
    // These only apply when tests exist, but these are created on the client
    // so the mark-up needs to be here to show/hide dynamically.
    $form['targeting']['audiences']['advanced'] = array(
      '#type' => 'container',
      '#theme' => 'acquia_lift_card',
      '#title' => t('Test settings'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
      '#tree' => FALSE,
      '#attributes' => array(
        'class' => array('acquia-lift-test-options'),
      ),
      '#disabled' => !$editable,
    );
    $form['targeting']['audiences']['advanced']['options'] = _acquia_lift_personalize_campaign_wizard_settings_tests($agent_data, array());
  }

  // Add ability to revert changes to targeting.
  if ($show_revert) {
    $form['actions']['revert'] = array(
      '#type' => 'submit',
      '#value' => t('Revert changes'),
      '#attributes' => array(
        'class' => array('acquia-lift-submit-button'),
      ),
    );
  }

  // Show all variations so that even if there are unassigned variations they
  // can be dragged into appropriate audiences.
  $form['targeting']['variations'] = array(
    '#type' => 'container',
    '#attributes' => array(
      'class' => array('personalize-wizard-column'),
      'id' => 'acquia-lift-targeting-variations',
    ),
    '#tree' => TRUE,
  );
  $form['targeting']['variations']['options'] = array(
    '#type' => 'container',
    '#theme' => 'acquia_lift_card',
    '#title' => t('All variations'),
    '#collapsible' => FALSE,
    '#attributes' => array(
      'class' => array('acquia-lift-informational'),
    ),
  );
  $form['targeting']['variations']['options']['instructions'] = array(
    '#markup' => t('To display a variation to a specific audience, drag the variation to the audience.'),
  );
  $form['targeting']['variations']['options']['assignment'] = array(
    '#type' => 'select',
    '#title' => t('Drag variations to the desired audiences.'),
    '#multiple' => TRUE,
    '#options' => $variation_options,
    '#default_value' => array_keys($variation_options),
    '#attributes' => array(
      'class' => array('acquia-lift-targeting-assignment'),
      'data-acquia-lift-targeting-allow-copy' => TRUE,
      'data-acquia-lift-targeting-allow-move' => FALSE,
      'data-acquia-lift-targeting-allow-remove' => FALSE,
    ),
  );
}

/**
 * Alter hook for the scheduling portion of the campaign wizard.
 */
function acquia_lift_personalize_campaign_wizard_scheduling_alter(&$form, &$form_state, $agent_data, $editable) {
  if (!$editable) {
    $form['scheduling']['#disabled'] = TRUE;
  }
}

/**
 * Alter hook for the review portion of the campaign wizard.
 */
function acquia_lift_personalize_campaign_wizard_review_alter(&$form, &$form_state, $agent_data, $editable) {
  module_load_include('inc', 'acquia_lift', 'acquia_lift.admin');

  // Take over the form validation and submit handling.
  $form['#validate'] = array('acquia_lift_personalize_campaign_wizard_validate');
  $form['#submit'] = array('acquia_lift_personalize_campaign_wizard_submit');

  // Make the overview a card.
  $form['review']['summary_column']['summary']['#type'] = 'container';
  $form['review']['summary_column']['summary']['#theme'] = 'acquia_lift_card';

    // Add audiences to the summary section.

  // Add MVT and audience counts to the overview.
  $is_mvt = isset($agent_data->data['variation_set_handling']) && $agent_data->data['variation_set_handling'] == ACQUIA_LIFT_DECISION_MULTIVARIATE;
  if ($is_mvt) {
    // Show the summary data for the MVT tests.
    $form['review']['summary_column']['summary']['option_sets'] = array(
      '#markup' => theme('personalize_wizard_summary_count', array(
        'count' => 1,
        'details' => theme('html_tag', array(
          'element' => array(
            '#tag' => 'h3',
            '#value' => t('Multivariate Test'),
          ),
        )),
      )),
    );
  }
  else {
    $option_set = acquia_lift_get_option_set_for_targeting($agent_data->machine_name);
    if (!empty($agent_data->data['lift_targeting'])) {
      // Read audience information from the agent if changes have been made.
      $targeting = $agent_data->data['lift_targeting'];
    }
    else {
      // Otherwise read audience information from the option set targeting.
      $targeting = acquia_lift_get_structure_from_targeting($option_set);
    }

    // Loop through the audiences and determine the label and variation count
    // to display as summary data.
    $items = array();
    if (!empty($option_set->targeting)) {
      foreach ($option_set->targeting as $audience_id => $audience) {
        $num_variations = isset($targeting[$audience_id]) ? count($targeting[$audience_id]) : 0;
        $items[] = $audience['label'] . ' (' . $num_variations . ' ' . format_plural($num_variations, 'variation', 'variations') . ')';
      }
    }
    $count_audiences = count($items);
    $form['review']['summary_column']['summary']['audiences'] = array(
      '#markup' => theme('personalize_wizard_summary_count', array(
          'count' => $count_audiences,
          'details' => theme('item_list', array(
            'title' => format_plural($count_audiences, 'Audience', 'Audiences'),
            'items' => $items,
          )),
        )
      )
    );
  }

  // Move the scheduling summary element to the bottom.
  $form['review']['summary_column']['summary']['scheduling']['#weight'] = 10;

  // Move the warnings into a card.
  if (!empty($form['review']['messages']['warnings'])) {
    $form['review']['messages']['warnings']['#type'] = 'container';
    $form['review']['messages']['warnings']['#theme'] = 'acquia_lift_card';
  }

  // Provide summary data only available after a personalization has been
  // started.
  if (empty($agent_data->started)) {
    return;
  }
  // Generate next steps.
  $option_sets = personalize_option_set_load_by_agent($agent_data->machine_name);
  $next_links = array(
    'blocks' => array(),
    'elements' => array(),
    'fields' => array(),
  );
  $destination = 'admin/structure/personalize/manage/' . $agent_data->machine_name . '/review';
  $total_next_steps = 0;
  foreach($option_sets as $option_set) {
    switch($option_set->plugin) {
      case 'block':
        $next_links['blocks'][] = l(check_plain($option_set->label), 'admin/structure/block', array('query' => array('destination' => $destination)));
      break;
      case 'elements':
        $pages = $option_set->data['pages'];
        if (!empty($pages)) {
          $pages = preg_split('~[\r\n]+~', $pages);
          $url = reset($pages);
          $next_links['elements'][] = l(check_plain($option_set->label), $url);
        }
      break;
      case 'fields':
        $url = _personalize_fields_get_entity_link_from_option_set($option_set);
        $next_links['fields'][] = l(check_plain($option_set->label), $url, array('query' => array('destination' => $destination)));
      break;
    }
  }
  $total_next_steps = array_reduce($next_links, function($total, $type) {
    $total += count($type);
    return $total;
  }, 0);
  if ($total_next_steps > 0) {
    $form['review']['messages']['next'] = array(
      '#type' => 'container',
      '#title' => format_plural($total_next_steps, 'Next step', 'Next steps'),
      '#theme' => 'acquia_lift_card',
      '#collapsible' => FALSE,
      '#attributes' => array(
        'class' => array('acquia-lift-review-next-steps'),
      ),
    );
  }
  if (!empty($next_links['blocks'])) {
    $form['review']['messages']['next']['blocks'] = array(
      '#markup' => theme('item_list', array(
        'items' => $next_links['blocks'],
        'title' => format_plural(
          count($next_links['blocks']),
          'Place your personalized block on your website',
          'Place your personalized blocks on your website'
        ),
      )),
    );
  }
  if (!empty($next_links['elements'])) {
    $form['review']['messages']['next']['elements'] = array(
      '#markup' => theme('item_list', array(
        'items' => $next_links['elements'],
        'title' => format_plural(
          count($next_links['elements']),
          'View your element variation set page',
          'View your element variation set pages'
        ),
      )),
    );
  }
  if (!empty($next_links['fields'])) {
    $form['review']['messages']['next']['fields'] = array(
      '#markup' => theme('item_list', array(
        'items' => $next_links['fields'],
        'title' => format_plural(
          count($next_links['fields']),
          'View your field variation set page',
          'View your field variation set pages'
        ),
      )),
    );
  }

  // Show resources for further troubleshooting.
  $form['review']['messages']['resources'] = array(
    '#type' => 'container',
    '#title' => t('Need help?'),
    '#theme' => 'acquia_lift_card',
    '#collapsible' => FALSE,
    '#attributes' => array(
      'class' => array('acquia-lift-review-next-steps'),
    ),
  );
  $url_options = array(
    'external' => TRUE,
    'attributes' => array(
      'target' => 'acquia_lift_resources',
    ),
  );
  $form['review']['messages']['resources']['links'] = array(
    '#markup' => theme('item_list', array(
      'items' => array(
        l(t('Learn how to utilize your personalization\'s reports'),
          ACQUIA_LIFT_DOCUMENTATION . '/campaign/report',
          $url_options
        ),
        l(t('Read more Acquia Lift documentation'),
          ACQUIA_LIFT_DOCUMENTATION,
          $url_options
        ),
        l(t('Find answers to common questions'),
          ACQUIA_LIFT_DOCUMENTATION . '/troubleshooting',
          $url_options
        ),
      ),
      'title' => t('Still having trouble with your personalization?'),
    )),
  );
}

/**
 ********************************************************************
 * S E C T I O N  H E L P
 *
 * Note: unless form alteration callbacks, help functions return the
 * renderable elements to be included as help rather than altering the form
 * directly to allow for consistent placement within the page.
 *
 ********************************************************************
 */

/**
 * Section help callback for variations section.
 */
function acquia_lift_personalize_campaign_wizard_variations_help(&$form, $form_state, $agent_data, $editable) {
  global $user;
  $dismissed = isset($user->data['acquia_lift_help_dismiss']) ? $user->data['acquia_lift_help_dismiss'] : array();

  $help_text = '';
  $help_text .= '<p>';
  $help_text .= t('A Variation Set contains all Variations which will be shown in a specific area on your site.');
  $help_text .= '</p><p>';
  $help_text .= 'For example, a "Frontpage Offers" block or a "Homepage Banner" would be a Variation Set, and the individual Variations will be the text or creative that you\'ll want to show to different visitors. You will set up testing and targeting rules on the "Who" page. Here, you should simply create all the variations which you want to show.';
  $help_text .= '</p><p>';
  $help_text .= t('If more than one Variation Set exists in a personalization, then the Variations will be in "lock step" with one another, meaning if a user sees Variation A from one Variation Set, they will always be shown Variation A in the the other Variation Set, whether it is set up as a test or as targeting.');
  $help_text .= '</p>';

  unset($form['variations']['title']['#instructions']);
  return array(
    '#type' => 'container',
    '#theme' => 'acquia_lift_wizard_section_help',
    '#image' => drupal_get_path('module', 'acquia_lift') . '/images/help/section_what.png',
    '#alt' => t('Structure of a variation set.'),
    '#text' => $help_text,
    '#collapsed' => array_search('variations', $dismissed) !== FALSE,
    'dismiss' => array(
      '#type' => 'button',
      '#name' => 'dismiss_variations',
      '#value' => t('OK, got it'),
      '#ajax' => array(
        'callback' => 'acquia_lift_personalize_campaign_wizard_help_ajax',
        'progress' => array(),
      ),
      '#limit_validation_errors' => array(),
    ),
  );
}

/**
 * Section help callback for goals section.
 */
function acquia_lift_personalize_campaign_wizard_goals_help(&$form, $form_state, $agent_data, $editable) {
  global $user;
  $dismissed = isset($user->data['acquia_lift_help_dismiss']) ? $user->data['acquia_lift_help_dismiss'] : array();

  unset($form['goals']['title']['#instructions']);
  $help_text = '';
  $help_text .= '<p>';
  $help_text .= t('Goals are the things that you want your visitors to do based on this personalization.');
  $help_text .= '</p><p>';
  $help_text .= t('Examples include clicking through on a link in the personalization, filling out a form on a later page, or completing a purchase. Completion of goals is how conversion rate will be determined and will show up in later reports.');
  $help_text .= '</p><p>';
  $help_text .= t('A default goal is created for each Variation Set which will capture a visitor clicking within any Variation in the Variation Set and in many cases you can simply use this goal and not add any others. If you create more than one goal, and they have different levels of importance, you can set goal value to reflect that and later view reports based on total Conversion Value rather than just Conversion Rate.');
  $help_text .= '<p>';

  return array(
    '#type' => 'container',
    '#theme' => 'acquia_lift_wizard_section_help',
    '#image' => drupal_get_path('module', 'acquia_lift') . '/images/help/section_why.png',
    '#alt' => t('Structure of a personalization with goals.'),
    '#text' => $help_text,
    '#collapsed' => array_search('goals', $dismissed) !== FALSE,
    'dismiss' => array(
      '#type' => 'button',
      '#name' => 'dismiss_goals',
      '#value' => t('OK, got it'),
      '#ajax' => array(
        'callback' => 'acquia_lift_personalize_campaign_wizard_help_ajax',
        'progress' => array(),
      ),
      '#limit_validation_errors' => array(),
    ),
  );
}

/**
 * Section help callback for targeting section.
 */
function acquia_lift_personalize_campaign_wizard_targeting_help(&$form, $form_state, $agent_data, $editable) {
  global $user;
  $dismissed = isset($user->data['acquia_lift_help_dismiss']) ? $user->data['acquia_lift_help_dismiss'] : array();

  unset($form['targeting']['title']['#instructions']);
  $help_text = '';
  $help_text .= '<p>';
  $help_text .= t('Audiences are the groups of people you wish to show different variations to.');
  $help_text .= '</p><p>';
  $help_text .= t('For example, you can create an audience for "Visitors from Los Angeles aged 18-25" and then target them with the "Winter Vacation" variation. The Variation(s) in the Everyone / Everyone Else box will be shown to any visitor who does not fall into an audience above it.');
  $help_text .= '</p><p>';
  $help_text .= t('You can add more than one Variation to an Audience, which will automatically create an A/B test between those Variations. Each Audience can have its own A/B Test. Also note that in the case where a visitor is in more than one Audience, the first Audience (from top to bottom) that matches will be used, so you can drag the audiences to rearrange if necessary. If you simply wish to test with no targeting, place all variations in the "Everyone" box.');
  $help_text .= '</p>';

  return array(
    '#type' => 'container',
    '#theme' => 'acquia_lift_wizard_section_help',
    '#image' => drupal_get_path('module', 'acquia_lift') . '/images/help/section_who.png',
    '#alt' => t('Structure of a personalization with targeting.'),
    '#text' => $help_text,
    '#collapsed' => array_search('targeting', $dismissed) !== FALSE,
    'dismiss' => array(
      '#type' => 'button',
      '#name' => 'dismiss_targeting',
      '#value' => t('OK, got it'),
      '#ajax' => array(
        'callback' => 'acquia_lift_personalize_campaign_wizard_help_ajax',
        'progress' => array(),
      ),
      '#limit_validation_errors' => array(),
    ),
  );
}

/**
 ********************************************************************
 *
 * A J A X  C A L L B A C K S
 *
 ********************************************************************
 */

/**
 * Ajax submit handler for any help section dismissal button
 */
function acquia_lift_personalize_campaign_wizard_help_ajax($form, &$form_state) {
  global $user;
  $current_dismiss = !empty($user->data['acquia_lift_help_dismiss']) ? $user->data['acquia_lift_help_dismiss'] : array();
  // #name property is "display_{step}"
  $dismiss = preg_replace('/^dismiss_/', '', $form_state['triggering_element']['#name']);
  if (!in_array($dismiss, $current_dismiss)) {
    $current_dismiss[] = $dismiss;
    user_save($user, array('data' => array('acquia_lift_help_dismiss' => $current_dismiss)));
  }
}

/**
 * Ajax submit handler when adding a new variation set.
 */
function acquia_lift_personalize_campaign_wizard_variations_ajax_add($form, &$form_state) {
  // Add a new variation set of an unspecified type.
  $form_state['new_option_sets'][] = '';
  end($form_state['new_option_sets']);
  $form_state['expanded_option_set'] = key($form_state['new_option_sets']);
  $form_state['rebuild'] = TRUE;
}

/**
 * Ajax submit handler to change the type of variation set being created.
 */
function acquia_lift_personalize_campaign_wizard_variations_ajax_change($form, &$form_state) {
  $delta = _acquia_lift_personalize_campaign_wizard_next_element($form_state, 'new');
  $form_state['new_option_sets'][$delta] = '';
  $form_state['expanded_option_set'] = $delta;
  $form_state['rebuild'] = TRUE;
}

/**
 * Ajax submit handler to cancel the creation of a specific variation set.
 */
function acquia_lift_personalize_campaign_wizard_variations_ajax_cancel($form, &$form_state) {
  // Get the delta for the cancelled variation set.
  $delta = _acquia_lift_personalize_campaign_wizard_next_element($form_state, 'new');
  unset($form_state['new_option_sets'][$delta]);
  $form_state['rebuild'] = TRUE;
}

/**
 * Ajax handler for the full new option set section.
 */
function acquia_lift_personalize_campaign_wizard_variations_ajax($form, &$form_state) {
  drupal_get_messages();
  return $form['variations']['editing']['new'];
}

/**
 * Ajax handler for a specific new variation set.
 */
function acquia_lift_personalize_campaign_wizard_variations_ajax_delta($form, &$form_state) {
  drupal_get_messages();
  $delta = _acquia_lift_personalize_campaign_wizard_next_element($form_state, 'new');
  return $form['variations']['editing']['new'][$delta];
}

/**
 * Submit handler to add another block option to a personalized block form.
 */
function acquia_lift_personalize_campaign_wizard_blocks_add($form, &$form_state) {
  // Value will be "blocks_add" or "blocks_{osid}" where the second part is the
  // key within the form state storage for the number of blocks to generate.
  list($blocks, $key) = explode('_', $form_state['triggering_element']['#value']);
  if ($key === 'add') {
    $key = 'add_' . _acquia_lift_personalize_campaign_wizard_next_element($form_state, 'new');
  }
  $form_state['option_set_num_blocks'][$key]++;
  $form_state['rebuild'] = TRUE;
}

/**
 * Submit handler to remove a block option from a personalized block form.
 */
function acquia_lift_personalize_campaign_wizard_blocks_remove($form, &$form_state) {
  // Value will be "remove_{delta}_{osid}" where the {delta} is the block delta
  // to remove and {osid} is the option set to remove it from.
  list($remove, $delta, $osid) = explode('_', $form_state['triggering_element']['#value']);
  if ($osid === 'add') {
    $osid = 'add_' . _acquia_lift_personalize_campaign_wizard_next_element($form_state, 'new');
  }
  $form_state['option_set_to_remove'][$osid] = $delta;
  $form_state['rebuild'] = TRUE;
}

/**
 * Callback to for AJAX to generate the personalize block wrapper form.
 */
function acquia_lift_personalize_campaign_wizard_blocks_ajax($form, &$form_state) {
  drupal_get_messages();
  // Both the add and remove buttons have the osid as the last in a "_"
  // delimited list.
  $value_parts = explode('_', $form_state['triggering_element']['#value']);
  $osid = end($value_parts);
  if ($osid === 'add') {
    $delta = _acquia_lift_personalize_campaign_wizard_next_element($form_state, 'new');
    return $form['variations']['editing']['new'][$delta]['block']['content']['pblock_wrapper']['blocks'];
  }
  else {
    return $form['variations']['editing']['option_sets']['option_set_' . $osid]['content']['pblock_wrapper']['blocks'];
  }
}

/**
 * Ajax submit handler when adding a new goal.
 */
function acquia_lift_personalize_campaign_wizard_goals_ajax_add($form, &$form_state) {
  // Add a new goal of an unspecified type.
  $form_state['new_goals'][] = '';
  end($form_state['new_goals']);
  $form_state['expanded_goal'] = key($form_state['new_goals']);
  $form_state['rebuild'] = TRUE;
}

/**
 * Ajax submit handler to change the type of goal being created.
 */
function acquia_lift_personalize_campaign_wizard_goals_ajax_change($form, &$form_state) {
  // Get the delta for the new goal.
  $delta = _acquia_lift_personalize_campaign_wizard_next_element($form_state, 'new');
  $form_state['new_goals'][$delta] = '';
  $form_state['expanded_goal'] = $delta;
  $form_state['rebuild'] = TRUE;
}

/**
 * Ajax submit handler to cancel the creation of a specific goal.
 */
function acquia_lift_personalize_campaign_wizard_goals_ajax_cancel($form, &$form_state) {
  // Get the delta for the cancelled goal.
  $delta = _acquia_lift_personalize_campaign_wizard_next_element($form_state, 'new');
  unset($form_state['new_goals'][$delta]);
  $form_state['rebuild'] = TRUE;
}

/**
 * Ajax handler when adding a new goal.
 */
function acquia_lift_personalize_campaign_wizard_goals_ajax($form, &$form_state) {
  drupal_get_messages();
  return $form['goals']['new'];
}

/**
 * Ajax handler when editing a specific new goal
 */
function acquia_lift_personalize_campaign_wizard_goals_ajax_delta($form, &$form_state) {
  drupal_get_messages();
  $delta = _acquia_lift_personalize_campaign_wizard_next_element($form_state, 'new');
  return $form['goals']['new'][$delta];
}

/**
 * Ajax handler for updating an existing goal
 */
function acquia_lift_personalize_campaign_wizard_goals_ajax_existing($form, &$form_state) {
  drupal_get_messages();
  $delta = _acquia_lift_personalize_campaign_wizard_next_element($form_state, 'all_goals');
  return $form['goals']['all_goals'][$delta];
}

/**
 * Submit handler for the "Add Context" button.
 */
function acquia_lift_personalize_campaign_wizard_targeting_add($form, &$form_state) {
  // New or existing indicator:
  $type_key = _acquia_lift_personalize_campaign_wizard_next_element($form_state, 'audiences');
  $context_key = _acquia_lift_personalize_campaign_wizard_next_element($form_state, $type_key);

  $form_state['num_contexts_audience'][$type_key][$context_key]++;
  $form_state['rebuild'] = TRUE;
}

/**
 * Submit handler for the "Remove Context" button.
 */
function acquia_lift_personalize_campaign_wizard_targeting_remove($form, &$form_state) {
  $type_key = _acquia_lift_personalize_campaign_wizard_next_element($form_state, 'audiences');
  $context_key = _acquia_lift_personalize_campaign_wizard_next_element($form_state, $type_key);
  $delta = _acquia_lift_personalize_campaign_wizard_next_element($form_state, 'contexts');

  $form_state['to_remove_audience'][$type_key][$context_key] = $delta;
  $form_state['rebuild'] = TRUE;
}

/**
 * Ajax callback for the add context and remove context buttons.
 */
function acquia_lift_personalize_campaign_wizard_targeting_ajax($form, &$form_state) {
  // Don't show any messages within the AJAX returned.
  drupal_get_messages();
  $type_key = _acquia_lift_personalize_campaign_wizard_next_element($form_state, 'audiences');
  $context_key = _acquia_lift_personalize_campaign_wizard_next_element($form_state, $type_key);
  return $form['targeting']['audiences'][$type_key][$context_key]['details'];
}

/**
 * Submit handler for the "Add target audience" button.
 */
function acquia_lift_personalize_campaign_wizard_targeting_audience_add($form, &$form_state) {
  // Since there are no types to select or other contextual data, just need
  // to keep track of the indexes of the new audiences.
  $form_state['new_audiences'][] = '';
  end($form_state['new_audiences']);
  $form_state['expanded_audience'] = key($form_state['new_audiences']);
  $form_state['rebuild'] = TRUE;
}

/**
 * Submit handler for the "Cancel" add audience button.
 */
function acquia_lift_personalize_campaign_wizard_targeting_audience_cancel($form, &$form_state) {
  $context_key = _acquia_lift_personalize_campaign_wizard_next_element($form_state, 'new');
  unset($form_state['new_audiences'][$context_key]);
  $form_state['rebuild'] = TRUE;
}

/**
 * AJAX callback for the add and cancel audience buttons.
 */
function acquia_lift_personalize_campaign_wizard_targeting_audience_ajax($form, &$form_state) {
  drupal_get_messages();
  return $form['targeting']['audiences']['new'];
}

/**
 ********************************************************************
 *
 * V A L I D A T I O N
 *
 ********************************************************************
 */

/**
 * Validation for the entire campaign wizard (all steps).
 */
function acquia_lift_personalize_campaign_wizard_validate(&$form, &$form_state) {
  $step_name = $form_state['storage']['step'];
  // Do not validate if the form is disabled.
  if (!empty($form[$step_name]['#disabled'])) {
    return;
  }

  // Run any base form validation if available.
  module_load_include('inc', 'personalize', 'personalize.admin.campaign');
  if (function_exists('personalize_campaign_wizard_validate_base')) {
    personalize_campaign_wizard_validate_base($form, $form_state);
  }

  $function = 'acquia_lift_personalize_campaign_wizard_' . $step_name . '_validate';
  if (function_exists($function)) {
    $function($form, $form_state);
  }
}

/**
 * Validation function for variations form.
 */
function acquia_lift_personalize_campaign_wizard_variations_validate(&$form, &$form_state) {
  // Get a reference to the full values array.
  $values = $form_state['values'];

  // Handles validation of block values and setting form errors correctly.
  // Using #limit_validation_errors for the single submit means that any
  // form_set_error from the personalize_blocks_form_validate() function
  // will never be set.  For now reproducing the relevant validation here.
  // @todo there has to be a better way with some further refactoring.
  $validateBlock = function($values, $form_error_parent) {
    $valid = FALSE;
    $first_block = '';
    if (isset($values['blocks'])) {
      $num_blocks = 0;
      foreach ($values['blocks'] as $j => $block) {
        if (empty($first_block)) {
          $first_block = $j;
        }
        if (empty($block['option_label'])) {
          form_set_error($form_error_parent . 'blocks][' . $j . '][option_label', t('A label is required for each block option.'));
        }
        if ($block['block']['block_type'] === 'add') {
          if (empty($block['block']['add']['info'])) {
            form_set_error($form_error_parent . 'blocks][' . $j . '][block][add][info', t('A block description is required when creating a new block.'));
          }
          if (empty($block['block']['add']['body']['value'])) {
            form_set_error($form_error_parent . 'blocks][' . $j . '][block][add][body', t('The block body is required when creating a new block.'));
          }
          // @see block_add_block_form_validate().
          $custom_block_exists = (bool) db_query_range('SELECT 1 FROM {block_custom} WHERE info = :info', 0, 1, array(':info' => $block['block']['add']['info']))->fetchField();

          if ($custom_block_exists) {
            form_set_error($form_error_parent . 'blocks][' . $j . '][block][add][info', t('Ensure that each block description is unique.'));
          }
          else {
            $num_blocks++;
          }
        }
        if (!empty($block['block']['bid'])) {
          $num_blocks++;
        }
      }
      $valid = $num_blocks > 1;
    }
    if (!$valid) {
      form_set_error($form_error_parent . 'blocks][' . $first_block . '][block][block_type]', t('You must add at least 2 blocks to your personalized block'));
    }
  };


  // Validation for existing option set types.
  if (!empty($form_state['values']['variations']['editing']['option_sets'])) {
    foreach ($form_state['values']['variations']['editing']['option_sets'] as $option_set_id => $option_set_values) {
      switch ($option_set_values['option_set']->plugin) {
        case "block":
          $validateBlock($option_set_values['content']['pblock_wrapper'], 'variations][editing][option_sets][' . $option_set_id . '][content][pblock_wrapper][');
          break;
        case "elements":
          if ($option_set_values['advanced']['pages_all'] == 0) {
            if (empty($option_set_values['advanced']['pages'])) {
              form_set_error('variations][editing][option_sets][' . $option_set_id . '][advanced][pages]', t('There must be at least one page entered where the webpage element variation is executed.'));
            }
            else {
              personalize_form_element_path_validate($form['variations']['editing']['option_sets'][$option_set_id]['advanced']['pages'], $form_state);
            }
          }
          break;
        default:
          // Unsupported option set type.
      }
      // Validate advanced settings.
      personalize_campaign_wizard_validate_variations_advanced($form_state['values']['variations']['editing']['option_sets'][$option_set_id], 'variations][editing][option_sets][' . $option_set_id . ']');
    }
  }
  // Validation for any new option set types once the details have been input.
  if (!empty($form_state['new_option_sets'])) {
    foreach($form_state['new_option_sets'] as $delta => $type) {
      if ($type != 'block') {
        continue;
      }
      $validateBlock($form_state['values']['variations']['editing']['new'][$delta]['block']['content']['pblock_wrapper'], 'variations][editing][new][' . $delta . '][block][content][pblock_wrapper][');
    }
  }

  // Put the form state back the way it was for the next steps in form processing.
  $form_state['values'] = $values;
}

/**
 * Validation function for goals form.
 */
function acquia_lift_personalize_campaign_wizard_goals_validate(&$form, &$form_state) {
  // Make sure that each action is only set once per campaign.
  // Note that the error message is only shown once as we only have access
  // to the action machine name for display and it would be too repetitive to
  // to show the same message over and over.
  $used_actions = array();
  if (!empty($form_state['values']['goals']['all_goals'])) {
    foreach($form_state['values']['goals']['all_goals'] as $delta => $goal) {
      if (empty($goal['action_name'])) {
        continue;
      }
      if (array_search($goal['action_name'], $used_actions) !== FALSE) {
        form_set_error('goals[all_goals][' . $delta . '][action_name]', t('Actions can only be used once per personalization.'));
        return;
      }
      else {
        $used_actions[] = $goal['action_name'];
      }
    }
  }
  if (!empty($form_state['new_goals'])) {
    foreach ($form_state['new_goals'] as $delta => $type) {
      if ($type == 'page') {
        // Validate the page input via plugin validator.
        ctools_include('plugins');
        if ($class = ctools_plugin_load_class('visitor_actions', 'actionable_element', 'page', 'handler')) {
          // @todo Any validation errors shown will not be shown with the
          // correct field highlighted, or at all if limit_validation_errors
          // is used.
          call_user_func_array(array($class, 'validate'), array($form_state['values']['goals']['new'][$delta]['details']));
        }
        continue;
      }
      if ($type !== 'existing') {
        continue;
      }
      $new_action_name = $form_state['values']['goals']['new'][$delta]['details']['action_name'];
      if (empty($new_action_name)) {
        continue;
      }
      if (array_search($new_action_name, $used_actions) !== FALSE) {
        form_set_error('goals[new][' . $delta . '][details][action_name]', t('Actions can only be used once per personalization.'));
        return;
      }
      $used_actions[] = $new_action_name;
    }
  }
}

/**
 * Validation function for targeting form.
 */
function acquia_lift_personalize_campaign_wizard_targeting_validate(&$form, &$form_state) {
  // Allow advanced AJAX functions such as add/remove contexts to skip
  // validation and still operate on the full form value set.
  if (!empty($form_state['triggering_element']['#skip_validation'])) {
    return;
  }

  $agent_data = $form['#agent'];

  /**
   * Anonymous function to validate a specific audience.
   */
  $validateAudienceFields = function($values, $form_prefix, $is_new, $contexts) use ($agent_data) {
    if (empty($values['name'])) {
      form_set_error($form_prefix . 'name', t('You must specify the name of the audience.'));
      return;
    }
    else if ($is_new) {
      // Verify that a new audience will have a unique machine name.
      module_load_include('inc', 'acquia_lift', 'acquia_lift.admin');
      $machine_name = personalize_generate_machine_name($values['name'], NULL, '-');
      $option_set = acquia_lift_get_option_set_for_targeting($agent_data->machine_name);
      if (isset($option_set->targeting[$machine_name])) {
        form_set_error($form_prefix . 'name', t('Please choose a different name for your audience as this one is already taken'));
        return;
      }
    }
    $has_context = FALSE;
    $audience_label = $values['name'];
    $contexts = isset($values['mapping']['contexts']) ? element_children($values['mapping']['contexts']) : array();
    foreach ($contexts as $index) {
      if (!empty($values['mapping']['contexts'][$index]['context'])) {
        $has_operator = !empty($values['mapping']['contexts'][$index]['value']['operator']);
        $match = $values['mapping']['contexts'][$index]['value']['match'];
        $has_value = !empty($match) || $match === '0';
        $context_value = $values['mapping']['contexts'][$index]['context'];
        $context_label = $contexts[$index]['context']['#options'][$context_value];
        //$context_label = $form['targeting']['audiences'][$audience_id]['details']['mapping']['contexts'][$index]['context']['#options'][$context_value];
        // Make sure that an operator and value have been entered.
        if (!$has_operator) {
          form_set_error($form_prefix . 'mapping][contexts][' . $index . '][value][operator', t('Please choose the operator for %context in the %audience audience.', array(
            '%context' => $context_label,
            '%audience' => $audience_label,
          )));
        }
        if (!$has_value) {
          form_set_error($form_prefix . 'mapping][contexts][' . $index . '][value][match', t('Please choose the value for %context context in the %audience audience.', array(
            '%context' => $context_label,
            '%audience' => $audience_label,
          )));
        }
        if ($has_operator && $has_value) {
          $has_context = TRUE;
        }
      }
    }
    if (!$has_context) {
      form_set_error($form_prefix . 'mapping][contexts][add][context', t('Please select at least one context to define the %audience audience.', array(
        '%audience' => $audience_label,
      )));
    }
  };

  // If we are adding a single audience validate the required audience fields.
  $delta = _acquia_lift_personalize_campaign_wizard_next_element($form_state, 'new');
  if (!empty($delta) && is_numeric($delta)) {
    $validateAudienceFields($form_state['values']['audiences']['new'][$delta]['details'], 'audiences[new][' . $delta . '][details][', TRUE, $form['targeting']['audiences']['new'][$delta]['details']['mapping']['contexts']);
    return;
  }
  // Validate all submitted audiences.
  $has_tests = FALSE;
  foreach ($form_state['values']['audiences'] as $type_id => $type_data) {
    $is_new = $type_id === 'new';
    foreach ($form_state['values']['audiences'][$type_id] as $audience_id => $audience) {
      if ($audience_id === ACQUIA_LIFT_TARGETING_EVERYONE_ELSE) {
        continue;
      }
      else if (!is_array($audience) || !isset($audience['details'])) {
        // Not an audience but some other form field.
        continue;
      }
      $validateAudienceFields($audience['details'], 'audiences[' . $type_id . '][' . $audience_id . '][details][', $is_new, $audience['details']['mapping']['contexts']);
      if (isset($audience['assignment_order'])) {
        $has_tests = $has_tests || count(explode(',', $audience['assignment_order'])) > 1;
      }
    }
  }

  // If any of the audiences contain tests, then also validate the test settings.
  if (isset($form_state['values']['control_rate'])) {
    $rate = $form_state['values']['control_rate'];
    if (!is_numeric($rate) || !($rate >= 0 && $rate <= 100)) {
      form_set_error('control_rate', t('Invalid percent to test specified'));
    }
  }
  if (isset($form_state['values']['explore_rate'])) {
    $rate = $form_state['values']['explore_rate'];
    if (!is_numeric($rate) || !($rate >= 0 && $rate <= 100)) {
      form_set_error('explore_rate', t('Invalid percent to test specified'));
    }
  }
}

/**
 ********************************************************************
 *
 * S U B M I S S I O N S
 *
 ********************************************************************
 */

/**
 * General Acquia Lift submission of campaign step form.
 */
function acquia_lift_personalize_campaign_wizard_submit(&$form, &$form_state) {
  module_load_include('inc', 'personalize', 'personalize.admin');
  module_load_include('inc', 'personalize', 'personalize.admin.campaign');
  module_load_include('inc', 'acquia_lift', 'acquia_lift.admin');

  // Only allow edit submit handlers to run if changes are allowed.
  $step_name = $form_state['storage']['step'];
  if (empty($form[$step_name]['#disabled'])) {

    // Run the base submit form handling.
    $agent_data = personalize_campaign_wizard_submit_base($form, $form_state);
    $agent_instance = personalize_agent_load_agent($agent_data->machine_name);

    // Call any step-specific submission handlers.
    $submit_function = 'acquia_lift_personalize_campaign_wizard_' . $form_state['storage']['step'] . '_submit';
    if (function_exists($submit_function)) {
      $submit_function($form, $form_state, $agent_data, $agent_instance);
    }

    // Save any changes to the campaign.
    personalize_agent_save($agent_data);
  }
  else {
    $agent_instance = personalize_agent_load_agent($form['#agent']->machine_name);
  }

  // If the subform submit handlers didn't already set a redirect, then set the
  // next step and rebuild the form.
  if (empty($form_state['redirect'])) {
    _personalize_campaign_wizard_rebuild($form_state, $agent_instance);
  }
}

/**
 * Submit handler for changing campaign status to an editable campaign.
 */
function acquia_lift_personalize_campaign_wizard_submit_editable(&$form, &$form_state) {
  // Run the base submit form handling.
  $agent_data = personalize_campaign_wizard_submit_base($form, $form_state);
  $next_status = $form_state['triggering_element']['#personalize_next_status'];
  if (personalize_agent_set_status($agent_data->machine_name, $next_status)) {
    drupal_set_message(t("The %campaign_name personalization is now editable.", array(
      '%campaign_name' => $agent_data->label,
    )));
  }
}

/**
 * Helper submit function to save a single block variation.
 */
function _acquia_lift_personalize_campaign_wizard_block_single_save($block_content, $option_set = NULL, $form_state) {
  module_load_include('inc', 'personalize_blocks', 'personalize_blocks.admin');
  $delta = _acquia_lift_personalize_campaign_wizard_next_element($form_state, 'new');
  _acquia_lift_personalize_campaign_wizard_form_state_blocks_alter($form_state, $block_content, 'add_' . $delta, $option_set);
  // Create any new blocks
  foreach($form_state['values']['blocks'] as &$block) {
    if ($block['block']['block_type'] === 'add') {
      $block['block']['bid'] = _personalize_blocks_add_custom_block($block['block']['add']);
    }
  }
  $pblock = _personalize_blocks_convert_form_to_personalized_block($form_state);
  personalize_option_set_save($pblock);
}

/**
 * Helper submit function to save a single existing element variation.
 *
 * @param array $values
 *   The values from the form state specific to this option set
 * @param stdClass $option_set
 *   The option set to be edited.
 */
function _acquia_lift_personalize_campaign_wizard_element_single_save($values, $option_set) {
  // Make sure to start with the latest option set information.
  $option_set = personalize_option_set_load($option_set->osid);
  $option_set->data['pages'] = isset($values['advanced']['pages_all']) && $values['advanced']['pages_all'] == 1 ? '' : $values['advanced']['pages'];
  personalize_option_set_save($option_set);
}

/**
 * Helper submit function to submit the general and advanced information
 * for an existing option set.
 *
 * @param array $values
 *   The form state values specific to the option set.
 * @param stdClass $option_set
 *   The option set object submitted with the form.
 */
function _acquia_lift_personalize_campaign_wizard_variations_single_save_general($values, $option_set) {
  // Make sure to start with the latest option set information.
  $option_set = personalize_option_set_load($option_set->osid);
  // Advanced form expects the label within its options.
  if (isset($values['content'])) {
    $option_label = isset($values['content']['title']) ? $values['content']['title'] : $values['content']['label'];
    $option_set->label = $option_label;
    $values['advanced']['label'] = $option_label;
    foreach ($option_set->options as &$option) {
      if (isset($values['content']['options'][$option['option_id']]['option_label'])) {
        $option['option_label'] = $values['content']['options'][$option['option_id']]['option_label'];
      }
    }
  }
  personalize_campaign_wizard_submit_variations_advanced($values, $option_set);
  personalize_option_set_save($option_set);
}

/**
 * Submit function to submit a single new block variation.
 */
function acquia_lift_personalize_campaign_wizard_variations_single_submit($form, $form_state) {
  // Triggering submit name is {type}_{existing}_{delta} where
  // {type} is the option set plugin
  // {existing} is 'new' or 'existing'
  // {delta} is the option set id for an existing edit or the index for a new
  list($type, $existing, $delta) = explode('_', $form_state['triggering_element']['#name']);
  $option_set = $existing == 'existing' ? $form_state['values']['variations']['editing']['option_sets']['option_set_' . $delta]['option_set'] : NULL;
  switch ($type) {
    case 'block':
      if ($existing == 'new') {
        _acquia_lift_personalize_campaign_wizard_block_single_save($form_state['values']['variations']['editing']['new'][$delta]['block']['content'], NULL, $form_state);
      }
      else {
        _acquia_lift_personalize_campaign_wizard_block_single_save($form_state['values']['variations']['editing']['option_sets']['option_set_' . $delta]['content'], $option_set, $form_state);
      }
      break;
    case 'elements':
      // Only existing elements can be saved this way.
      _acquia_lift_personalize_campaign_wizard_element_single_save($form_state['values']['variations']['editing']['option_sets']['option_set_' . $delta], $option_set);
  }
  if ($existing == 'existing') {
    _acquia_lift_personalize_campaign_wizard_variations_single_save_general($form_state['values']['variations']['editing']['option_sets']['option_set_' . $delta], $option_set);
  }
}

/**
 * Submit function for variations form.
 */
function acquia_lift_personalize_campaign_wizard_variations_submit(&$form, &$form_state, &$agent_data, $agent_instance) {
  if (!empty($form_state['storage']['acquia_lift_variation_set_handling'])) {
    $agent_data->data['variation_set_handling'] = $form_state['storage']['acquia_lift_variation_set_handling'];
    $agent_data = personalize_agent_save($agent_data);
  }

  // Save any existing option sets.
  if (!empty($form_state['values']['variations']['editing']['option_sets'])) {
    foreach ($form_state['values']['variations']['editing']['option_sets'] as $option_set_values) {
      $option_set = $option_set_values['option_set'];
      switch ($option_set_values['option_set']->plugin) {
        case "block":
          _acquia_lift_personalize_campaign_wizard_block_single_save($option_set_values['content'], $option_set, $form_state);
          break;
        case "elements":
          _acquia_lift_personalize_campaign_wizard_element_single_save($option_set_values, $option_set);
          break;
        default:
          // Unsupported option set type.
      }
      // Save general and advanced option set data.
      _acquia_lift_personalize_campaign_wizard_variations_single_save_general($option_set_values, $option_set);
    }
  }

  // Create any new option sets.
  if (!empty($form_state['new_option_sets'])) {
    foreach($form_state['new_option_sets'] as $delta => $type) {
      if ($type !== 'block') {
        continue;
      }
      _acquia_lift_personalize_campaign_wizard_block_single_save($form_state['values']['variations']['editing']['new'][$delta]['block']['content'], NULL, $form_state);
    }
  }
}

/**
 * Submit handler for entering a URL to begin creation an element variation.
 */
function acquia_lift_personalize_campaign_wizard_variations_submit_element_add($form, &$form_state) {
  // If this is the variation handling has been set in this process, save it
  // to the campaign before redirecting.
  if (isset($form_state['storage']['acquia_lift_variation_set_handling'])) {
    $agent_data = $form['#agent'];
    $agent_data->data['variation_set_handling'] = $form_state['storage']['acquia_lift_variation_set_handling'];
    personalize_agent_save($agent_data);
  }
  // Set a session variable that we can check upon the next page load to
  // trigger element editing mode for element variations.
  $_SESSION['acquia_lift_element_trigger'] = true;
  // Set a session variable to use as a destination upon completion of element
  // creation.
  $agent = $form['#agent'];
  $_SESSION['acquia_lift_element_destination'] = url('admin/structure/personalize/manage/' . $agent->machine_name . '/variations', array('absolute' => TRUE));

  // Redirect to the requested URL to create an option set.
  $delta = _acquia_lift_personalize_campaign_wizard_next_element($form_state, 'new');
  $form_state['redirect'] = $form_state['values']['variations']['editing']['new'][$delta]['element']['content']['url'] ?: '<front>';

  // Run the form submit handler in case any other variations were added or
  // edited.
  acquia_lift_personalize_campaign_wizard_submit($form, $form_state);
}

/**
 * Helper submit function to handle saving a new goal.
 */
function _acquia_lift_personalize_campaign_wizard_goals_single_add($delta, $form, $form_state, $agent_data) {
  $type = $form_state['new_goals'][$delta];
  if (empty($type)) {
    return;
  }
  // Get a reference to the original values.
  $values = $form_state['values'];
  $form_state['values'] = $values['goals']['new'][$delta]['details'];
  $form_state['values']['agent'] = $agent_data;
  $form_state['values']['goal_type'] = $type;
  $save_form['goal']['action_name'] = ($type == 'existing') ? $form['goals']['new'][$delta]['details']['action_name'] : array();
  module_load_include('inc', 'acquia_lift', 'acquia_lift.admin.unibar');
  acquia_lift_goal_type_create_form_submit($save_form, $form_state);
  // Restore the form state values.
  $form_state['values'] = $values;
}

/**
 * Submit function for a single goal
 */
function acquia_lift_personalize_campaign_wizard_goals_single_submit($form, &$form_state) {
  $agent_data = $form['#agent'];
  module_load_include('inc', 'acquia_lift', 'acquia_lift.admin');
  if (!acquia_lift_target_definition_changes_allowed($agent_data)) {
    return;
  }
  list( , , $delta) = explode('_', $form_state['triggering_element']['#name']);
  _acquia_lift_personalize_campaign_wizard_goals_single_add($delta, $form, $form_state, $agent_data);
  unset($form_state['new_goals'][$delta]);
  unset($form_state['expanded_goal']);
  $form_state['rebuild'] = TRUE;
}

/**
 * Submit function for goals form.
 */
function acquia_lift_personalize_campaign_wizard_goals_submit(&$form, &$form_state, &$agent_data, $agent_instance) {
  if (!acquia_lift_target_definition_changes_allowed($agent_data)) {
    return;
  }

  // Check if any goals have been removed during editing.
  $saved_goals = $existing_goals = array();
  foreach (personalize_goal_load_by_conditions(array('agent' => $agent_data->machine_name)) as $goal) {
    // We can only have one goal with a given action per agent, so
    // we key the array of existing goals by action name.
    $existing_goals[$goal->action] = $goal->id;
  }

  // Check for any new goals.
  if (isset($form_state['new_goals'])) {
    foreach ($form_state['new_goals'] as $delta => $type) {
      _acquia_lift_personalize_campaign_wizard_goals_single_add($delta, $form, $form_state, $agent_data);
    }
  }

  // Update any existing goals.
  if (isset($form_state['values']['goals']['all_goals'])) {
    foreach ($form_state['values']['goals']['all_goals'] as $goal) {
      if (!empty($goal['action_name'])) {
        try {
          personalize_goal_save($agent_data->machine_name, $goal['action_name'], $goal['value'], $goal['goal_id']);
          $saved_goals[$goal['action_name']] = $goal['goal_id'];
        }
        catch (Exception $e) {
          drupal_set_message($e->getMessage(), 'error');
        }
      }
    }
  }

  // The difference between existing goals and saved goals are the ones
  // that need to be deleted.
  $to_delete = array_diff_key($existing_goals, $saved_goals);
  foreach ($to_delete as $goal_id) {
    personalize_goal_delete($goal_id);
  }
}

/**
 * Submit handler for entering a URL to begin creation of an element goal.
 */
function acquia_lift_personalize_campaign_wizard_goals_submit_element_add($form, &$form_state) {
  $agent_data = $form['#agent'];

  // Redirect to the requested URL to create an option set.
  $delta = _acquia_lift_personalize_campaign_wizard_next_element($form_state, 'new');
  $_SESSION['visitor_actions_ui_edit_mode'] = 1;
  $_SESSION['visitor_actions_ui_edit_mode_return'] = 'admin/structure/personalize/manage/' . $agent_data->machine_name . '/goals';

  // Run the form submit handler in case any other variations were added or
  // edited.
  acquia_lift_personalize_campaign_wizard_submit($form, $form_state);

  $element_page = $form_state['values']['goals']['new'][$delta]['details']['url'] ?: '<front>';
  drupal_goto($element_page);
}

/**
 * Submit handler to delete a goal.
 */
function acquia_lift_personalize_campaign_wizard_goals_submit_delete($form, &$form_state) {
  $goal_id = $form_state['triggering_element']['#attributes']['data-acquia-lift-personalize-goal-id'];
  if (!empty($goal_id)) {
    personalize_goal_delete($goal_id);
    $form_state['num_goals']--;
  }
}

/**
 * Submit function for targeting form.
 */
function acquia_lift_personalize_campaign_wizard_targeting_submit(&$form, &$form_state, &$agent_data, $agent_instance) {
  if ($form_state['triggering_element']['#value'] == t('Revert changes')) {
    $form_state['redirect'] = array(
      'admin/structure/personalize/manage/' . $agent_data->machine_name . '/targeting/revert',
      array(
        'query' => array(
          'destination' => 'admin/structure/personalize/manage/' . $agent_data->machine_name . '/targeting',
        ),
      ),
    );
    return;
  }

  // Handle targeting audiences.
  if (empty($form_state['values']['audiences'])) {
    return;
  }
  module_load_include('inc', 'acquia_lift', 'acquia_lift.admin');
  $targeting = array();
  $audience_count = 0;
  // Determine the values for the "Everyone else" audience
  $final_audience = _acquia_lift_personalize_campaign_wizard_everyone_else_audience();

  // Set the weight higher than any of the audience options.
  foreach($form_state['values']['audiences'] as $type_id => $type) {
    foreach ($form_state['values']['audiences'][$type_id] as $audience_id => $audience) {
      if ($audience_id === ACQUIA_LIFT_TARGETING_EVERYONE_ELSE || !isset($audience['details']['weight']) || empty($audience['details']['name'])) {
        continue;
      }
      $final_audience['weight'] = $audience['details']['weight'] > $final_audience['weight'] ? $audience['details']['weight'] : $final_audience['weight'];
    }
  }
  $final_audience['weight'] += 10;
  // Now process all of the audiences.
  $has_tests = FALSE;
  foreach ($form_state['values']['audiences'] as $type_id => $type) {
    foreach ($form_state['values']['audiences'][$type_id] as $audience_id => $audience) {
      if (empty($audience['details']['name']) && $audience_id !== ACQUIA_LIFT_TARGETING_EVERYONE_ELSE) {
        continue;
      }

      if ($audience_id === ACQUIA_LIFT_TARGETING_EVERYONE_ELSE) {
        $audience_values = $final_audience;
      }
      else {
        $audience_values = array(
          'name' => $audience['details']['name'],
          'weight' => $audience['details']['weight'],
          'contexts' => array(),
          'strategy' => $audience['details']['strategy'],
          'id' => $audience_id === 'add' ? NULL : $audience_id,
        );
        // We need to massage the context information as submitted in the form into
        // an array of contexts that can be consumed by the
        // acquia_lift_target_audience_save() function.
        foreach ($audience['details']['mapping']['contexts'] as $context_values) {
          if ($context_values['context'] == '') {
            continue;
          }
          $context_values['match'] = $context_values['value']['match'];
          $context_values['operator'] = $context_values['value']['operator'];
          unset($context_values['value'], $context_values['remove']);
          $audience_values['contexts'][] = $context_values;
        }
      }
      if (acquia_lift_target_audience_save($audience_values['name'], $agent_data->machine_name, $audience_values['contexts'], $audience_values['strategy'], $audience_values['weight'], $audience_values['id'])) {
        if ($type_id === 'new') {
          drupal_set_message(t('The target audience %audience was created successfully', array(
            '%audience' => $audience['details']['name'],
          )));
        }
        $audience_count++;
      }
      else {
        drupal_set_message(t('There was a problem saving the target audience %audience.', array(
          '%audience' => $audience['details']['name'],
        )), 'error');
      }
      // Handle the assignment of options to audiences.
      if (!empty($audience['assignment_order'])) {
        $targeting[$audience_id] = explode(',', $audience['assignment_order']);
        $has_tests = $has_tests || count($targeting[$audience_id]) > 1;
      }
    }
  }
  acquia_lift_save_targeting_structure($agent_data, $targeting);

  // Save advanced test settings if applicable.
  if ($has_tests) {
    // Save the test options on the main targeting agent.  When the agent creates
    // embedded tests then it will read these properties and set them on each
    // test agent.
    if (isset($form_state['values']['decision_style'])) {
      $agent_data->data['decision_style'] = $form_state['values']['decision_style'];
    }
    if (isset($form_state['values']['control_rate'])) {
      $agent_data->data['control_rate'] = $form_state['values']['control_rate'];
    }
    if (isset($form_state['values']['explore_rate'])) {
      $agent_data->data['explore_rate'] = $form_state['values']['explore_rate'];
    }
  }
}

/**
 * Submit function for the review page.
 */
function acquia_lift_personalize_campaign_wizard_review_submit(&$form, &$form_state, &$agent_data, $agent_instance) {
  // If the "Start" button was pressed then we sync everything to Lift and start
  // the campaign.
  if (isset($form_state['triggering_element']['#personalize_next_status'])) {
    $next_status = $form_state['triggering_element']['#personalize_next_status'];
    module_load_include('inc', 'acquia_lift', 'acquia_lift.admin');
    if (!acquia_lift_target_set_status($agent_data, $next_status)) {
      form_set_error(NULL, t('There was a problem implementing the personalization as defined.'));
    }
    $form_state['redirect'] = 'admin/structure/personalize/manage/' . $agent_data->machine_name . '/review';
  }
}

/**
 * Submit function for the status change buttons.
 */
function acquia_lift_personalize_campaign_wizard_status_submit($form, &$form_state) {
  $next_status = $form_state['triggering_element']['#personalize_next_status'];
  $agent_data = personalize_agent_load($form_state['values']['agent_name']);
  module_load_include('inc', 'acquia_lift', 'acquia_lift.admin');
  if (!acquia_lift_target_set_status($agent_data, $next_status)) {
    drupal_set_message(t('There was a problem implementing the personalization as defined.  Please check the <a href="!review_url">review section</a> for more details.',array(
      '!review_url' => url('admin/structure/personalize/manage/' . $agent_data->machine_name . '/review'),
    )), 'error');
  }
}

/**
 ********************************************************************
 *
 * H E L P E R S
 *
 ********************************************************************
 */

/**
 * Retrieve the next nested element's name.
 *
 * Useful for when the element name is a dynamic counter and locatable based
 * on its nesting within the parent structure.
 *
 * @param array $form_state
 *   The current form state to retrieve parents
 * @param string $previous
 *   The previous element name.
 * @param array $parents
 *   An option array of parents.  If not provided, the triggering_element's
 *   parents will be used.
 * @return string
 *   The next nested element's name.
 */
function _acquia_lift_personalize_campaign_wizard_next_element($form_state, $previous, $parents = NULL) {
  if (!is_array($parents)) {
    if (!isset($form_state['triggering_element'])) {
      return '';
    }
    $parents = $form_state['triggering_element']['#array_parents'];
  }
  if (($delta_pos = array_search($previous, $parents, TRUE)) === FALSE) {
    return '';
  }
  $delta_pos++;
  if (count($parents) < $delta_pos) {
    return '';
  }
  return $parents[$delta_pos];
}

/**
 * Generates a multidimensional array of variation display labels based on the
 * option sets and type of multiple variation set handling.
 *
 * Full option array can also be returned by specifying $labels_only = FALSE.
 * In this case, a boolean targeting key will be added to options with
 * lock step handling to indicate which options come from the option set
 * used for targeting.
 *
 * @param array $option_sets
 *   The option sets to be converted into a variation display array
 * @param $handling
 *   The type of variation handling to use for multiple option sets.
 * @param $labels_only
 *   If true, then only the option labels will be returned, otherwise, the
 *   full option array will be returned in the arrays.
 * @param $empty_indicator
 *   The string to add when a variation is missing in a lock-step combination.
 *
 * @todo: Implement MVT handling.
 */
function _acquia_lift_personalize_campaign_wizard_variation_displays($option_sets, $handling = ACQUIA_LIFT_DECISION_LOCKSTEP, $labels_only = TRUE, $empty_indicator = '-') {
  $all_variations = array();
  // Display for zero variations.
  if (empty($option_sets)) {
    return $all_variations;
  }
  $first_os = reset($option_sets);
  // Handle display for a single variation set.
  if (count($option_sets) == 1) {
    foreach($first_os->options as $delta => $option) {
      $all_variations[$delta][0] = $labels_only ? theme('acquia_lift_single_variation', array('option' => $option['option_label'])) : $option;
    }
    return $all_variations;
  }
  // Specialty handling for multivariate tests.
  if ($handling === ACQUIA_LIFT_DECISION_MULTIVARIATE) {
    // @todo: Produce a cartesion product of all variation options.
    return $all_variations;
  }
  // Default multiple handling is lock-step variation handling.
  if (!$labels_only) {
    $targeting_os = acquia_lift_get_option_set_for_targeting($first_os->agent);
  }
  $set_delta = 0;
  $max_variations = 0;
  foreach ($option_sets as $osid => $option_set) {
    $delta = 0;
    foreach($option_set->options as $option) {
      if (!$labels_only) {
        $option['targeting'] = ($osid == $targeting_os->osid);
        $option['option_set_label'] = $option_set->label;
      }
      $all_variations[$delta][$set_delta] = $labels_only ? theme('acquia_lift_single_variation', array('option_set' => $option_set->label, 'option' => $option['option_label'])) : $option;
      $max_variations = max($max_variations, count($all_variations[$delta]));
      $delta++;
    }
    $set_delta++;
  }

  // Now fill the empty variations indexes and re-order so that
  // the missing variations are obvious.
  $fillEmptyVariations = function(&$variations, $max_variations) use ($empty_indicator) {
    for ($i = 0; $i < $max_variations; $i++) {
      if (!isset($variations[$i])) {
        $variations[$i] = $empty_indicator;
      }
    }
    ksort($variations);
  };
  foreach ($all_variations as &$variations) {
    $fillEmptyVariations($variations, $max_variations);
  }
  return $all_variations;
}

/**
 * Generate the blocks portion of the campaign wizard form.
 *
 * @param array $form
 *   The form array reference
 * @param array $form_state
 *   The current form state
 * @param stdClass $agent_data
 *   The data for the current campaign
 * @param array $parents
 *   The array of parents for this element
 * @param stdClass $option_set
 *   The option set class that should be shown within the form
 * @return array
 *   The form section for the requestd block
 */
function _acquia_lift_personalize_campaign_wizard_variations_block(&$form, &$form_state, $agent_data, $parents, $option_set = NULL) {
  $element = array();
  $values = isset($form_state['values']) ? $form_state['values'] : array();

  if (empty($option_set->osid)) {
    if (isset($form_state['values'])) {
      $block_content = drupal_array_get_nested_value($form_state['values'], $parents);
      $form_state_key = 'add_' . _acquia_lift_personalize_campaign_wizard_next_element($form_state, 'new', $parents);
    }
    else {
      $block_content = array();
      $form_state_key = 'add_0';
    }
  } else {
    $block_content = empty($values['variations']['editing']['option_sets']['option_set_' . $option_set->osid]['content']) ? array() : $values['variations']['editing']['option_sets']['option_set_' . $option_set->osid]['content'];
    $form_state_key = $option_set->osid;
  }

  module_load_include('inc', 'personalize_blocks', 'personalize_blocks.admin');
  module_load_include('inc', 'acquia_lift', 'acquia_lift.ui');
  module_load_include('inc', 'acquia_lift', 'acquia_lift.admin');

  $changes_enabled = empty($option_set) || acquia_lift_target_definition_changes_allowed($agent_data);

  _acquia_lift_personalize_campaign_wizard_form_state_blocks_alter($form_state, $block_content, $form_state_key, $option_set);
  $element = personalize_blocks_form($element, $form_state, 'embed', $option_set);
  // This will add the current number of blocks and the personalized block
  // into the form state.  We move it into a more structured format to support
  // multiple blocks forms per page.
  $form_state['option_set_num_blocks'][$form_state_key] = $form_state['num_blocks'];
  $form_state['option_set_pblock'][$form_state_key] = $form_state['pblock'];
  // Clear the to_remove from the main option set keys
  unset($form_state['option_set_to_remove'][$form_state_key]);
  // Set the form state values back.
  $form_state['values'] = $values;

  // The agent cannot be changed.
  $element['personalize']['#access'] = FALSE;
  $element['agent_select'] = array(
    '#type' => 'value',
    '#value' => $agent_data->machine_name,
  );
  // Add help text to the title.
  $element['title']['#attributes']['placeholder'] = t('Example: Front page offers');
  // Make the variations wrapper a plain container rather than a fieldset.
  $element['pblock_wrapper']['#theme_wrappers'] = array('container');
  // Adjust the blocks container for our own user interface.
  unset($element['pblock_wrapper']['blocks']['#theme']);
  unset($element['pblock_wrapper']['#tree']);
  unset($element['pblock_wrapper']['blocks']['#tree']);

  $pblock_id = drupal_html_id('edit-' . implode('-', $parents) . '-pblock-wrapper');
  // Remove any duplicate indicator that Drupal may have added from seeing this
  // id generated more than once. This can happen in the AJAX callback.
  $dup_re = "/\\-\\-[a-z0-9]+$/";
  $pblock_id = preg_replace($dup_re, '', $pblock_id);
  $more_id = $pblock_id . '-blocks-more';
  $blocks_id = $pblock_id . '-blocks';
  // Need to update the id so that it is unique on the page.
  $element['pblock_wrapper']['blocks']['#attributes']['id'] = $blocks_id;
  $element['pblock_wrapper']['blocks']['#attributes']['class'][] = 'acquia-lift-personalize-blocks';

  $pblock_blocks = element_children($element['pblock_wrapper']['blocks']);
  $placeholder_help = array(
    t('Example: Winter vacation'),
    t('Example: Island retreat'),
  );
  foreach ($pblock_blocks as $i => $key) {
    $block = &$element['pblock_wrapper']['blocks'][$key];
    $block['block']['#attributes']['class'][] = 'acquia-lift-personalize-block-wrapper';
    $option_id = isset($block['option_id']['#value']) ? $block['option_id']['#value'] : '';

    // Set the option label as a revealing input and give it a label.
    if ($option_set && !empty($option_id)) {
      $block['option_label']['#theme_wrappers'][] = 'acquia_lift_revealing_input';
      $block['option_label']['#title'] = _personalize_generate_option_index($key);
      $block['option_label']['#fix'] = FALSE;
    }
    else {
      // Do not allow automatically generated option labels.
      $block['option_label']['#default_value'] = '';
      $block['option_label']['#title'] = t('Variation title') . ' <span class="form-required" title="This field is required.">*</span>';
      unset($block['option_label']['#size']);
      if (isset($placeholder_help[$i])) {
        $block['option_label']['#attributes']['placeholder'] = $placeholder_help[$i];
      }
    }
    if (!empty($option_set->preview_link) && !empty($option_id)) {
      $block['preview'] = array(
        '#markup' => '<div class="acquia-lift-preview clearfix">' . l(t('Preview variation'), $option_set->preview_link, array(
            'attributes' => array('target' => 'preview'),
            'query' => array(PERSONALIZE_PRESELECTION_PARAM => personalize_stringify_osid($option_set->osid) . '--' . $option_id),
          )) . '</div>',
      );
    }
    // Weights are not manipulated here.
    $block['weight']['#access'] = FALSE;

    // Set the order for inputs within a block option.
    $block['remove']['#weight'] = 10;
    $block['option_label']['#weight'] = 20;
    $block['preview']['#weight'] = 30;
    $block['block']['#weight'] = 40;

    // Block type selection states
    $select_class = 'select-' . $pblock_id . '-' . $key;
    $block['block']['block_type']['#attributes']['class'][] = $select_class;
    $block['block']['bid']['#states']['visible'] = array(
      ':input.' . $select_class => array('value' => 'select'),
    );

    $block['block']['add']['#title'] = '';
    $block['block']['add']['#states']['visible'] = array(
      ':input.' . $select_class => array('value' => 'add'),
    );

    acquia_lift_chosenify_element($block['block']['bid'], array('acquia-lift-chosen-select-third'));

    // Hide the remove button if the option is currently targeted in a running
    // campaign.
    if (!empty($option_id) && !$changes_enabled && acquia_lift_target_option_targeted($agent_data->machine_name, $option_id)) {
      $block['remove']['#access'] = FALSE;
    }
    if (count($pblock_blocks) <= 2) {
      $block['remove']['#access'] = FALSE;
    }

    // If the block still allows removal, then adjust the callbacks based on
    // whether it is an option that requires deletion or just an AJAX
    // re-display of the form.
    if (!isset($block['remove']['#access']) || $block['remove']['#access'] == TRUE) {
      if (!empty($option_id)) {
        // If the option is an existing one, then redirect to a confirmation form
        // prior to deleting.
        $remove_url = "admin/structure/personalize/variations/personalize-blocks/manage/{$option_set->osid}/{$option_id}/delete";
        $block['remove'] = array(
          '#markup' => l(t('Delete'), $remove_url, array(
            'attributes' => array(
              'class' => array('acquia-lift-delete'),
            ),
            'query' => array('destination' => 'admin/structure/personalize/manage/' . $agent_data->machine_name . '/variations'),
          )),
        );
      }
      else {
        // If the block is a new option that has never been saved, then just
        // allow a quick AJAX adjustment for the number to show.
        $block['remove']['#attributes']['id'] = $blocks_id . '-' . $key . '-remove';
        $block['remove']['#attributes']['class'][] = 'acquia-lift-delete';
        $block['remove']['#submit'] = array('acquia_lift_personalize_campaign_wizard_blocks_remove');
        $block['remove']['#ajax'] = array(
          'callback' => 'acquia_lift_personalize_campaign_wizard_blocks_ajax',
          'wrapper' => $blocks_id,
          'effect' => 'fade',
        );
        $block['remove']['#value'] .= '_' . (empty($option_set) ? 'add' : $option_set->osid);
      }
    }
  }

  // Update the "Add another" button to match the desired styling and language.
  $element['pblock_wrapper']['#attributes']['id'] = $pblock_id;
  $element['pblock_wrapper']['blocks_more']['#tag'] = 'button';
  $element['pblock_wrapper']['blocks_more']['#text'] = t('Add variation');
  $element['pblock_wrapper']['blocks_more']['#value'] = 'blocks_' . (empty($option_set) ? 'add' : $option_set->osid);
  $element['pblock_wrapper']['blocks_more']['#theme_wrappers'] = array('personalize_html_tag');
  $element['pblock_wrapper']['blocks_more']['#attributes']['id'] = $more_id;
  $element['pblock_wrapper']['blocks_more']['#attributes']['class'][] = 'personalize-add-link';
  $element['pblock_wrapper']['blocks_more']['#attributes']['name'] = 'blocks_add_variation_' . $form_state_key;
  $element['pblock_wrapper']['blocks_more']['#prefix'] = '<span class="personalize-add-link-prefix"></span>';
  $element['pblock_wrapper']['blocks_more']['#submit'] = array('acquia_lift_personalize_campaign_wizard_blocks_add');
  $element['pblock_wrapper']['blocks_more']['#ajax'] = array(
    'callback' => 'acquia_lift_personalize_campaign_wizard_blocks_ajax',
    'wrapper' => $blocks_id,
    'effect' => 'fade',
  );
  // Remove the form's action buttons
  unset($element['actions']);
  return $element;
}

/**
 * Adds values to form_state as expected by personalize blocks form handlers.
 *
 * @param $form_state
 *   The form state to update (by reference).
 * @param $block_content
 *   The array that is the root of the blocks content values.
 * @param $key
 *   The key used for this particular option set in the form state.
 * @param $option_set
 *   The currently referenced option set data.
 */
function _acquia_lift_personalize_campaign_wizard_form_state_blocks_alter(&$form_state, $block_content = array(), $key, $option_set = NULL) {
  if (!empty($block_content)) {
    $form_state['values'] = $block_content;
    if (!empty($block_content['pblock_wrapper']['blocks'])) {
      $form_state['values']['blocks'] = $block_content['pblock_wrapper']['blocks'];
    }
  }
  else {
    unset($form_state['values']);
  }
  if (!empty($form_state['values']['blocks'])) {
    foreach ($form_state['values']['blocks'] as $delta => $block_info) {
      // Set the "add" indicator if the user is creating a new block.
      if (!empty($block_info['block']['add']['info']) && !empty($block_info['block']['add']['body']['value'])) {
        $form_state['values']['blocks'][$delta]['block']['bid'] = 'add';
        unset($form_state['values']['blocks'][$delta]['osid']);
      }
    }
  }

  if (!empty($form_state['option_set_num_blocks'][$key])) {
    $form_state['num_blocks'] = $form_state['option_set_num_blocks'][$key];
  }
  else {
    unset($form_state['num_blocks']);
  }
  if (!empty($form_state['option_set_pblock'][$key])) {
    $form_state['pblock'] = $form_state['option_set_pblock'][$key];
  }
  else {
    unset($form_state['pblock']);
  }
  if (isset($form_state['option_set_to_remove'][$key])) {
    $form_state['to_remove'] = $form_state['option_set_to_remove'][$key];
  }
  else {
    unset($form_state['to_remove']);
  }
}

/**
 * Form to edit or add an element variation.
 *
 * @param array $form
 *   The form array reference
 * @param array $form_state
 *   The current form state
 * @param stdClass $agent_data
 *   The data for the current campaign
 * @param stdClass $option_set
 *   (Optional) The option set to display within the form if editing.
 * @return array
 *   The form element for the requested element variation set.
 *
 * NOTE: limit_validation_errors is not set for either of this submit buttons
 * due to a requirement to actually save the page content before exiting for
 * element variation actions.
 */
function _acquia_lift_personalize_campaign_wizard_variations_element(&$form, &$form_state, $agent_data, $option_set = NULL) {
  global $base_url;
  $element = array();
  if (empty($option_set->osid)) {
    $element['url'] = array(
      '#type' => 'textfield',
      '#element_validate' => array('personalize_form_element_path_validate'),
      '#title' => t('Page where element exists'),
      '#allow_dynamic' => FALSE,
      '#allow_external' => TRUE,
      '#field_prefix' => $base_url . '/',
    );
    $element['navigate'] = array(
      '#type' => 'submit',
      '#submit' => array('acquia_lift_personalize_campaign_wizard_variations_submit_element_add'),
      '#value' => t('Go'),
    );
    return $element;
  }
  // Form for an existing option set.
  $delete_links = array();
  foreach ($option_set->options as $option) {
    $delete_links[$option['option_id']] = "admin/structure/personalize/variations/personalize-elements/manage/{$option_set->osid}/{$option['option_id']}/delete";
  }
  $element = _acquia_lift_personalize_campaign_wizard_variations_general_edit($agent_data, $option_set, $delete_links);

  if ($option_set->data['personalize_elements_type'] == 'runJS') {
    // Javascript changes can't be edited on the page they were created but
    // only within the form.
    // @todo change this to following the same gear icon UX convention used
    // for editing visitor actions.
    $element['edit'] = array(
      '#type' => 'link',
      '#href' => 'admin/structure/personalize/variations/personalize-elements/manage/' . $option_set->osid . '/edit',
      '#title' => t('Edit variations'),
      '#options' => array(
        'query' => array('destination' => 'admin/structure/personalize/manage/' . $option_set->agent . '/variations'),
      ),
    );
  }
  else if (!empty($option_set->preview_link)) {
    // Note that utilizing the personalize_html_tag theme here somehow prevents
    // the element and form-altered submit handlers from being called.
    $element['edit'] = array(
      '#type' => 'link',
      '#href' => $option_set->preview_link,
      '#title' => t('Edit variations in context'),
    );
  }
  return $element;
}

/**
 * General markup to show the basic editing for an existing option set.
 *
 * This applies to all variation sets outside of blocks.
 *
 * @param stdClass $agent_data
 *   The data for the personalization for this option set.
 * @param stdClass $option_set
 *   The option set to edit.
 * @param array $delete_links
 *   An array of links to delete options within the set keyed by the option id.
 *   If there is no link for the option, then the option cannot be deleted.
 * @return array
 *   The form render structure.
 */
function _acquia_lift_personalize_campaign_wizard_variations_general_edit($agent_data, $option_set, $delete_links = array()) {
  $element = array();
  $editable = acquia_lift_target_definition_changes_allowed($agent_data);
  $element['label'] = array(
    '#type' => 'textfield',
    '#required' => TRUE,
    '#default_value' => $option_set->label,
    '#title' => t('Variation set')
  );
  $show_delete = $editable && count($option_set->options) > 2;
  foreach ($option_set->options as $option) {
    $option_id = $option['option_id'];
    $element['options'][$option_id] = array(
      '#type' => 'container',
      '#attributes' => array(
        'class' => array('acquia-lift-element-variations'),
      ),
    );
    if ($show_delete && !empty($delete_links[$option_id])) {
      $element['options'][$option_id]['remove'] = array(
        '#markup' => l(t('Delete'), $delete_links[$option_id], array(
          'attributes' => array(
            'class' => array('acquia-lift-delete'),
          ),
          'query' => array('destination' => 'admin/structure/personalize/manage/' . $agent_data->machine_name . '/variations'),
        )),
      );
    }
    $element['options'][$option_id]['option_label'] = array(
      '#type' => 'textfield',
      '#required' => TRUE,
      '#default_value' => $option['option_label'],
      '#title' => t('Variation'),
      '#theme_wrappers' => array('acquia_lift_revealing_input'),
    );
    if (!empty($option_set->preview_link)) {
      $element['options'][$option_id]['preview'] = array(
        '#markup' => '<div class="acquia-lift-preview">' . l(t('Preview variation'), $option_set->preview_link, array(
            'attributes' => array('target' => 'preview'),
            'query' => array(PERSONALIZE_PRESELECTION_PARAM => personalize_stringify_osid($option_set->osid) . '--' . $option['option_id']),
          )) . '</div>',
      );
    }
  };
  return $element;
}

/**
 * Form to edit a personalize fields variation.
 *
 * This provides a brief display of each field and the link to edit within the
 * context of the parent entity.  There is no "add" version of this as fields
 * are created within the context of the parent entity.
 *
 * @param array $form
 *   The form array reference
 * @param array $form_state
 *   The current form state
 * @param stdClass $agent_data
 *   The data for the current campaign
 * @param stdClass $option_set
 *   The option set to display within the form editing.
 * @return array
 *   The form element for the requested element variation set.
 *
 * NOTE: limit_validation_errors is not set for submit buttons due to a
 * requirement to actually save the page content before exiting for editing.
 */
function _acquia_lift_personalize_campaign_wizard_variations_fields(&$form, &$form_state, $agent_data, $option_set) {
  $element = _acquia_lift_personalize_campaign_wizard_variations_general_edit($agent_data, $option_set);

  // Load the entity's field display for the personalized field.
  try {
    $entities = entity_load($option_set->field_info['entity_type'], array($option_set->field_info['entity_id']));
    $display = field_view_field($option_set->field_info['entity_type'], $entities[$option_set->field_info['entity_id']], $option_set->field_info['field_name']);

    // Customize the display based on the type of rendering to be better presented
    // in a small list.
    $options = array_values($option_set->options);
    foreach (element_children($display) as $delta => $id) {
      if (isset($display[$id]['#image_style'])) {
        $display[$id]['#image_style'] = 'thumbnail';
      }
      else {
        if (isset($display[$id]['#markup'])) {
          $display[$id]['#markup'] = drupal_truncate_bytes($display[$id]['#markup'], 150);
        }
      }
      $option = $options[$delta];
      $element['options'][$option['option_id']]['preview_display'] = array(
        '#markup' => '<div class="acquia-lift-preview">' . drupal_render($display[$id]) . '</div>',
      );
    }
    $field_title = !empty($display['#title']) ? $display['#title'] : $option_set->field_info['field_name'];
    $element['label']['#description'] = t('Variation set created from %field_name field', array('%field_name' => $field_title));
  }
  catch (Exception $e) {
    // There is something wrong with loading the entity's personalized fields.
    // Either the entity does not exist or we are unable to display in
    // traditional ways.
    // Log this message for reference but allow page processing to continue.
    watchdog('php', 'Cannot load entity for personalized fields display: ' . $e->getMessage(), WATCHDOG_ERROR);
  }

  $edit_link = personalize_fields_personalize_edit_link($option_set);
  $machine_name = $agent_data->machine_name;
  if (!empty($edit_link)) {
    $element['edit'] = array(
      '#type' => 'link',
      '#href' => $edit_link,
      '#options' => array(
        'query' => array(
          'destination' => "admin/structure/personalize/manage/${machine_name}/variations",
        ),
      ),
      '#title' => t('Edit variations in context'),
    );
  }
  return $element;
}

/**
 * Form to select a page for a new element goal.
 *
 * @return array
 *   The form element to enter a path to the page for an element goal.
 *
 * NOTE: limit_validation_errors is not set for either of this submit buttons
 * due to a requirement to actually save the page content before exiting for
 * element variation actions.
 */
function _acquia_lift_personalize_campaign_wizard_goal_element_form() {
  global $base_url;
  $form = array();

  $form['url'] = array(
    '#type' => 'textfield',
    '#element_validate' => array('personalize_form_element_path_validate'),
    '#title' => t('Page where element exists'),
    '#allow_dynamic' => FALSE,
    '#allow_external' => TRUE,
    '#field_prefix' => $base_url . '/',
  );
  $form['navigate'] = array(
    '#type' => 'submit',
    '#submit' => array('acquia_lift_personalize_campaign_wizard_goals_submit_element_add'),
    '#value' => t('Go'),
  );
  return $form;
}


/**
 * Helper function to generate a change type sub-section of a form.
 *
 * @param string $selected_label
 *   The string to show for the currently selected type.
 * @param array $change_options
 *   An array of options to be added to the change submit button.  This will
 *   generally include the #submit and #ajax properties.
 * @return array
 *   The render array for the sub-form.
 */
function _acquia_lift_personalize_campaign_wizard_change_type_form($selected_label, $change_options = array()) {
  $form = array();
  $form['change_type_container'] = array(
    '#type' => 'container',
    '#attributes' => array('class' => array('container-inline')),
  );
  $form['change_type_container']['display'] = array(
    '#type' => 'item',
    '#title' => t('Type'),
    '#markup' => $selected_label,
  );
  $form['change_type_container']['change'] = array(
    '#type' => 'submit',
    '#value' => t('Change type'),
    '#limit_validation_errors' => array(),
    '#attributes' => array(
      'class' => array('acquia-lift-link'),
    ),
  );
  $form['change_type_container']['change'] += $change_options;
  $form['change_type_container']['change']['#ajax']['progress'] = array(
    'message' => '',
    'type' => 'throbber',
  );
  return $form;
}

/**
 * Helper function to generate an audience sub-form.
 *
 * @param $agent_data
 *   The current agent data.
 * @param $parent_type
 *   Indicates if this is "new" or "existing" (container names used for
 *   audiences of those types).
 * @param $parent_identifier
 *   The identifier used for this audience within the form, either the
 *   machine_name or "add" for new audiences.
 * @param $audience
 *   (Optional) The existing audience.  This contains:
 *   - label: The name of the audience
 *   - weight: the weight of the audience as compared ot all audiences for the
 *     option set.
 *   - targeting_features: An array of feature strings (context/value)
 *   - targeting_rules: An array of rules including context and plugin
 *   - targeting_strategy: The strategy to use when combining contexts
 * @return array
 *   The form element
 */
function _acquia_lift_personalize_campaign_wizard_targeting_audience(&$form, &$form_state, $agent_data, $parent_type, $parent_identifier, $audience = NULL) {
// Build up a list of available context values for targeting.
  module_load_include('inc', 'personalize', 'personalize.admin');
  $form['#targeting_values'] = $targeting_values = personalize_get_targeting_options_for_agent($agent_data);
  // This is the portion of the form that will be replace when the "add new",
  // "remove context" links are clicked
  $main_wrapper_id = 'acquia-lift-targeting-' . $parent_type . '-' . $parent_identifier;

  // TRICKY: can't replace the id with a custom ID or the states won't work here.
  // Work-around is to add a prefix/suffix with the id we want to use for
  // AJAX replacement.
  // @see https://www.drupal.org/node/2138611
  $element = array(
    '#type' => 'container',
    '#prefix' => '<div id="' . $main_wrapper_id . '">',
    '#suffix' => '</div>',
  );
  $element['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Audience name'),
    '#default_value' => !empty($audience['label']) ? $audience['label'] : '',
    '#required' => !empty($audience),
    '#attributes' => array(
      'placeholder' => t('Example: Los Angeles 18-25 year olds'),
    ),
  );
  $element['weight'] = array(
    '#type' => 'textfield',
    '#title' => t('Weight'),
    '#default_value' => !empty($audience['weight']) ? $audience['weight'] : 50,
    '#size' => 3,
    '#element_validate' => array('element_validate_number'),
    '#attributes' => array(
      'class' => array('acquia-lift-sortable-weight'),
    ),
    '#access' => !empty($audience),
  );

  $element['mapping'] = array(
    '#tree' => TRUE,
    '#theme_wrappers' => array('container'),
  );
  $element['mapping']['contexts'] = array(
    '#type' => 'container',
    '#attributes' => array(
      'class' => array('acquia-lift-targeting'),
    ),
  );
  $element['mapping']['contexts']['title'] = array(
    '#markup' => theme('html_tag', array('element' => array(
      '#tag' => 'label',
      '#value' => t('Definition'),
    ))),
  );

  // Load from the existing form if passed first, otherwise, load from audience.
  $mappings = array();
  if (isset($form_state['values']['audiences'][$parent_type][$parent_identifier]['details']['mapping']['contexts'])) {
    foreach($form_state['values']['audiences'][$parent_type][$parent_identifier]['details']['mapping']['contexts'] as $delta => $context) {
      if (empty($context['context'])) {
        $plugin_name = $context_option = '';
      }
      else {
        list($plugin_name, $context_option) = explode(PERSONALIZE_TARGETING_ADMIN_SEPARATOR, $context['context']);
      }
      // Important: preserve the delta passed through the form as it is used
      // to determine the item to delete when "remove" is clicked.
      $mappings[$delta] = array(
        'plugin' => $plugin_name,
        'context' => $context_option,
        'operator' => $context['value']['operator'],
        'match' => $context['value']['match'],
      );
    }
  }
  else if (!empty($audience['targeting_features'])) {
    foreach ($audience['targeting_features'] as $feature) {
      if (isset($audience['targeting_rules'][$feature])) {
        $mappings[] = $audience['targeting_rules'][$feature];
      }
    }
  }
  // If the "Remove" button was clicked for a context for this audience, we
  // need to remove that context from the form.
  if (isset($form_state['to_remove_audience'][$parent_type][$parent_identifier])) {
    unset($mappings[$form_state['to_remove_audience'][$parent_type][$parent_identifier]]);
    unset($form_state['to_remove_audience'][$parent_type][$parent_identifier]);
    $form_state['num_contexts_audience'][$parent_type][$parent_identifier]--;
  }

  // Make sure there is at least an empty context.
  if (empty($mappings)) {
    $mappings[] = array(
      'context' => '',
      'operator' => 'equals',
      'match' => '',
      'plugin' => '',
    );
  }

  // If the "Add another" button was clicked, we need to add contexts to get up
  // to the number indicated.
  $num_contexts = count($mappings);
  if (isset($form_state['num_contexts_audience'][$parent_type][$parent_identifier]) && $form_state['num_contexts_audience'][$parent_type][$parent_identifier] > $num_contexts) {
    while ($num_contexts < $form_state['num_contexts_audience'][$parent_type][$parent_identifier]) {
      $mappings[] = array(
        'context' => '',
        'operator' => 'equals',
        'match' => '',
        'plugin' => '',
      );
      $num_contexts++;
    }
  }
  $form_state['num_contexts_audience'][$parent_type][$parent_identifier] = count($mappings);

  foreach ($mappings as $delta => $mapping) {
    $element['mapping']['contexts'][$delta] = personalize_explicit_targeting_mapping_element($mapping, $targeting_values, $parent_identifier . '-' . $delta);

    // Add a "remove" button for this context.
    // NOTE: ajax.js expects the ID of the element to match the element's name
    // even when a different selector is passed.
    $element['mapping']['contexts'][$delta]['remove'] = array(
      '#prefix' => '<div class="acquia-lift-remove-context">',
      '#suffix' => '</div>',
      '#type' => 'submit',
      '#tag' => 'button',
      '#text' => t('Remove'),
      '#value' => 'remove_' . $delta,
      '#theme_wrappers' => array('personalize_html_tag'),
      '#access' => count($mappings) > 1,
      '#name' => 'audiences[' . $parent_type . '][' . $parent_identifier . '][details][mapping][contexts][' . $delta . '][remove]',
      '#attributes' => array(
        'class' => array('personalize-delete-context', 'form-submit',),
        'title' => t('Delete this context.'),
        'id' => 'edit-audiences-' . $parent_type . '-' . $parent_identifier . '-details-mapping-contexts-' . $delta . '-remove',
      ),
      '#submit' => array('acquia_lift_personalize_campaign_wizard_targeting_remove'),
      '#skip_validation' => TRUE,
      '#ajax' => array(
        'callback' => 'acquia_lift_personalize_campaign_wizard_targeting_ajax',
        'wrapper' => $main_wrapper_id,
        'effect' => 'fade',
      ),
    );
  }
  // Create an "add new context" link.
  $element['add_new'] = array(
    '#prefix' => '<span class="personalize-add-link-prefix"></span>',
    '#type' => 'submit',
    '#tag' => 'button',
    '#text' => t('Add context'),
    '#value' => 'add_context_' . $parent_type . '_' . $parent_identifier,
    '#theme_wrappers' => array('personalize_html_tag'),
    '#submit' => array('acquia_lift_personalize_campaign_wizard_targeting_add'),
    '#name' => 'audiences[' . $parent_type . '][' . $parent_identifier . '][details][add_new]',
    '#attributes' => array(
      'class' => array('personalize-add-link'),
      'title' => t('Click here to add more contexts.'),
      'id' => 'edit-audiences-' . $parent_type . '-' . $parent_identifier . '-details-add-new'
    ),
    '#ajax' => array(
      'callback' => 'acquia_lift_personalize_campaign_wizard_targeting_ajax',
      'wrapper' => $main_wrapper_id,
      'effect' => 'fade',
    ),
    '#skip_validation' => TRUE,
  );
  $element['strategies'] = array(
    '#type' => 'container',
  );

  // Add radio buttons so the user can select how multiple features for an option
  // should be treated.
  $default_strategy = empty($audience['targeting_strategy']) ? 'OR' : $audience['targeting_strategy'];
  if (count($mappings) > 1) {
    $element['strategy'] = array(
      '#type' => 'select',
      '#multiple' => FALSE,
      '#field_prefix' => t('Visitor must have '),
      '#field_suffix' => t(' of the specified contexts'),
      '#description' => t('Choose how multiple contexts should be applied to options. Choose "any" if the rule should apply if the user has any of the contexts. Choose "all" if the rule should apply only if the user has all of the contexts.'),
      '#options' => array(
        'OR' => 'any',
        'AND' => 'all'
      ),
      '#default_value' => $default_strategy,
    );
  }
  else {
    $element['strategy'] = array(
      '#type' => 'value',
      '#value' => $default_strategy,
    );
  }
  return $element;
}

/**
 * Form to include for test settings to apply to all embedded tests.
 *
 * @param stdClass $agent_data
 *   The data for the current targeting agent.
 * @param array $element_parents
 *   The array of parents for this form element.
 * @return array
 *   The renderable form array for the test options.
 */
function _acquia_lift_personalize_campaign_wizard_settings_tests($agent_data, $element_parents) {
  $form = array();
  $form['#attached'] = array(
    'js' => array(
      drupal_get_path('module', 'acquia_lift') . '/js/acquia_lift.agent.admin.js',
    ),
  );

  $control_rate = isset($agent_data->data['control_rate']) ? $agent_data->data['control_rate'] : 0;
  $form['control_rate'] = array(
    '#type' => 'acquia_lift_percentage',
    '#title' => t('Exclusion Group'),
    '#field_suffix' => '%',
    '#size' => 3,
    '#description' => t('A fixed variation will be shown to those excluded from the test, by default the first variation in the set.'),
    '#default_value' => $control_rate,
    '#rest_title' => t('Test Group'),
    '#rest_description' => t('Personalized variations will be shown.'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $decision_style = isset($agent_data->data['decision_style']) ? $agent_data->data['decision_style'] : 'random';
  $form['decision_style'] = array(
    '#type' => 'radios',
    '#title' => t('Decision Style'),
    '#options' => array('random' => t('Test only'), 'adaptive' => t('Auto-personalize')),
    '#default_value' => $decision_style,
    '#title_display' => 'invisible',
  );
  $form['decision_style']['adaptive'] = array(
    '#description' => t('Adapts to users and chooses the best option over time.'),
  );
  $form['decision_style']['random'] = array(
    '#description' => t('Tests variations and reports results.'),
  );
  $explore_rate = isset($agent_data->data['explore_rate']) ? $agent_data->data['explore_rate'] : 20;
  $decision_style_parents = $element_parents;
  $decision_style_parents[] = 'decision_style';
  $decision_style_form_element = '';
  foreach ($decision_style_parents as $i => $parent_name) {
    $decision_style_form_element .= ($i ? '[' . $parent_name . ']' : $parent_name);
  }
  $form['explore_rate_container'] = array(
    '#type' => 'container',
    '#tree' => FALSE,
    '#states' => array(
      'visible' => array(
        ':input[name="' . $decision_style_form_element . '"]' => array('value' => 'adaptive'),
      ),
    ),
  );
  $form['explore_rate_container']['explore_rate'] = array(
    '#type' => 'acquia_lift_percentage',
    '#title' => t('Random Group'),
    '#field_suffix' => '%',
    '#description' => t('Variations will be shown randomly and tracked to adjust for false positives.'),
    '#size' => 3,
    '#default_value' => $explore_rate,
    '#rest_title' => t('Personalized Group'),
    '#rest_description' => t('The "best" variation will be shown for each visitor based on our algorithm.'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  return $form;
}

/**
 ********************************************************************
 *
 * R E L E V A N T  W O R K F L O W  F O R M S
 *
 ********************************************************************
 */

/**
 * Show a delete confirmation for an agent audience.
 */
function acquia_lift_target_audience_delete($form, $form_state, $agent, $audience_id) {
  module_load_include('inc', 'acquia_lift', 'acquia_lift.admin');
  $option_set = acquia_lift_get_option_set_for_targeting($agent->machine_name);
  if (empty($option_set->targeting[$audience_id])) {
    drupal_set_message(t('Could not find the audience %audience_id in %agent.', array(
        '%audience_id' => $audience_id,
        '%agent' => $agent->machine_name,
      ))
      , 'error');
    return array();
  }
  $form['audience_id'] = array(
    '#type' => 'value',
    '#value' => $audience_id,
  );
  $form['agent_name'] = array(
    '#type' => 'value',
    '#value' => $agent->machine_name,
  );
  $form['option_set'] = array(
    '#type' => 'value',
    '#value' => $option_set,
  );
  return confirm_form($form, t('Are you sure you want to delete %audience from %agent?', array(
    '%audience' => empty($option_set->targeting[$audience_id]['label']) ? $audience_id : $option_set->targeting[$audience_id]['label'],
    '%agent' => $agent->label,
  )), 'admin/structure/personalize/manage/' . $agent->machine_name . '/targeting', NULL, t('Delete'), t('Cancel'));
}

/**
 * Submit handler for agent audience delete submission.
 */
function acquia_lift_target_audience_delete_submit($form, &$form_state) {
  $option_set = $form_state['values']['option_set'];
  $audience_id = $form_state['values']['audience_id'];
  $audience_name = isset($option_set->targeting[$audience_id]['label']) ? $option_set->targeting[$audience_id]['label'] : $audience_id;
  // First remove any agent-level targeting structure for this audience.
  $agent = personalize_agent_load($option_set->agent);
  if (isset($agent->data['lift_targeting'][$audience_id])) {
    unset($agent->data['lift_targeting'][$audience_id]);
    personalize_agent_save($agent);
  }
  // Delete the audience definition along with any saved targeting.
  unset($option_set->targeting[$audience_id]);
  personalize_option_set_save($option_set);
  drupal_set_message(t('The target audience %name has been removed.', array('%name' => $audience_name)));
  $form_state['redirect'] = 'admin/structure/personalize/manage/' . $form_state['values']['agent_name'] . '/targeting';
}

/**
 * Confirm form for reverting the changes to targeting.
 *
 * @param stdClass $agent
 *   The agent data whose targeting changes are being reverted.
 */
function acquia_lift_confirm_revert_changes($form, &$form_state, $agent) {
  $form['agent'] = array(
    '#type' => 'value',
    '#value' => $agent->machine_name
  );
  $form['revert_changes'] = array('#type' => 'value', '#value' => TRUE);
  return confirm_form($form,
    t('Are you sure you want to revert the targeting changes for the %agent personalization?',
      array('%agent' => $agent->label)),
    'admin/structure/personalize/' . $agent->machine_name . '/targeting',
    t('Reverting the targeting will discard all changes since the personalization was set to running.'),
    t('Revert changes'),
    t('Cancel'));
}

/**
 * Submit callback for the "Revert changes" confirm form.
 */
function acquia_lift_confirm_revert_changes_submit($form, &$form_state) {
  if ($agent = personalize_agent_load($form_state['values']['agent'])) {
    $agent->data['lift_targeting'] = array();
    personalize_agent_save($agent);
  }
}
