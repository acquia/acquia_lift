<?php
/**
 * @file acquia_lift.admin.wizard.inc Functions specific to the Acquia Lift
 * alteration of the campaign creation wizard.
 */

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

>>>>>>> Move process bar hook changes into their own callback.
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
  $form['#submit'] = array('acquia_lift_personalize_campaign_wizard_variations_submit');

  $form['#attached']['css'][] = drupal_get_path('module', 'acquia_lift') . '/css/acquia_lift.admin.css';

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
        _acquia_lift_personalize_campaign_wizard_variations_block($form['variations']['option_sets']['option_set_' . $option_set->osid]['content'], $form_state, $agent_data, array('variations', 'option_sets', 'option_set_' . $option_set->osid, 'content'), $option_set);
        break;
      case 'elements':
        _acquia_lift_personalize_campaign_wizard_variations_element($form['variations']['option_sets']['option_set_' . $option_set->osid]['content'], $form_state, $agent_data, $option_set);
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
        _acquia_lift_personalize_campaign_wizard_variations_block($form['variations']['add_variation']['details']['block']['content'], $form_state, $agent_data, array('variations', 'add_variation', 'details', 'block', 'content'));
        break;

      case 'element':
        $form['variations']['add_variation']['details']['element'] = array(
          '#type' => 'container',
          '#attributes' => array(
            'class' => array('acquia-lift-element-variation-set'),
          ),
        );
        _acquia_lift_personalize_campaign_wizard_variations_element($form['variations']['add_variation']['details']['element']['content'], $form_state, $agent_data);
        break;
    }
  }

  // Put the form state back the way it was for the next steps in form processing.
  $form_state['values'] = $values;
}

/**
 * Alter hook for the targeting portion of the campaign wizard.
 */
function acquia_lift_personalize_campaign_wizard_targeting_alter(&$form, &$form_state, $form_id) {
  if (!empty($form['visitor_context']) && $form['visitor_context']['#type'] == 'select') {
    module_load_include('inc', 'acquia_lift', 'acquia_lift.ui');
    acquia_lift_chosenify_element($form['visitor_context']);
  }
}

/**
 ********************************************************************
 *
 * A J A X  C A L L B A C K S
 *
 ********************************************************************
 */

/**
 * Submit handler to add another block option to a personalized block form.
 */
function acquia_lift_personalize_campaign_wizard_variations_ajax_add(&$form, &$form_state) {
  return $form['variations']['add_variation']['details'];
}

/**
 * Submit handler to add another block option to a personalized block form.
 */
function acquia_lift_personalize_campaign_wizard_blocks_add(&$form, &$form_state) {
  // Value will be "blocks_add" or "blocks_{osid}" where the second part is the
  // key within the form state storage for the number of blocks to generate.
  list($blocks, $key) = explode('_', $form_state['triggering_element']['#value']);
  $form_state['option_set_num_blocks'][$key]++;
  $form_state['rebuild'] = TRUE;
}

/**
 * Callback to for AJAX to generate the personalize block wrapper form.
 */
function acquia_lift_personalize_campaign_wizard_blocks_ajax(&$form, &$form_state) {
  // The triggering element and the blocks container share a common parent.
  $parents = $form_state['triggering_element']['#array_parents'];
  $section = $form;
  for ($i = 0; $i < count($parents) -1; $i++) {
    $section = $section[$parents[$i]];
  }
  return $section['blocks'];
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
 ********************************************************************
 *
 * S U B M I S S I O N S
 *
 ********************************************************************
 */

/**
 * Submit function for variations form.
 */
function acquia_lift_personalize_campaign_wizard_variations_submit(&$form, &$form_state) {
  module_load_include('inc', 'personalize', 'personalize.admin');
  module_load_include('inc', 'personalize', 'personalize.admin.campaign');
  // Save a reference to the full form state values.
  $values = $form_state['values'];
  $click_id = $form_state['clicked_button']['#id'];

  // Run the base submit form handling.
  $agent = personalize_campaign_wizard_submit_base($form, $form_state);

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

  // If user selected to add another variation, then return to the variations step.

  // Otherwise the next step is "why".
  $form_state['storage']['step'] = 'goals';
  $form_state['rebuild'] = TRUE;
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
 */
function _acquia_lift_personalize_campaign_wizard_variations_block(&$form, &$form_state, $agent_data, $parents, $option_set = NULL) {
  $form = array();
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
  $form = personalize_blocks_form($form, $form_state, 'embed', $option_set);
  // This will add the current number of blocks and the personalized block
  // into the form state.  We move it into a more structured format to support
  // multiple blocks forms per page.
  $form_state['option_set_num_blocks'][$form_state_key] = $form_state['num_blocks'];
  $form_state['option_set_pblock'][$form_state_key] = $form_state['pblock'];
  // Set the form state values back.
  $form_state['values'] = $values;

  // The agent cannot be changed.
  $form['personalize']['#access'] = FALSE;
  $form['agent_select'] = array(
    '#type' => 'value',
    '#value' => $agent_data->machine_name,
  );
  // Make the variations wrapper a plain container rather than a fieldset.
  $form['pblock_wrapper']['#theme_wrappers'] = array('container');
  // Adjust the blocks container for our own user interface.
  unset($form['pblock_wrapper']['blocks']['#theme']);
  unset($form['pblock_wrapper']['#tree']);
  unset($form['pblock_wrapper']['blocks']['#tree']);

  $pblock_id = drupal_html_id('edit-' . implode('-', $parents) . '-pblock-wrapper');
  $more_id = $pblock_id . '-blocks-more';
  $blocks_id = $pblock_id . '-blocks';
  // Need to update the id so that it is unique on the page.
  $form['pblock_wrapper']['blocks']['#attributes']['id'] = $blocks_id;
  // @todo Give a default variation set name if not yet set, i.e., "Set 1".

  foreach (element_children($form['pblock_wrapper']['blocks']) as $key) {
    $block = &$form['pblock_wrapper']['blocks'][$key];
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
  $form['pblock_wrapper']['#attributes']['id'] = $pblock_id;
  $form['pblock_wrapper']['blocks_more']['#tag'] = 'button';
  $form['pblock_wrapper']['blocks_more']['#text'] = t('Add variation');
  $form['pblock_wrapper']['blocks_more']['#value'] = 'blocks_' . (empty($option_set) ? 'add' : $option_set->osid);
  $form['pblock_wrapper']['blocks_more']['#theme_wrappers'] = array('personalize_html_tag');
  $form['pblock_wrapper']['blocks_more']['#attributes']['id'] = $more_id;
  $form['pblock_wrapper']['blocks_more']['#attributes']['class'][] = 'personalize-add-link';
  $form['pblock_wrapper']['blocks_more']['#prefix'] = '<span class="personalize-add-link-prefix"></span>';
  $form['pblock_wrapper']['blocks_more']['#submit'] = array('acquia_lift_personalize_campaign_wizard_blocks_add');
  $form['pblock_wrapper']['blocks_more']['#ajax'] = array(
    'callback' => 'acquia_lift_personalize_campaign_wizard_blocks_ajax',
    'wrapper' => $blocks_id,
    'effect' => 'fade',
  );
  // Remove the form's action buttons
  unset($form['actions']);
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
 *
 * NOTE: limit_validation_errors is not set for either of this submit buttons
 * due to a requirement to actually save the page content before exiting for
 * element variation actions.
 */
function _acquia_lift_personalize_campaign_wizard_variations_element(&$form, &$form_state, $agent_data, $option_set = NULL) {
  if (empty($option_set->osid)) {
    $form['url'] = array(
      '#type' => 'textfield',
      '#element_validate' => array('personalize_form_element_path_validate'),
      '#title' => t('Page where element exists'),
      '#allow_dynamic' => FALSE,
      '#allow_external' => TRUE,
      '#required' => TRUE,
    );
    $form['navigate'] = array(
      '#type' => 'submit',
      '#submit' => array('acquia_lift_personalize_campaign_wizard_variations_submit_element_add'),
      '#value' => t('Go'),
    );
    return;
  }
  // Form for an existing option set.
  $form['label'] = array(
    '#type' => 'textfield',
    '#required' => TRUE,
    '#theme_wrappers' => array('acquia_lift_revealing_input'),
    '#default_value' => $option_set->label,
    '#title' => t('Variation set')
  );
  if (!empty($option_set->data['pages'])) {
    // Note that utilizing the personalize_html_tag theme here somehow prevents
    // the element and form-altered submit handlers from being called.
    $form['edit'] = array(
      '#type' => 'submit',
      '#submit' => array('acquia_lift_personalize_campaign_wizard_variations_submit_element_edit'),
      '#value' => t('Edit'),
      '#name' => 'edit_' . $option_set->osid,
    );
  }
}
