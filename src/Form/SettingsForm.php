<?php

namespace Drupal\acquia_perz\Form;

use Drupal\Core\Form\ConfigFormBase;
use Drupal\Core\Form\FormStateInterface;

/**
 * Defines the form to configure the Content Index Service connection settings.
 */
class SettingsForm extends ConfigFormBase {

  /**
   * {@inheritdoc}
   */
  public function getFormId() {
    return 'acquia_perz_settings_form';
  }

  /**
   * {@inheritdoc}
   */
  protected function getEditableConfigNames() {
    return ['acquia_perz.settings'];
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state) {
    $settings = $this
      ->config('acquia_perz.settings');

    $form['all_uuids_decision_endpoint'] = [
      '#type' => 'textfield',
      '#title' => $this->t('All Uuids decision endpoint'),
      '#default_value' => $settings->get('all_uuids_decision_endpoint'),
      '#required' => TRUE,
    ];
    $form['uuids_slots_decision_endpoint'] = [
      '#type' => 'textfield',
      '#title' => $this->t('Uuids Slots endpoint'),
      '#default_value' => $settings->get('uuids_slots_decision_endpoint'),
      '#required' => TRUE,
    ];

    $form['slot1_id'] = [
      '#type' => 'textfield',
      '#title' => $this->t('Slot 1 ID'),
      '#default_value' => $settings->get('slot1_id'),
      '#required' => TRUE,
    ];
    $form['slot2_id'] = [
      '#type' => 'textfield',
      '#title' => $this->t('Slot 2 ID'),
      '#default_value' => $settings->get('slot2_id'),
      '#required' => TRUE,
    ];
    $form['slot3_id'] = [
      '#type' => 'textfield',
      '#title' => $this->t('Slot 3 ID'),
      '#default_value' => $settings->get('slot3_id'),
      '#required' => TRUE,
    ];
    return parent::buildForm($form, $form_state);
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    $settings = $this->config('acquia_perz.settings');
    $values = $form_state->getValues();
    $settings->set('all_uuids_decision_endpoint', trim($values['all_uuids_decision_endpoint']));
    $settings->set('uuids_slots_decision_endpoint', trim($values['uuids_slots_decision_endpoint']));
    $settings->set('slot1_id', trim($values['slot1_id']));
    $settings->set('slot2_id', trim($values['slot2_id']));
    $settings->set('slot3_id', trim($values['slot3_id']));
    $settings->save();
    parent::submitForm($form, $form_state);
  }

}
