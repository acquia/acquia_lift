<?php

namespace Drupal\acquia_perz\Form;

use Drupal\Core\Form\ConfigFormBase;
use Drupal\Core\Form\FormStateInterface;

/**
 * Defines the form to configure the Content Index Service connection settings.
 */
class CISSettingsForm extends ConfigFormBase {

  /**
   * {@inheritdoc}
   */
  public function getFormId() {
    return 'acquia_perz_cis_settings';
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
    $cis_settings = $settings->get('cis');

    $form['cis'] = [
      '#tree' => TRUE,
    ];
    $form['cis']['endpoint'] = [
      '#type' => 'textfield',
      '#title' => $this->t('Endpoint'),
      '#default_value' => $cis_settings['endpoint'],
      '#required' => TRUE,
    ];
    $form['cis']['account_id'] = [
      '#type' => 'textfield',
      '#title' => $this->t('Account ID'),
      '#default_value' => $cis_settings['account_id'],
      '#required' => TRUE,
    ];
    $form['cis']['environment'] = [
      '#type' => 'textfield',
      '#title' => $this->t('Environment'),
      '#default_value' => $cis_settings['environment'],
      '#required' => TRUE,
    ];
    $form['cis']['origin'] = [
      '#type' => 'textfield',
      '#title' => $this->t('Origin'),
      '#default_value' => $cis_settings['origin'],
      '#required' => TRUE,
    ];
    $form['cis']['api_key'] = [
      '#type' => 'textfield',
      '#title' => $this->t('API Key'),
      '#default_value' => $cis_settings['api_key'],
      '#required' => TRUE,
    ];
    return parent::buildForm($form, $form_state);
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    $settings = $this->config('acquia_perz.settings');
    $values = $form_state->getValues()['cis'];
    $settings->set('cis.endpoint', trim($values['endpoint']));
    $settings->set('cis.account_id', trim($values['account_id']));
    $settings->set('cis.environment', trim($values['environment']));
    $settings->set('cis.api_key', trim($values['api_key']));
    $settings->set('cis.origin', trim($values['origin']));
    $settings->save();
    parent::submitForm($form, $form_state);
  }

}
