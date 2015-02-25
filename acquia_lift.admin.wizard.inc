<?php
/**
 * @file personalize.admin.campaign.inc Functions specific to the Acquia Lift
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

>>>>>>> Move process bar hook changes into their own callback.
/**
 * Alter hook for the variations portions of the campaign wizard.
 */
function acquia_lift_personalize_campaign_wizard_variations_alter(&$form, &$form_state, $form_id) {
  // Rebuild the variations form to show the customized Lift approach.
  unset($form['variations']['title']['summary']);
  unset($form['variations']['option_sets']);

  $form['#attached']['css'][] = drupal_get_path('module', 'acquia_lift') . '/css/acquia_lift.admin.css';

  $agent_data = $form['#agent'];
  if (empty($agent_data->machine_name)) {
    return;
  }
  $option_sets = personalize_option_set_load_by_agent($agent_data->machine_name);

  // Make a container to hold existing option sets.
  if (!empty($option_sets)) {
    $form['variations']['option_sets'] = array(
      '#tree' => TRUE,
      '#type' => 'container',
      '#attributes' => array(
        'class' => array('personalize-wizard-variation-sets'),
      ),
    );
  }
  // Add an option set edit card for each option set.
  foreach ($option_sets as $option_set) {
    $form['variations']['option_sets']['option_set_' . $option_set->osid] = array(
      '#tree' => TRUE,
      '#type' => 'container',
      '#attributes' => array(
        'class' => array('personalize-option-set'),
      ),
    );
    $form['variations']['option_sets']['option_set_' . $option_set->osid]['content'] = array(
      '#markup' => 'Option set form for type: ' . $option_set->plugin,
    );
  }

  // Show the card to add a new option set last.
  module_load_include('inc', 'acquia_lift', 'acquia_lift.ui');
  $option_set_types = acquia_lift_option_set_types_ui();
  foreach ($option_set_types as $type => $details) {
    $options[$type] = $details['title'];
  }
  $form['variations']['add_variation'] = array(
    '#type' => 'container',
    '#theme' => 'acquia_lift_card',
    '#collapsed' => !empty($option_sets),
    '#title' => t('Add variation set'),
    '#collapsible' => TRUE,
  );
  // Tricky: we need this container in order to hide the radio list when a
  // selection is made.  The radios type does not have a proper wrapping
  // element to apply visible/invisible states.
  $form['variations']['add_variation']['option_set_type_container'] = array(
    '#type' => 'container',
    '#attributes' => array(
      'class' => array('form-item'),
    ),
    '#states' => array(
      'invisible' => array(
        ':input[name="option_set_type"]' => array('!value' => false),
      ),
    )
  );
  $form['variations']['add_variation']['option_set_type_container']['option_set_type'] = array(
    '#type' => 'radios',
    '#options' => $options,
    '#default_value' => '',
    '#required' => TRUE,
    '#theme' => 'acquia_lift_radio_list',
    '#ajax' => array(
      'callback' => 'acquia_lift_personalize_campaign_wizard_variations_ajax_add',
      'wrapper' => 'acquia-lift-add-variation-details',
      'effect' => 'fade',
    ),
  );
  foreach ($option_set_types as $type => $details) {
    $form['variations']['add_variation']['option_set_type_container']['option_set_type'][$type]['#description'] = $details['description'];
    $form['variations']['add_variation']['option_set_type_container']['option_set_type'][$type]['#image'] = $details['logo'];
  }

  $form['variations']['add_variation']['details'] = array(
    '#type' => 'container',
    '#attributes' => array(
      'id' => 'acquia-lift-add-variation-details',
    ),
  );
  if (isset($form_state['values']['option_set_type'])) {
    switch ($form_state['values']['option_set_type']) {
      case 'block':
        $form['variations']['add_variation']['details']['block'] = array(
          '#type' => 'container',
          '#attributes' => array(
            'class' => array('acquia-lift-block-variation-set'),
          ),
        );
        $form['variations']['add_variation']['details']['block']['content'] = array(
          '#markup' => 'Block form here',
        );
        break;

      case 'element':
        $form['variations']['add_variation']['details']['element'] = array(
          '#type' => 'container',
          '#attributes' => array(
            'class' => array('acquia-lift-element-variation-set'),
          ),
        );
        $form['variations']['add_variation']['details']['element']['content'] = array(
          '#markup' => 'Element form here',
        );
        break;
    }
  }
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
