<?php
/**
 * @file acquia_lift.admin.unibar.inc
 *
 * Contains the administrative functionality callbacks from the unified
 * navigation bar.
 */

/***************************************************************
 *
 * V A R I A T I O N  C R E A T E / E D I T
 *
 ***************************************************************/

/**
 * Form handler for an element variation details form.
 *
 * @param string $variation_type
 *   The variation type that defines the type of details to include.  This
 *   is a key defined as a personalize_elements variation.
 * @param string $osid
 *   (Optional) The option set id when adding variations to an existing option
 *   set.  If null, then create a new option set with the control and variation.
 * @param string $option_id
 *   (Optional) The option id for the specific variation to be edited.
 *
 * @see hook_personalize_elements_variation_types
 */
function acquia_lift_element_variation_details_form($form, &$form_state, $variation_type, $osid = NULL, $option_id = NULL) {
  module_load_include('inc', 'personalize_elements', 'personalize_elements.admin');
  $types = module_invoke_all('personalize_elements_variation_types');
  if (!isset($types[$variation_type]['contextual']['formitem'])) {
    $form['error'] = array(
      '#markup' => t('Invalid element variation type. Please close this message and create a new variation.'),
    );
    return $form;
  }
  $default_option_label = '';
  if (!empty($osid)) {
    $osid = personalize_resolve_stringified_osid($osid);
    $option_set = personalize_option_set_load($osid);
    $default_set_name = $option_set->label;
    $edit_option = NULL;
    if (!empty($option_id)) {
      if ($option_id === PERSONALIZE_CONTROL_OPTION_ID) {
        $form['error'] = array(
          '#markup' => t('The control variation cannot be edited.'),
        );
        return $form;
      }
      foreach ($option_set->options as $option) {
        if ($option['option_id'] === $option_id) {
          $edit_option = $option;
          $form['option'] = array(
            '#type' => 'value',
            '#value' => $edit_option,
          );
          break;
        }
      }
    }
    $default_option_label = !empty($edit_option['option_label']) ?  $edit_option['option_label'] : '';

    $form['osid'] = array(
      '#type' => 'value',
      '#value' => $osid,
    );
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
    '#access' => empty($osid),
  );
  $form['pages'] = array(
    '#type' => 'hidden',
  );
  $form['agent'] = array(
    '#type' => 'hidden',
  );
  // Option set title applicable to element variations.
  $form['title'] = array(
    '#type' => 'textfield',
    '#title' => t('Variation set name'),
    '#description' => t('Descriptive name for the variation set, used by Acquia Lift for administration and reporting.'),
    '#default_value' => isset($default_set_name) ? $default_set_name : '',
    '#required' => TRUE,
    '#maxlength' => 255,
    '#attributes' => array(
      'placeholder' => t('Example: Front page offers'),
    ),
  );
  // Option label
  $form['option_label'] = array(
    '#type' => 'textfield',
    '#title' => t('Variation name'),
    '#required' => TRUE,
    '#maxlength' => PERSONALIZE_MACHINE_NAME_MAXLENGTH,
    '#default_value' => personalize_sanitize_string($default_option_label),
    '#attributes' => array(
      'placeholder' => t('Example: Winter vacation'),
    ),
  );
  // Now include the details specific to this variation type.
  $form['personalize_elements_content'] = $types[$variation_type]['contextual']['formitem'];
  if (isset($edit_option)) {
    $form['personalize_elements_content']['#default_value'] = personalize_elements_filter_personalize_element_option($variation_type, $option['personalize_elements_content']);
    $form['variation_number']['#type'] = 'value';
    $form['variation_number']['#value'] = $edit_option['option_id'];
  }

  $form['actions'] = array(
    '#type' => 'actions',
  );
  // Form actions.
  // TRICKY: These must have ids that are different from our CTools loaded
  // modal forms or else their submit buttons will be processed via Drupal ajax
  // rather than the main URL callbacks leading to a 200 response with an
  // empty response body.
  $form['actions']['variation_type_submit_form_add'] = array(
    '#type' => 'submit',
    '#value' => t('Add another'),
    '#attributes' => array(
      'class' => array('action-item-primary-active', 'acquia-lift-submit-button'),
    ),
    '#ajax' => array(
      'callback' => 'acquia_lift_element_variation_details_form_ajax_callback',
      'wrapper' => 'acquia-lift-element-variation-details-form',
      'progress' => array(
        'message' => NULL,
        'type' => 'throbber',
      ),
    ),
  );
  $form['actions']['variation_type_submit_form'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
    '#attributes' => array(
      'class' => array('acquia-lift-submit-button'),
    ),
    '#ajax' => array(
      'callback' => 'acquia_lift_element_variation_details_form_ajax_callback',
      'wrapper' => 'acquia-lift-element-variation-details-form',
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
      'callback' => 'acquia_lift_element_variation_details_form_ajax_callback',
      'wrapper' => 'acquia-lift-element-variation-details-form',
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
 * Form validation handler for acquia_lift_element_variation_details_form.
 */
function acquia_lift_element_variation_details_form_validate($form, &$form_state) {
  module_load_include('inc', 'personalize_elements', 'personalize_elements.admin');
  if (!personalize_elements_selector_validate($form_state['values']['selector'])) {
    form_set_error('selector', t('The selector cannot contain tags.'));
  }
  $filtered = personalize_elements_filter_personalize_element_option($form_state['values']['variation_type'], $form_state['values']['personalize_elements_content']);
  if ($filtered !== $form_state['values']['personalize_elements_content']) {
    $form_state['values']['personalize_elements_content'] = $filtered;
    drupal_set_message(t('Disallowed HTML tags were removed from the variation.'), 'warning');
  }
}

/**
 * Form submit handler for acquia_lift_element_variation_details_form to create a
 * new personalize elements option set or edit and existing option.
 *
 * Calls the appropriate handler for the creation of the personalize elements
 * variation based on the type of campaign.
 */
function acquia_lift_element_variation_details_form_submit($form, &$form_state) {
  // Prepare the values to those expected by the personalize elements module.
  $values = $form_state['values'];

  if (empty($values['osid'])) {
    // Creating a new option set and a new option within it.
    $values['options'] = array(
      array(
        'option_label' => $values['option_label'],
        'personalize_elements_content' => $values['personalize_elements_content'],
      ),
    );
    $values['add_control_option'] = TRUE;
    $values['agent_select'] = $values['agent'];

    module_load_include('inc', 'personalize_elements', 'personalize_elements.admin');
    $option_set = personalize_elements_convert_form_values_to_option_set($values);
    // For new variation sets the pages value is set to the current page by
    // the JavaScript.
    $option_set->preview_link = $values['pages'];
    $message = t('The variation set has been created.');
  }
  else {
    $option_set = personalize_option_set_load($values['osid']);
    $option_set->label = $values['title'];
    if (empty($values['variation_number'])) {
      // Adding a variation to an existing option set.
      $option_set->options[] = array(
        'option_label' => $values['option_label'],
        'personalize_elements_content' => $values['personalize_elements_content'],
      );
      $message = t('The variation has been created.');
    }
    else {
      // Editing an existing option set option.
      foreach ($option_set->options as &$option) {
        if ($option['option_id'] === $values['variation_number']) {
          $option['personalize_elements_content'] = $values['personalize_elements_content'];
          $option['option_label'] = $values['option_label'];
          break;
        }
      }
      $message = t('The variation has been updated.');
    }
  }
  $option_set = personalize_option_set_save($option_set);
  // Now re-load the option set so that all hooks get run.
  // This is particularly important to set the selector property.
  $option_set = personalize_option_set_load($option_set->osid);
  // Save the updated option sets for use within the Ajax callback.
  $form_state['storage']['option_sets'] = array($option_set);
  $form_state['storage']['osid'] = $option_set->osid;
  // Determine the option set to preview after save.
  if (!empty($values['variation_number'])) {
    // Preview the edited variation.
    $form_state['storage']['option_id'] = $values['variation_number'];
  }
  else {
    // It was a new option, so preview the last one.
    $updated_options = array_values($option_set->options);
    $updated_option = end($updated_options);
    $form_state['storage']['option_id'] = $updated_option['option_id'];
  }
  $form_state['storage']['confirmation_message'] = $message;
}

/**
 * Responds to AJAX submission of a variation type details page.
 */
function acquia_lift_element_variation_details_form_ajax_callback($form, &$form_state) {
  $op = isset($form_state['values']['op']) ? $form_state['values']['op'] : '';
  $add_another = ($op == t('Add another'));
  $add_and_close = ($op == t('Save'));
  $cancel = ($op == t('Cancel'));
  $has_destination = !empty($_SESSION['acquia_lift_element_destination']);

  if ($has_destination) {
    ctools_include('ajax');
  }

  $commands = array();

  if ($add_another || $add_and_close) {
    $errors = form_get_errors();
    // If validation errors exist, return the form.
    if (!empty($errors)) {
      return $form;
    }

    $agent_name = $form_state['values']['agent'];
    if ($add_another || !$has_destination) {
      $settings = array();
      // Get the updated option sets for the element variation so that the
      // unibar can display the latest information.
      $option_sets = $form_state['storage']['option_sets'];
      $option_set_settings = array_map('_personalize_convert_option_set_to_js_setting', $option_sets);
      foreach($option_set_settings as $option_set_setting) {
        $settings['personalize']['option_sets'][key($option_set_setting)] = reset($option_set_setting);
      }
      // Include whether the option sets are editable for unibar.
      foreach ($option_sets as $option_set) {
        $settings += acquia_lift_get_option_set_editable_settings($option_set);
      }

      // Also get the updated personalize elements options.
      personalize_elements_limit_option_sets_by_path($option_sets, $form_state['values']['pages']);
      $settings['personalize_elements']['elements'] = personalize_elements_element_settings($option_sets);
      // We need to update the campaigns as well in case a campaign was paused by this action.
      $settings['acquia_lift']['campaigns'] = acquia_lift_get_campaign_details();
      // And the agent map.
      $settings['personalize']['agent_map'][$agent_name] = personalize_agent_get_map_settings($agent_name);
      // Note: JavaScript assets for personalize elements are included in
      // personalize_elements_page_build when the module is enabled and therefore
      // do not need to be explicitly added here.
      $commands[] = ajax_command_settings($settings, TRUE);
    }

    if ($add_and_close) {
      // End the editing mode.
      $commands[] = acquia_lift_command_variation_toggle(FALSE);
      if ($has_destination) {
        $commands[] = ctools_ajax_command_redirect($_SESSION['acquia_lift_element_destination']);
        unset($_SESSION['acquia_lift_element_destination']);
      }
      else {
        // Preview the new variation.
        $commands[] = acquia_lift_command_variation_preview($form_state['values']['agent'], $form_state['storage']['osid'], $form_state['storage']['option_id']);
        // Show a confirmation message.
        $current_status = personalize_agent_get_status($agent_name);
        $message = $form_state['storage']['confirmation_message'];
        if ($current_status === PERSONALIZE_STATUS_NOT_STARTED) {
          $errors = personalize_verify_agent($agent_name);
          if (empty($errors)) {
            $message .= ' ' . t('You can now start the personalization.');
          }
          else {
            $message .= ' ' . t('Add one or more goals by clicking Why &gt; Add goal.');
          }
        }
        $commands[] = acquia_lift_command_messagebox($message, 10);
      }
    }
    else {
      // Adding another variation.
      $option_set = personalize_option_set_load($form_state['storage']['osid']);
      $commands[] = acquia_lift_command_variation_edit($option_set);
    }
  }
  else if ($cancel) {
    // Turn off edit mode and redirect them.
    $commands[] = acquia_lift_command_variation_toggle(FALSE);
    if ($has_destination) {
      $commands[] = ctools_ajax_command_redirect($_SESSION['acquia_lift_element_destination']);
      unset($_SESSION['acquia_lift_element_destination']);
    }
  }

  return personalize_ajax_commands_deliver($commands, TRUE);
}

/***************************************************************
 *
 * O P T I O N  S E T  C R E A T E
 *
 ***************************************************************/

/**
 * Page callback to create a new option set.
 *
 * @param string $agent_name
 *   The machine name of the current camaign.
 * @param string $ajax
 *   Indicates if request is from 'ajax' or 'nojs' support.
 */
function acquia_lift_option_set_add_modal_callback($ajax) {
  if (!$ajax) {
    drupal_goto('admin/structure/personalize');
    return;
  }

  // Show the option set type selection form in a modal.
  ctools_include('modal');
  ctools_modal_add_js();

  module_load_include('inc', 'acquia_lift', 'acquia_lift.ui');
  $types = acquia_lift_option_set_types_ui();
  // Indicate that type links should not open in a modal.
  foreach ($types as $type) {
    $type['modal'] = FALSE;
  }
  // Update the path for block creation.
  $agent_name = personalize_get_campaign_context();
  $types['block']['path'] = 'admin/structure/personalize/manage/' . $agent_name . '/variations';
  $types['block']['url_options']['query'][] = array('create' => 'block');

  $output = theme('acquia_lift_type_list', array(
    'id' => 'acquia-lift-option-set-type-list',
    'items' => $types,
  ));
  return ctools_modal_render(t('Add a variation set'), $output);
}

/***************************************************************
 *
 * E L E M E N T  V A R I A T I O N  D E L E T E
 *
 ***************************************************************/

/**
 * Page callback to generate an element variation delete confirm modal window.
 *
 * @param string $osid
 *   The stringified option set id
 * @param string $option_id
 *   The id of the option to be deleted.
 */
function acquia_lift_element_variation_delete_modal_callback($osid, $option_id) {
  acquia_lift_create_ctools_form(
    t('Delete variation'),
    'acquia_lift_element_variation_delete_form',
    array(
      'osid' => personalize_resolve_stringified_osid($osid),
      'option_id' => $option_id,
    ),
    'acquia_lift_element_variation_delete_complete_callback'
  );
}

/**
 * Ctools form processing complete handler for deletion of an element variation.
 * @see acquia_lift_create_ctools_form().
 */
function acquia_lift_element_variation_delete_complete_callback($form, $form_state) {
  $settings = array();
  $form_option_set = $form_state['values']['option_set'];
  $osid = $form_option_set->osid;

  $option_sets = personalize_option_set_load_by_agent($form_state['values']['agent_name']);
  $updated = NULL;
  foreach ($option_sets as $option_set) {
    if ($option_set->osid === $osid) {
      $updated = $option_set;
      break;
    }
  }
  if ($updated) {
    // The variation has been deleted, but the option set still has more.
    $message = t('The variation has been deleted.');
    $settings = _personalize_convert_option_set_to_js_setting($updated);
  }
  else {
    $message = t('The variation set has been deleted.');
    if (empty($option_sets)) {
      // There are no more option sets in the agent.
      $settings['empty'] = $form_state['values']['agent_name'];
    }
    else {
      // The last variation was removed from the option set, but the agent
      // still has other option sets.
      $settings[personalize_stringify_osid($osid)] = 'empty';
    }
  }
  $campaign_settings['acquia_lift']['campaigns'] = acquia_lift_get_campaign_details();
  $commands = array(
    ajax_command_settings($campaign_settings, TRUE),
    ctools_modal_command_dismiss(),
    acquia_lift_command_messagebox($message, 10),
    acquia_lift_command_option_set_updates($settings),
  );
  return $commands;
}

/**
 * Form handler to generate a confirmation form to delete an element variation.
 *
 * This form is called within a ctools modal window.
 *
 * @param string $agent_name
 *   The name of the campaign to edit.
 * @param int $osid
 *   The id of the option set
 * @param string $option_id
 *   The id of the option/variation.
 */
function acquia_lift_element_variation_delete_form($form, &$form_state, $osid, $option_id) {
  ctools_include('modal');
  ctools_include('ajax');
  ctools_add_js('ajax-responder');

  $option_set = personalize_option_set_load($osid);
  $variation_name = $option_id;
  foreach ($option_set->options as $option) {
    if ($option['option_id'] === $option_id) {
      $variation_name = $option['option_label'];
      break;
    }
  }

  $form = array();
  $form['agent_name'] = array(
    '#type' => 'value',
    '#value' => $option_set->agent,
  );
  $form['option_set'] = array(
    '#type' => 'value',
    '#value' => $option_set,
  );
  $form['option_id'] = array(
    '#type' => 'value',
    '#value' => $option_id,
  );
  if ($option_id == PERSONALIZE_CONTROL_OPTION_ID) {
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
    '#value' => t('Delete'),
  );
  $form['actions']['reset'] = array(
    '#markup' => ctools_ajax_text_button(t('Cancel'), 'admin/structure/acquia_lift/cancel/nojs', t('Cancel'), 'acquia-lift-cancel-button'),
  );
  return $form;
}

/**
 * Submit handler for acquia_lift_element_variation_delete_form().
 */
function acquia_lift_element_variation_delete_form_submit($form, &$form_state) {
  $option_set = $form_state['values']['option_set'];
  foreach ($option_set->options as $key => $option) {
    if ($option['option_id'] === $form_state['values']['option_id']) {
      unset($option_set->options[$key]);
      break;
    }
  }
  // @todo: This logic should be moved into personalize.
  if (count($option_set->options) < 2) {
    personalize_option_set_delete($option_set->osid);
  }
  else {
    personalize_option_set_save($option_set);
  }
}

/***************************************************************
 *
 * G O A L  C R E A T E
 *
 ***************************************************************/

/**
 * Page callback to create a new goal by selecting the type of goal to create.
 *
 * @param bool $ajax
 *   Indicates if the form is called with ajax support.
 */
function acquia_lift_goal_create_modal_callback($ajax) {
  if (!$ajax) {
    drupal_goto('admin/structure/personalize');
    return;
  }

  // Show the goal type selection form in a modal.
  ctools_include('modal');
  ctools_modal_add_js();

  module_load_include('inc', 'acquia_lift', 'acquia_lift.ui');
  $types = acquia_lift_goal_types_ui();
  // Handle differences in modal submits based on type of goal.
  foreach ($types as $id => &$type) {
    if ($id === 'element') {
      $type['modal'] = FALSE;
    }
    else {
      $type['modal'] = TRUE;
      $type['ctools_style'] = 'ctools-modal-acquia-lift-style';
    }
  }

  $output = theme('acquia_lift_type_list', array(
    'id' => 'acquia-lift-modal-goal-type-list',
    'items' => $types,
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
function acquia_lift_goal_type_create_modal_callback($type, $ajax) {
  acquia_lift_create_ctools_form(
    t('Add a goal'),
    'acquia_lift_goal_type_create_form',
    array('type' => $type),
    'acquia_lift_goal_type_create_completed_callback'
  );
}

/**
 * Ctools form processing complete handler for the creation of a new goal by
 * type.
 * @see acquia_lift_create_ctools_form().
 */
function acquia_lift_goal_type_create_completed_callback($form, &$form_state) {
  $agent = $form_state['values']['agent'];
  $settings = acquia_lift_get_campaign_details();
  if ($settings[$agent->machine_name]['verified'] && personalize_agent_get_status($agent->machine_name) === PERSONALIZE_STATUS_NOT_STARTED) {
    $message = t('Goal added. You can now !start the personalization.', array('!start' => l(t('start'), 'admin/structure/personalize/' . $agent->machine_name)));
  }
  else {
    $message = personalize_sanitize_string($form_state['acquia_lift']['goal_name']) . ' ' . t('goal added to personalization.');
  }
  // Load updated data about custom defined visitor actions.
  $actions = visitor_actions_custom_load_multiple();

  $commands = array(
    ctools_modal_command_dismiss(),
    ajax_command_settings(array(
      'acquia_lift' => array(
        'campaigns' => $settings,
        'customActions' => $actions,
      )
    ), TRUE),
    acquia_lift_command_messagebox($message, 10),
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
function acquia_lift_goal_type_create_form($form, &$form_state, $type) {
  ctools_include('modal');
  ctools_include('ajax');
  ctools_add_js('ajax-responder');

  $agent_name = personalize_get_campaign_context();
  $agent = personalize_agent_load($agent_name);
  if (empty($agent)) {
    $form['message'] = array(
      '#markup' => t('You must select a personalization before adding goals.'),
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
      $form['goal'] = _acquia_lift_existing_goal_create_form($agent);
      break;
    case 'page':
      $form['goal_type_change'] = array(
        '#markup' => theme('acquia_lift_create_type_change', array(
          'type' => t('Page goal'),
          'id' => 'acquia-lift-create-goal-page',
          'change_link' => $change_link,
        )),
      );
      $form['goal'] = _acquia_lift_page_goal_create_form($agent);
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
function _acquia_lift_existing_goal_create_form($agent) {
  // Exclude any actions already used in this campaign.
  $exclude = array();
  $goals = personalize_goal_load_by_conditions(array('agent' => $agent->machine_name));
  foreach ($goals as $goal) {
    $exclude[$goal->action] = $goal->action;
  }
  module_load_include('inc', 'personalize', 'personalize.admin');
  $all_goal_options = personalize_goals_options(TRUE);
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
function _acquia_lift_page_goal_create_form($agent) {
  $form['actionable_element'] = array(
    '#type' => 'value',
    '#value' => 'page',
  );
  $form['title'] = array(
    '#title' => t('Title'),
    '#type' => 'textfield',
    '#required' => TRUE,
    '#attributes' => array(
      'placeholder' => t('Example: Purchases any item'),
    ),
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

  $form['#element_validate'][] = '_acquia_lift_page_goal_create_form_validate';
  module_load_include('inc', 'visitor_actions', 'visitor_actions.admin');
  visitor_actions_get_form_options('page', $form, FALSE);
  return $form;
}

/**
 * Form validation for creating a new page goal simplified form.
 */
function _acquia_lift_page_goal_create_form_validate($form, &$form_state) {
  // Allow the plugin to validate the form values.
  ctools_include('plugins');
  if ($class = ctools_plugin_load_class('visitor_actions', 'actionable_element', $form_state['values']['actionable_element'], 'handler')) {
    call_user_func_array(array($class, 'validate'), array($form_state['values']));
  }
}

/**
 * Submit handler to create a new goal for a campaign.
 */
function acquia_lift_goal_type_create_form_submit($form, &$form_state) {
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
      $action_name = personalize_generate_machine_name($goal_name, 'visitor_actions_action_name_exists', '_');
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
    if (!empty($action_name)) {
      personalize_goal_save($agent_data->machine_name, $action_name, 1);
    }
  } catch (Exception $e) {
    drupal_set_message($e->getMessage(), 'error');
  }
  $form_state['acquia_lift']['goal_name'] = $goal_name;
}

/***************************************************************
 *
 * G O A L  R E N A M E
 *
 ***************************************************************/

/**
 * Page callback to generate a goal rename modal window.
 *
 * @param string $action_name
 *   The machine name for the visitor action for the goal.
 */
function acquia_lift_goal_rename_modal_callback($action_name) {
  $action = visitor_actions_custom_load($action_name);
  acquia_lift_create_ctools_form(
    t('Rename goal'),
    'acquia_lift_goal_rename_form',
    array(
      'action' => $action,
    ),
    'acquia_lift_goal_rename_complete_callback'
  );
}

/**
 * Ctools form processing complete handler for the renaming of a goal.
 * @see acquia_lift_create_ctools_form().
 */
function acquia_lift_goal_rename_complete_callback($form, $form_state) {
  $settings = acquia_lift_get_campaign_details();
  $message = $form_state['acquia_lift']['goal_rename_success'] ? t('The goal has been renamed.') : t('There was a problem renaming the goal.');
  $commands = array(
    ctools_modal_command_dismiss(),
    ajax_command_settings(array('acquia_lift' => array(
      'campaigns' => $settings,
    )), TRUE),
    acquia_lift_command_messagebox($message, 10),
  );
  return $commands;
}

/**
 * Form handler to generate a form to change a goal name.
 *
 * This form is called within a ctools modal window.
 *
 * @param array $action
 *   The goal's underlying visitor action.
 */
function acquia_lift_goal_rename_form($form, &$form_state, $action) {
  ctools_include('modal');
  ctools_include('ajax');
  ctools_add_js('ajax-responder');

  $form = array();
  $form['action'] = array(
    '#type' => 'value',
    '#value' => $action,
  );
  $form['label'] = array(
    '#type' => 'textfield',
    '#title' => t('New name'),
    '#default_value' => $action['label'],
    '#required' => TRUE,
    '#maxlength' => PERSONALIZE_MACHINE_NAME_MAXLENGTH,
  );
  $form['actions']['submit_form'] = array(
    '#type' => 'submit',
    '#attributes' => array(
      'class' => array('action-item-primary-active', 'acquia-lift-submit-button'),
    ),
    '#value' => t('Rename'),
  );
  $form['actions']['reset'] = array(
    '#markup' => ctools_ajax_text_button(t('Cancel'), 'admin/structure/acquia_lift/cancel/nojs', t('Cancel'), 'acquia-lift-cancel-button'),
  );
  return $form;
}

/**
 * Submit handler for acquia_lift_goal_rename().
 */
function acquia_lift_goal_rename_form_submit($form, &$form_state) {
  $action = $form_state['values']['action'];
  $action['label'] = $form_state['values']['label'];
  // Limit those fields to save based on those available in the custom table.
  $schema = drupal_get_schema('visitor_actions_actions');
  $action = array_intersect_key($action, $schema['fields']);
  $form_state['acquia_lift']['goal_rename_success'] = visitor_actions_save_action($action);
}

/***************************************************************
 *
 * V I S I T O R  A C T I O N  E D I T
 *
 ***************************************************************/
/**
 * Page callback to generate a visitor action edit modal window.
 *
 * @param array $action
 *   The action to edit.
 */
function acquia_lift_visitor_action_edit_modal_callback($action) {
  acquia_lift_create_ctools_form(
    t('Edit visitor action'),
    'acquia_lift_visitor_action_edit_form',
    array(
      'action' => $action,
    ),
    'acquia_lift_visitor_action_edit_complete_callback'
  );
}

/**
 * Ctools processing complete handler for the editing of a visitor action.
 * @see acquia_lift_create_ctools_form().
 */
function acquia_lift_visitor_action_edit_complete_callback($form, $form_state) {
  $commands = array(
    ctools_modal_command_dismiss(),
    ctools_ajax_command_reload(),
  );
  return $commands;
}

/**
 * Form handler to generate a form to edit a visitor action.
 *
 * This is called within a ctools modal window.
 *
 * @param array $action
 *   The visitor action to edit.
 */
function acquia_lift_visitor_action_edit_form($form, &$form_state, $action) {
  ctools_include('modal');
  ctools_include('ajax');
  ctools_add_js('ajax-responder');

  module_load_include('inc', 'visitor_actions', 'visitor_actions.admin');
  $form = visitor_actions_form($form, $form_state, NULL, TRUE, $action);
  $form['actions']['submit_form']['#attributes']['class'][] = 'action-item-primary-active';
  $form['actions']['submit_form']['#attributes']['class'][] = 'acquia-lift-submit-button';
  return $form;
}

/**
 * Submit handler for acquia_lift_visitor_action_edit_form().
 */
function acquia_lift_visitor_action_edit_form_submit($form, &$form_state) {
  module_load_include('inc', 'visitor_actions', 'visitor_actions.admin');
  return visitor_actions_form_submit($form, $form_state);
}

/***************************************************************
 *
 * G O A L  D E L E T E
 *
 ***************************************************************/


/**
 * Page callback to generate a goal delete confirm modal window.
 *
 * @param string $agent_name
 *   The name of the campaign to edit.
 * @param string $action_name
 *   The machine name for the visitor action for the goal.
 */
function acquia_lift_goal_delete_modal_callback($agent_name, $action_name) {
  acquia_lift_create_ctools_form(
    t('Delete goal'),
    'acquia_lift_goal_delete_form',
    array(
      'agent_name' => $agent_name,
      'action_name' => $action_name,
    ),
    'acquia_lift_goal_delete_complete_callback'
  );
}

/**
 * Ctools form processing complete handler for deletion of a goal.
 * @see acquia_lift_create_ctools_form().
 */
function acquia_lift_goal_delete_complete_callback($form, $form_state) {
  $message = $form_state['acquia_lift']['goal_delete_success'] ? t('The goal has been deleted.') : t('There was a problem deleting the goal.');
  $commands = array(
    ctools_modal_command_dismiss(),
    acquia_lift_command_messagebox($message, 10),
    acquia_lift_command_goal_updates($form_state['values']['agent_name']),
  );
  return $commands;
}

/**
 * Form handler to generate a confirmation form to delete a goal.
 *
 * This form is called within a ctools modal window.
 *
 * @param string $agent_name
 *   The name of the campaign to edit.
 * @param string $action_name
 *   The machine name for the visitor action for the goal.
 */
function acquia_lift_goal_delete_form($form, &$form_state, $agent_name, $action_name) {
  ctools_include('modal');
  ctools_include('ajax');
  ctools_add_js('ajax-responder');

  $form = array();
  // Will only load if this is a custom action (not defined in code).
  $action = visitor_actions_custom_load($action_name);
  $form['agent_name'] = array(
    '#type' => 'value',
    '#value' => $agent_name,
  );
  $form['action_name'] = array(
    '#type' => 'value',
    '#value' => $action_name,
  );
  $form['confirm'] = array(
    '#prefix' => '<div>',
    '#suffix' => '</div>',
    '#markup' => t('Are you sure you want to delete %goal?', array(
      '%goal' => isset($action['label']) ? $action['label'] : $action_name,
    )),
  );
  $form['actions']['submit_form'] = array(
    '#type' => 'submit',
    '#attributes' => array(
      'class' => array('action-item-primary-active', 'acquia-lift-submit-button'),
    ),
    '#value' => t('Delete'),
  );
  $form['actions']['reset'] = array(
    '#markup' => ctools_ajax_text_button(t('Cancel'), 'admin/structure/acquia_lift/cancel/nojs', t('Cancel'), 'acquia-lift-cancel-button'),
  );
  return $form;
}

/**
 * Submit handler for acquia_lift_goal_delete_form().
 */
function acquia_lift_goal_delete_form_submit($form, &$form_state) {
  $goals = personalize_goal_load_by_conditions(array(
    'agent' => $form_state['values']['agent_name'],
    'action' => $form_state['values']['action_name'],
  ));
  if (count($goals) === 1) {
    $goal = reset($goals);
    personalize_goal_delete($goal->id);
    $form_state['acquia_lift']['goal_delete_success'] = TRUE;
  }
  else {
    $form_state['acquia_lift']['goal_delete_success'] = FALSE;
  }
}

/***************************************************************
 *
 * G E N E R A L  D I A L O G  F U N C T I O N S
 *
 ***************************************************************/

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
