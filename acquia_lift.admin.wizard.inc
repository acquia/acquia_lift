<?php
/**
 * @file acquia_lift.admin.wizard.inc Functions specific to the Acquia Lift
 * alteration of the campaign creation wizard.
 */
define('ACQUIA_LIFT_TARGETING_EVERYONE_ELSE', 'everyone-else');
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
function acquia_lift_personalize_campaign_wizard_process_bar_alter(&$form, &$form_state, $form_id) {
  $agent = $form['#agent'];

  // If we're editing an existing agent, add a "Reset data" button next to
  // the Pause/Resume button.
  $reset_form = array(
    '#prefix' => '<div id="personalize-acquia-lift-reset-form">',
    '#suffix' => '</div>'
  );
  $reset_form['actions']['reset'] = array(
    '#prefix' => '<div id="personalize-acquia-lift-reset">',
    '#suffix' => '</div>',
    '#type' => 'submit',
    '#name' => 'reset_submit',
    '#value' => t('Reset data'),
    '#attributes' => array(
      'class' => array('action-item-primary-active'),
    ),
    '#ajax' => array(
      'callback' => 'personalize_acquia_lift_ajax_callback',
      'wrapper' => 'personalize-acquia-lift-reset-form',
      'effect' => 'fade',
    )
  );
  $reset_form['actions']['reset']['#submit'] = array('acquia_lift_reset_submit');
  $form['process_bar']['actions']['reset_lift_agent'] = $reset_form;
  // Now the Sync button.
  $sync_form = array(
    '#prefix' => '<div id="personalize-acquia-lift-sync-form">',
    '#suffix' => '</div>'
  );
  $sync_form['agent_name'] = array(
    '#type' => 'value',
    '#value' => $agent->machine_name,
  );
  $sync_form['actions']['sync'] = array(
    '#prefix' => '<div id="personalize-acquia-lift-sync">',
    '#suffix' => '</div>',
    '#type' => 'submit',
    '#name' => 'sync_submit',
    '#value' => t('Sync with Lift'),
    '#attributes' => array(
      'class' => array('action-item-primary-active'),
    ),
  );
  $sync_form['actions']['sync']['#submit'] = array('acquia_lift_sync_campaign_submit');
  $form['process_bar']['actions']['sync_lift_agent'] = $sync_form;
}

/**
 * Alter hook for the base campaign wizard form.
 */
function acquia_lift_personalize_campaign_wizard_base_alter(&$form, &$form_state, $form_id) {
  // Make the campaign name a self-revealing text input.
  $form['agent_basic_info']['title']['#theme_wrappers'][] = 'acquia_lift_revealing_input';
}

/**
 * Alter hook for the variations portions of the campaign wizard.
 */
function acquia_lift_personalize_campaign_wizard_variations_alter(&$form, &$form_state, $form_id) {
  // Rebuild the variations form to show the customized Lift approach.
  unset($form['variations']['title']['summary']);
  unset($form['variations']['option_sets']);
  $form['variations']['#tree'] = TRUE;

  // Take over the form validation and submit handling.
  $form['#validate'] = array('acquia_lift_personalize_campaign_wizard_variations_validate');
  $form['#submit'] = array('acquia_lift_personalize_campaign_wizard_submit');

  $agent_data = $form['#agent'];
  if (empty($agent_data->machine_name)) {
    return;
  }
  $option_sets = personalize_option_set_load_by_agent($agent_data->machine_name);

  // Make a container to hold existing option sets.
  if (!empty($option_sets)) {
    $form['variations']['option_sets'] = array(
      '#type' => 'container',
      '#attributes' => array(
        'class' => array('personalize-wizard-variation-sets'),
      ),
    );
  }
  // Add an option set edit card for each option set.
  foreach ($option_sets as $option_set) {
    $option_set_plugin = personalize_get_option_set_type($option_set->plugin);
    $form['variations']['option_sets']['option_set_' . $option_set->osid] = array(
      '#tree' => TRUE,
      '#type' => 'container',
      '#theme' => 'acquia_lift_card',
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
      '#title' => $option_set->label,
      '#attributes' => array(
        'class' => array('personalize-option-set'),
      ),
    );
    $form['variations']['option_sets']['option_set_' . $option_set->osid]['option_set'] = array(
      '#type' => 'value',
      '#value' => $option_set,
    );
    $form['variations']['option_sets']['option_set_' . $option_set->osid]['header'] = array(
      '#type' => 'container',
    );
    $delete_link = module_hook($option_set_plugin['module'], 'personalize_delete_link') ? module_invoke($option_set_plugin['module'], 'personalize_delete_link', $option_set) : '';
    if (!empty($delete_link)) {
      $form['variations']['option_sets']['option_set_' . $option_set->osid]['header']['delete'] = array(
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
    switch ($option_set->plugin) {
      case 'block':
        $form['variations']['option_sets']['option_set_' . $option_set->osid]['content'] = _acquia_lift_personalize_campaign_wizard_variations_block($form, $form_state, $agent_data, array('variations', 'option_sets', 'option_set_' . $option_set->osid, 'content'), $option_set);
        break;
      case 'elements':
        $form['variations']['option_sets']['option_set_' . $option_set->osid]['content'] = _acquia_lift_personalize_campaign_wizard_variations_element($form, $form_state, $agent_data, $option_set);
        break;
      // Unsupported display - get an edit link if provided by the plugin.
      default:
        $edit_link = module_hook($option_set_plugin['module'], 'personalize_edit_link') ? module_invoke($option_set_plugin['module'], 'personalize_edit_link', $option_set) : '';
        if (!empty($edit_link)) {
          $markup = l(t('Edit this variation set'), $edit_link);
        }
        else {
          $markup = t('Variation set type ' . $option_set->plugin . ' does not provide a link for editing.');
        }
        $form['variations']['option_sets']['option_set_' . $option_set->osid]['content'] = array(
          '#markup' => $markup,
        );
    }
    // Add the advanced settings form.
    $form['variations']['option_sets']['option_set_' . $option_set->osid]['advanced'] = personalize_campaign_wizard_variations_advanced($form, $form_state, $option_set);
    // Option set label is edited in a different spot so don't show it twice.
    $form['variations']['option_sets']['option_set_' . $option_set->osid]['advanced']['label']['#access'] = FALSE;
  }

  // Show the card to add a new option set last.
  module_load_include('inc', 'acquia_lift', 'acquia_lift.ui');
  $option_set_types = acquia_lift_option_set_types_ui();
  foreach ($option_set_types as $type => $details) {
    $options[$type] = $details['title'];
  }
  $form['variations']['add_variation'] = array(
    '#type' => 'container',
    '#tree' => TRUE,
    '#theme' => 'acquia_lift_card',
    '#collapsed' => !empty($option_sets),
    '#title' => t('Add variation set'),
    '#collapsible' => TRUE,
  );
  // Tricky: we need this container in order to hide the radio list when a
  // selection is made.  The radios type does not have a proper wrapping
  // element to apply visible/invisible states.
  $form['variations']['add_variation']['option_set_types'] = array(
    '#type' => 'container',
    '#attributes' => array(
      'class' => array('form-item'),
    ),
    '#states' => array(
      'invisible' => array(
        ':input[name="variations[add_variation][option_set_types][option_set_type]"]' => array('!value' => false),
      ),
    )
  );
  $form['variations']['add_variation']['option_set_types']['option_set_type'] = array(
    '#type' => 'radios',
    '#options' => $options,
    '#theme' => 'acquia_lift_radio_list',
    '#ajax' => array(
      'callback' => 'acquia_lift_personalize_campaign_wizard_variations_ajax_add',
      'wrapper' => 'acquia-lift-add-variation-details',
      'effect' => 'fade',
    ),
    '#attributes' => array(
      'autocomplete' => 'off',
    ),
  );
  foreach ($option_set_types as $type => $details) {
    $form['variations']['add_variation']['option_set_types']['option_set_type'][$type]['#description'] = $details['description'];
    $form['variations']['add_variation']['option_set_types']['option_set_type'][$type]['#image'] = $details['logo'];
  }

  $form['variations']['add_variation']['details'] = array(
    '#type' => 'container',
    '#attributes' => array(
      'id' => 'acquia-lift-add-variation-details',
    ),
  );
  if (isset($form_state['values']['variations']['add_variation']['option_set_types']['option_set_type'])) {
    switch ($form_state['values']['variations']['add_variation']['option_set_types']['option_set_type']) {
      case 'block':
        $form['variations']['add_variation']['details']['block'] = array(
          '#type' => 'container',
          '#attributes' => array(
            'class' => array('acquia-lift-block-variation-set'),
          ),
        );
        $form['variations']['add_variation']['details']['block']['content'] = _acquia_lift_personalize_campaign_wizard_variations_block($form, $form_state, $agent_data, array('variations', 'add_variation', 'details', 'block', 'content'));
        break;

      case 'element':
        $form['variations']['add_variation']['details']['element'] = array(
          '#type' => 'container',
          '#attributes' => array(
            'class' => array('acquia-lift-element-variation-set'),
          ),
        );
        $form['variations']['add_variation']['details']['element']['content'] = _acquia_lift_personalize_campaign_wizard_variations_element($form, $form_state, $agent_data);
        break;
    }
  }
}

/**
 * Alter hook for the targeting portion of the campaign wizard.
 */
function acquia_lift_personalize_campaign_wizard_targeting_alter(&$form, &$form_state, $form_id) {
  if (!empty($form['targeting']['visitor_context']) && $form['targeting']['visitor_context']['#type'] == 'select') {
    module_load_include('inc', 'acquia_lift', 'acquia_lift.ui');
    acquia_lift_chosenify_element($form['targeting']['visitor_context']);
  }

  // Acquia Lift Target agents allow for audience designation to be used across
  // the agent.
  $agent_data = $form['#agent'];
  if ($agent_data->plugin !== 'acquia_lift_target') {
    return;
  }
  module_load_include('inc', 'acquia_lift', 'acquia_lift.admin');
  $option_set = acquia_lift_get_option_set_for_targeting($agent_data->machine_name);

  if (empty($option_set)) {
    drupal_set_message(t('You must <a href="!variations_url">create variation sets</a> before targeting.', array(
      '!variations_url' => url('admin/structure/personalize/manage/' . $agent_data->machine_name . '/variations'),
    )), 'warning');
    return;
  }
  // Take over the form validation and submit handling.
  $form['#validate'] = array('acquia_lift_personalize_campaign_wizard_targeting_validate');
  $form['#submit'] = array('acquia_lift_personalize_campaign_wizard_submit');

  unset($form['targeting']['option_sets']);

  $variation_options = $saved_targeting = array();
  // Load the targeting that is currently in use for a campaign.
  $existing_targeting = acquia_lift_get_structure_from_targeting($option_set);
  // Track whether an option to revert targeting changes should be shown.
  $show_revert = FALSE;
  // The settings reflected in the form will either be what is currently running
  // or any saved overrides from when this form was last submitted (if the
  // review step was never completed to apply them.)
  if (!empty($agent_data->data['lift_targeting'])) {
    // Get the current state of targeting work within the UI that has not
    // yet been saved to the agent.
    $saved_targeting = $agent_data->data['lift_targeting'];
    if ($saved_targeting != $existing_targeting) {
      drupal_set_message(t('The targeting settings shown here do not match what is currently running for this campaign.'), 'warning');
      $show_revert = TRUE;
    }
  }
  else {
    if (personalize_agent_get_status($agent_data->machine_name) != PERSONALIZE_STATUS_NOT_STARTED) {
      // The current agent is running and there have been no UI changes made
      // since it was started.
      drupal_set_message(t('The targeting settings shown here represent what is currently running for this campaign.'));
    }
    // Set the saved targeting to be the same as existing targeting for
    // presentation within this form.
    $saved_targeting = $existing_targeting;
  }
  // Get the names of the options from the representative option set.
  foreach ($option_set->options as $option) {
    $variation_options[$option['option_id']] = $option['option_label'];
  }

  // Begin the audience form.
  $form['audiences'] = array(
    '#type' => 'container',
    '#tree' => TRUE,
    '#attributes' => array(
      'id' => 'acquia-lift-targeting-audiences',
    ),
  );

  // Add a new audience rule.
  $form['audiences']['add'] = array(
    '#type' => 'container',
    '#theme' => 'acquia_lift_card',
    '#collapsible' => TRUE,
    '#collapsed' => !empty($option_set->targeting),
    '#title' => t('Add target audience'),
  );
  $form['audiences']['add']['details'] = _acquia_lift_personalize_campaign_wizard_targeting_audience($form, $form_state, $agent_data, 'add');

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
    foreach ($option_set->targeting as $audience_id => $audience) {
      $form['audiences'][$audience_id] = array(
        '#type' => 'container',
        '#theme' => 'acquia_lift_card',
        '#title' => isset($audience['label']) ? $audience['label'] : 'Audience: ' . $audience_id,
        '#collapsible' => FALSE,
      );
      $form['audiences'][$audience_id]['header'] = array(
        '#type' => 'container'
      );
      if ($audience_id !== ACQUIA_LIFT_TARGETING_EVERYONE_ELSE) {
        $form['audiences'][$audience_id]['header']['edit'] = array(
          '#type' => 'checkbox',
          '#title' => t('Edit'),
          '#attributes' => array(
            'class' => array('acquia-lift-edit'),
          ),
        );
        $form['audiences'][$audience_id]['header']['delete'] = array(
          '#markup' => l(t('Delete'), 'admin/structure/personalize/manage/' . $agent_data->machine_name . '/audience/' . $audience_id . '/delete', array(
            'attributes' => array(
              'class' => array('acquia-lift-delete')
            ),
          )),
        );

        // Show the form to edit the audience segment when edit is selected.
        $form['audiences'][$audience_id]['details'] = _acquia_lift_personalize_campaign_wizard_targeting_audience($form, $form_state, $agent_data, $audience_id, $audience);
        $form['audiences'][$audience_id]['details']['#states'] = array(
          'visible' => array(
            ':input[name="audiences[' . $audience_id . '][header][edit]"]' => array('checked' => TRUE),
          )
        );
      }
      // Show the form to assign variations when not in edit mode.
      $form['audiences'][$audience_id]['assignment'] = array(
        '#type' => 'select',
        '#title' => t('Add display option'),
        '#multiple' => TRUE,
        '#options' => $variation_options,
        '#default_value' => isset($saved_targeting[$audience_id]) ? $saved_targeting[$audience_id] : '',
        '#empty_option' => t('Select...'),
        '#states' => array(
          'visible' => array(
            ':input[name="audiences[' . $audience_id . '][header][edit]"]' => array('checked' => FALSE),
          )
        )
      );
    }
  }
  if ($show_revert) {
    $form['actions']['revert'] = array(
      '#type' => 'submit',
      '#value' => t('Revert changes'),
    );
  }

  // Add a new audience.
  $form['audiences']['add'] = array(
    '#type' => 'container',
    '#theme' => 'acquia_lift_card',
    '#collapsible' => TRUE,
    '#collapsed' => !empty($option_set->targeting),
    '#title' => t('Add target audience'),
  );
  $form['audiences']['add']['details'] = _acquia_lift_personalize_campaign_wizard_targeting_audience($form, $form_state, $agent_data, 'add');
}

/**
 ********************************************************************
 *
 * A J A X  C A L L B A C K S
 *
 ********************************************************************
 */

/**
 * Ajax handler when adding a new variation set.
 */
function acquia_lift_personalize_campaign_wizard_variations_ajax_add($form, &$form_state) {
  return $form['variations']['add_variation']['details'];
}

/**
 * Submit handler to add another block option to a personalized block form.
 */
function acquia_lift_personalize_campaign_wizard_blocks_add($form, &$form_state) {
  // Value will be "blocks_add" or "blocks_{osid}" where the second part is the
  // key within the form state storage for the number of blocks to generate.
  list($blocks, $key) = explode('_', $form_state['triggering_element']['#value']);
  $form_state['option_set_num_blocks'][$key]++;
  $form_state['rebuild'] = TRUE;
}

/**
 * Callback to for AJAX to generate the personalize block wrapper form.
 */
function acquia_lift_personalize_campaign_wizard_blocks_ajax($form, &$form_state) {
  // The triggering element and the blocks container share a common parent.
  $parents = $form_state['triggering_element']['#array_parents'];
  $section = $form;
  for ($i = 0; $i < count($parents) -1; $i++) {
    $section = $section[$parents[$i]];
  }
  return $section['blocks'];
}

/**
 * Submit handler for the "Add Context" button.
 */
function acquia_lift_personalize_campaign_wizard_targeting_add($form, &$form_state) {
  // Increment the number of contexts to be rendered.
  $context_key = $form_state['triggering_element']['#array_parents'][1];
  $form_state['num_contexts_audience'][$context_key]++;
  $form_state['rebuild'] = TRUE;
}

/**
 * Submit handler for the "Remove Context" button.
 */
function acquia_lift_personalize_campaign_wizard_targeting_remove($form, &$form_state) {
  $context_key = $form_state['triggering_element']['#array_parents'][1];
  $parents = $form_state['clicked_button']['#parents'];
  $delta_pos = array_search('contexts', $parents) + 1;
  $delta = $parents[$delta_pos];
  $form_state['to_remove_audience'][$context_key] = $delta;
  $form_state['rebuild'] = TRUE;
}

/**
 * Ajax callback for the add context and remove context buttons.
 */
function acquia_lift_personalize_campaign_wizard_targeting_ajax($form, &$form_state) {
  $context_key = $form_state['triggering_element']['#array_parents'][1];
  return $form['audiences'][$context_key]['details']['mapping']['contexts'];
}

/**
 ********************************************************************
 *
 * V A L I D A T I O N
 *
 ********************************************************************
 */

/**
 * Validation function for variations form.
 */
function acquia_lift_personalize_campaign_wizard_variations_validate(&$form, &$form_state) {
  // Get a reference to the full values array.
  $values = $form_state['values'];

  // Run any base form validation if available.
  module_load_include('inc', 'personalize', 'personalize.admin.campaign');
  if (function_exists('personalize_campaign_wizard_validate_base')) {
    personalize_campaign_wizard_validate_base($form, $form_state);
  }
  // Validation for existing option set types.
  if (!empty($form_state['values']['variations']['option_sets'])) {
    foreach ($form_state['values']['variations']['option_sets'] as $option_set_id => $option_set_values) {
      switch ($option_set_values['option_set']->plugin) {
        case "block":
          module_load_include('inc', 'personalize_blocks', 'personalize_blocks.admin');
          _acquia_lift_personalize_campaign_wizard_form_state_blocks_alter($form_state, $option_set_values['content'], $option_set_values['option_set']);
          personalize_blocks_form_validate($form, $form_state);
          $form_state['values'] = $values;
          break;
        case "element":
          break;
        default:
          // Unsupported option set type.
      }
      // Validate advanced settings.
      personalize_campaign_wizard_validate_variations_advanced($form_state['values']['variations']['option_sets'][$option_set_id], 'variations][option_sets][' . $option_set_id . ']');
    }
  }
  // Validation for any new option set types once the details have been input.
  if (!empty($form_state['values']['variations']['add_variation']['option_set_types']['option_set_type']) &&
    !empty($values['variations']['add_variation']['details'])) {
    switch ($form_state['values']['variations']['add_variation']['option_set_types']['option_set_type']) {
      case "block":
        module_load_include('inc', 'personalize_blocks', 'personalize_blocks.admin');
        _acquia_lift_personalize_campaign_wizard_form_state_blocks_alter($form_state, $values['variations']['add_variation']['details']['block']['content']);
        personalize_blocks_form_validate($form, $form_state);
        $form_state['values'] = $values;
        break;
      case "element":
        break;
    }
  }

  // Put the form state back the way it was for the next steps in form processing.
  $form_state['values'] = $values;
}

/**
 * Validation function for targeting form.
 */
function acquia_lift_personalize_campaign_wizard_targeting_validate(&$form, &$form_state) {
  // Verify that a new audience will have a unique machine name.
  if (!empty($form_state['values']['audiences']['add']['details']['name'])) {
    module_load_include('inc', 'acquia_lift', 'acquia_lift.admin');
    $machine_name = personalize_generate_machine_name($form_state['values']['audiences']['add']['details']['name'], NULL, '-');
    $agent_data = $form['#agent'];
    $option_set = acquia_lift_get_option_set_for_targeting($agent_data->machine_name);
    if (isset($option_set->targeting[$machine_name])) {
      form_set_error('audiences[add][details][name', t('Please choose a different name for your audience as this one is already taken'));
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

  // If the subform submit handlers didn't already set a redirect, then set the
  // next step and rebuild the form.
  if (empty($form_state['redirect'])) {
    $form_state['storage']['step'] = _personalize_campaign_wizard_next_step($form_state, $agent_instance);;
    $form_state['rebuild'] = TRUE;
  }
}

/**
 * Submit function for variations form.
 */
function acquia_lift_personalize_campaign_wizard_variations_submit(&$form, &$form_state, &$agent_data, $agent_instance) {
  // Save a reference to the full form state values.
  $values = $form_state['values'];
  $click_id = $form_state['clicked_button']['#id'];

  // Common functionality needed before submitting the block submission
  // handler whether for new or existing blocks.
  $submitBlockHandler = function ($block_content, $option_set = NULL) use ($form, $form_state) {
    module_load_include('inc', 'personalize_blocks', 'personalize_blocks.admin');
    _acquia_lift_personalize_campaign_wizard_form_state_blocks_alter($form_state, $block_content, $option_set);
    // Submit handler only runs if there is a specific button id.
    $form_state['clicked_button']['#id'] = 'edit-submit-form';
    personalize_blocks_form_submit($form, $form_state);
  };

  // Save any existing option sets.
  if (!empty($form_state['values']['variations']['option_sets'])) {
    foreach ($form_state['values']['variations']['option_sets'] as $option_set_values) {
      $option_set = $option_set_values['option_set'];
      switch ($option_set_values['option_set']->plugin) {
        case "block":
          $submitBlockHandler($option_set_values['content'], $option_set);
          break;
        case "element":
          break;
        default:
          // Unsupported option set type.
      }
      // Advanced form expects the label within its options.
      $option_set = personalize_option_set_load($option_set->osid);
      $option_set_values['advanced']['label'] = $option_set_values['content']['title'];
      personalize_campaign_wizard_submit_variations_advanced($option_set_values, $option_set);
      personalize_option_set_save($option_set);
    }
  }

  // Create any new option sets.
  if (!empty($form_state['values']['variations']['add_variation']['option_set_types']['option_set_type'])) {
    switch ($form_state['values']['variations']['add_variation']['option_set_types']['option_set_type']) {
      case "block":
        $submitBlockHandler($values['variations']['add_variation']['details']['block']['content']);
        break;
      case "element":
        break;
    }
  }
  // Clear out any redirects set by option set forms.
  unset($form_state['redirect']);
  // Set the clicked button ID back.
  $form_state['clicked_button']['#id'] = $click_id;
}

/**
 * Submit handler for entering a URL to begin creation an element variation.
 */
function acquia_lift_personalize_campaign_wizard_variations_submit_element_add($form, &$form_state) {
  // Set a session variable that we can check upon the next page load to
  // trigger element editing mode for either page or element variations.
  $agent_data = $form['#agent'];
  $agent = personalize_agent_load_agent($agent_data->machine_name);
  $element_type = $agent instanceof AcquiaLiftPageVariationInterface ? 'page' : 'element';
  $_SESSION['acquia_lift_element_trigger'] = $element_type;

  // Redirect to the requested URL to create an option set.
  $form_state['redirect'] = $form_state['values']['variations']['add_variation']['details']['element']['content']['url'];
}

/**
 * Submit handler to redirect to an element option set page for editing.
 */
function acquia_lift_personalize_campaign_wizard_variations_submit_element_edit($form, &$form_state) {
  // Value for the edit button is "edit_{osid}".
  list($prefix, $osid) = explode('_', $form_state['triggering_element']['#name']);
  $option_set = $form['variations']['option_sets']['option_set_' . $osid]['option_set']['#value'];
  $form_state['redirect'] = $option_set->data['pages'];
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

  // Save any visitor context if passed.
  // Add the visitor context configuration.
  $visitor_context = array();
  if (isset($form_state['values']['visitor_context'])) {
    $visitor_context = personalize_admin_convert_visitor_context_form_values($form_state['values']['visitor_context']);
  }
  $agent_data->data['visitor_context'] = $visitor_context;

  // Handle targeting audiences.
  if (empty($form_state['values']['audiences'])) {
    return;
  }
  module_load_include('inc', 'acquia_lift', 'acquia_lift.admin');
  $targeting = array();
  $audience_count = 0;
  // Determine the values for the "Everyone else" audience
  $final_audience = array(
    'name' => t('Everyone else'),
    'contexts' => array(),
    'weight' => 1,
    'strategy' => 'OR',
    'id' => ACQUIA_LIFT_TARGETING_EVERYONE_ELSE,
  );
  // Set the weight higher than any of the audience options.
  foreach($form_state['values']['audiences'] as $audience_id => $audience) {
    if ($audience_id === ACQUIA_LIFT_TARGETING_EVERYONE_ELSE || !isset($audience['details']['weight']) || empty($audience['details']['name'])) {
      continue;
    }
    $final_audience['weight'] = $audience['details']['weight'] > $final_audience['weight'] ? $audience['details']['weight'] : $final_audience['weight'];
  }
  $final_audience['weight'] += 10;
  // Now process all of the audiences.
  foreach ($form_state['values']['audiences'] as $audience_id => $audience) {
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
      if ($audience_id === 'add') {
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
    if (isset($audience['assignment'])) {
      $targeting[$audience_id] = array_values($audience['assignment']);
    }
  }
  // Create the "Everyone else" audience if there are other audiences and it
  // does not exist.
  if ($audience_count > 0 && !isset($form_state['values']['audiences'][ACQUIA_LIFT_TARGETING_EVERYONE_ELSE])) {
    acquia_lift_target_audience_save($final_audience['name'], $agent_data->machine_name, $final_audience['contexts'], $final_audience['strategy'], $final_audience['weight']);
  }
  acquia_lift_save_targeting_structure($agent_data, $targeting);
}

/**
 ********************************************************************
 *
 * H E L P E R S
 *
 ********************************************************************
 */

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
    $block_content = empty($values['variations']['add_variation']['details']['block']['content']) ? array() : $values['variations']['add_variation']['details']['block']['content'];
    $form_state_key = 'add';
  } else {
    $block_content = empty($values['variations']['option_sets']['option_set_' . $option_set->osid]['content']) ? array() : $values['variations']['option_sets']['option_set_' . $option_set->osid]['content'];
    $form_state_key = $option_set->osid;
  }

  module_load_include('inc', 'personalize_blocks', 'personalize_blocks.admin');
  module_load_include('inc', 'acquia_lift', 'acquia_lift.ui');

  _acquia_lift_personalize_campaign_wizard_form_state_blocks_alter($form_state, $block_content, $option_set);
  $element = personalize_blocks_form($element, $form_state, 'embed', $option_set);
  // This will add the current number of blocks and the personalized block
  // into the form state.  We move it into a more structured format to support
  // multiple blocks forms per page.
  $form_state['option_set_num_blocks'][$form_state_key] = $form_state['num_blocks'];
  $form_state['option_set_pblock'][$form_state_key] = $form_state['pblock'];
  // Set the form state values back.
  $form_state['values'] = $values;

  // The agent cannot be changed.
  $element['personalize']['#access'] = FALSE;
  $element['agent_select'] = array(
    '#type' => 'value',
    '#value' => $agent_data->machine_name,
  );
  // Make the variations wrapper a plain container rather than a fieldset.
  $element['pblock_wrapper']['#theme_wrappers'] = array('container');
  // Adjust the blocks container for our own user interface.
  unset($element['pblock_wrapper']['blocks']['#theme']);
  unset($element['pblock_wrapper']['#tree']);
  unset($element['pblock_wrapper']['blocks']['#tree']);

  $pblock_id = drupal_html_id('edit-' . implode('-', $parents) . '-pblock-wrapper');
  $more_id = $pblock_id . '-blocks-more';
  $blocks_id = $pblock_id . '-blocks';
  // Need to update the id so that it is unique on the page.
  $element['pblock_wrapper']['blocks']['#attributes']['id'] = $blocks_id;
  // @todo Give a default variation set name if not yet set, i.e., "Set 1".

  foreach (element_children($element['pblock_wrapper']['blocks']) as $key) {
    $block = &$element['pblock_wrapper']['blocks'][$key];
    // Set the option label as a revealing input and give it a label.
    $block['option_label']['#theme_wrappers'][] = 'acquia_lift_revealing_input';
    $block['option_label']['#title'] = _personalize_generate_option_index($key);
    // Weights are not manipulated here.
    $block['weight']['#access'] = FALSE;
    // Remove the link to add a new block from the drop-down options.
    unset($block['block']['bid']['#options']['add']);
    unset($block['block']['add']['#states']);
    // Hide the add form by default.
    $block['block']['add']['#collapsible'] = TRUE;
    $block['block']['add']['#collapsed'] = TRUE;
    $block['block']['add']['#title'] = t('Create new and add');

    acquia_lift_chosenify_element($block['block']['bid'], array('acquia-lift-chosen-select-half'));

    // Hide the remove button at this point.
    $block['remove']['#access'] = FALSE;
  }
  // Update the "Add another" button to match the desired styling and language.
  $element['pblock_wrapper']['#attributes']['id'] = $pblock_id;
  $element['pblock_wrapper']['blocks_more']['#tag'] = 'button';
  $element['pblock_wrapper']['blocks_more']['#text'] = t('Add variation');
  $element['pblock_wrapper']['blocks_more']['#value'] = 'blocks_' . (empty($option_set) ? 'add' : $option_set->osid);
  $element['pblock_wrapper']['blocks_more']['#theme_wrappers'] = array('personalize_html_tag');
  $element['pblock_wrapper']['blocks_more']['#attributes']['id'] = $more_id;
  $element['pblock_wrapper']['blocks_more']['#attributes']['class'][] = 'personalize-add-link';
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
 * @param $option_set
 *   The currently referenced option set data.
 */
function _acquia_lift_personalize_campaign_wizard_form_state_blocks_alter(&$form_state, $block_content = array(), $option_set = NULL) {
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

  $key = empty($option_set) ? 'add' : $option_set->osid;
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
  $element = array();
  if (empty($option_set->osid)) {
    $element['url'] = array(
      '#type' => 'textfield',
      '#element_validate' => array('personalize_form_element_path_validate'),
      '#title' => t('Page where element exists'),
      '#allow_dynamic' => FALSE,
      '#allow_external' => TRUE,
      '#required' => TRUE,
    );
    $element['navigate'] = array(
      '#type' => 'submit',
      '#submit' => array('acquia_lift_personalize_campaign_wizard_variations_submit_element_add'),
      '#value' => t('Go'),
    );
    return $element;
  }
  // Form for an existing option set.
  $element['label'] = array(
    '#type' => 'textfield',
    '#required' => TRUE,
    '#theme_wrappers' => array('acquia_lift_revealing_input'),
    '#default_value' => $option_set->label,
    '#title' => t('Variation set')
  );
  if (!empty($option_set->data['pages'])) {
    // Note that utilizing the personalize_html_tag theme here somehow prevents
    // the element and form-altered submit handlers from being called.
    $element['edit'] = array(
      '#type' => 'submit',
      '#submit' => array('acquia_lift_personalize_campaign_wizard_variations_submit_element_edit'),
      '#value' => t('Edit'),
      '#name' => 'edit_' . $option_set->osid,
    );
  }
  return $element;
}

/**
 * Helper function to generate an audience sub-form.
 *
 * @param $agent_data
 *   The current agent data.
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
function _acquia_lift_personalize_campaign_wizard_targeting_audience(&$form, &$form_state, $agent_data, $parent_identifier, $audience = NULL) {
// Build up a list of available context values for targeting.
  module_load_include('inc', 'personalize', 'personalize.admin');
  $targeting_values = personalize_get_targeting_values_for_agent($agent_data);
  $element = array(
    '#type' => 'container',
  );
  $element['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Audience name'),
    '#default_value' => !empty($audience['label']) ? $audience['label'] : '',
    '#required' => !empty($audience),
  );
  $element['weight'] = array(
    '#type' => 'textfield',
    '#title' => t('Weight'),
    '#default_value' => !empty($audience['weight']) ? $audience['weight'] : 50,
    '#size' => 3,
    '#element_validate' => array('element_validate_number'),
  );

  // This is the portion of the form that will be replace when the "add new" or "remove
  // context" links are clicked.
  $main_wrapper_id = 'acquia-lift-targeting-' . $parent_identifier;

  $element['mapping'] = array(
    '#tree' => TRUE,
    '#theme_wrappers' => array('container'),
  );
  $element['mapping']['contexts'] = array(
    '#type' => 'container',
    '#attributes' => array(
      'id' => $main_wrapper_id,
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
  if (isset($form_state['values']['audiences'][$parent_identifier]['details']['mapping']['contexts'])) {
    foreach($form_state['values']['audiences'][$parent_identifier]['details']['mapping']['contexts'] as $delta => $context) {
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
  // If the "Remove" button was clicked for a context, we need to remove that context
  // from the form.
  if (isset($form_state['to_remove_audience'][$parent_identifier])) {
    unset($mappings[$form_state['to_remove_audience'][$parent_identifier]]);
    unset($form_state['to_remove_audience'][$parent_identifier]);
    $form_state['num_contexts_audience'][$parent_identifier]--;
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
  if (isset($form_state['num_contexts_audience'][$parent_identifier]) && $form_state['num_contexts_audience'][$parent_identifier] > $num_contexts) {
    while ($num_contexts < $form_state['num_contexts_audience'][$parent_identifier]) {
      $mappings[] = array(
        'context' => '',
        'operator' => 'equals',
        'match' => '',
        'plugin' => '',
      );
      $num_contexts++;
    }
  }
  $form_state['num_contexts_audience'][$parent_identifier] = count($mappings);

  foreach ($mappings as $delta => $mapping) {
    $element['mapping']['contexts'][$delta] = personalize_explicit_targeting_mapping_element($mapping, $targeting_values, $delta);

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
      '#attributes' => array(
        'class' => array('personalize-delete-context', 'form-submit',),
        'title' => t('Delete this context.'),
        'id' => 'edit-audiences-' . $parent_identifier . '-details-mapping-contexts-' . $delta . '-remove',
      ),
      '#submit' => array('acquia_lift_personalize_campaign_wizard_targeting_remove'),
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
    '#value' => 'add_context_' . $parent_identifier,
    '#theme_wrappers' => array('personalize_html_tag'),
    '#submit' => array('acquia_lift_personalize_campaign_wizard_targeting_add'),
    '#attributes' => array(
      'class' => array('personalize-add-link'),
      'title' => t('Click here to add more contexts.'),
      'id' => 'edit-audiences-' . $parent_identifier . '-details-add-new'
    ),
    '#ajax' => array(
      'callback' => 'acquia_lift_personalize_campaign_wizard_targeting_ajax',
      'wrapper' => $main_wrapper_id,
      'effect' => 'fade',
    ),
  );
  $element['strategies'] = array(
    '#type' => 'container',
  );

  // Add radio buttons so the user can select how multiple features for an option
  // should be treated.
  $default_strategy = 'OR';
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
  return $element;
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
  unset($option_set->targeting[$audience_id]);
  // If this leaves only the "Everyone else" audience, then delete it too.
  if (count($option_set->targeting) === 1 && array_key_exists(ACQUIA_LIFT_TARGETING_EVERYONE_ELSE, $option_set->targeting)) {
    unset($option_set->targeting[ACQUIA_LIFT_TARGETING_EVERYONE_ELSE]);
  }
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
    t('Are you sure you want to revert the targeting changes for the %agent campaign?',
      array('%agent' => $agent->label)),
    'admin/structure/personalize/' . $agent->machine_name . '/targeting',
    t('Reverting the targeting will discard all changes since the campaign was set to running.'),
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
