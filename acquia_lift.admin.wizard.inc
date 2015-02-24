<?php
/**
 * @file personalize.admin.campaign.inc Functions specific to the Acquia Lift
 * alteration of the campaign creation wizard.
 */

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

  foreach ($option_sets as $option_set) {

  }

  // Form to add a new option set.
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
  );
  foreach ($option_set_types as $type => $details) {
    $form['variations']['add_variation']['option_set_type_container']['option_set_type'][$type]['#description'] = $details['description'];
    $form['variations']['add_variation']['option_set_type_container']['option_set_type'][$type]['#image'] = $details['logo'];
  }

  // Block form
  $form['variations']['add_variation']['block'] = array(
    '#type' => 'container',
    '#attributes' => array(
      'class' => array('acquia-lift-block-variation-set'),
    ),
    '#states' => array(
      'visible' => array(
        'input[name="option_set_type"]' => array('value' => 'block'),
      ),
    ),
  );
  $form['variations']['add_variation']['block']['content'] = array(
    '#markup' => 'Block form here',
  );

  // Element form
  $form['variations']['add_variation']['element'] = array(
    '#type' => 'container',
    '#attributes' => array(
      'class' => array('acquia-lift-element-variation-set'),
    ),
    '#states' => array(
      'visible' => array(
        'input[name="option_set_type"]' => array('value' => 'element'),
      ),
    ),
  );
  $form['variations']['add_variation']['element']['content'] = array(
    '#markup' => 'Element form here',
  );
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
