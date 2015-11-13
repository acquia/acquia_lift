<?php

/**
 * @file
 * Contains \Drupal\acquia_lift\Form\Settings.
 */

namespace Drupal\acquia_lift\Form;

use Drupal\Core\Config\Config;
use Drupal\Core\Form\ConfigFormBase;
use Drupal\Core\Form\FormStateInterface;
use Symfony\Component\HttpFoundation\Request;

/**
 * Defines a form that configures devel settings.
 */
class Settings extends ConfigFormBase {
  /**
   * {@inheritdoc}
   */
  public function getFormId() {
    return 'acquia_lift_settings_form';
  }

  /**
   * {@inheritdoc}
   */
  protected function getEditableConfigNames() {
    return [
      'acquia_lift.settings',
    ];
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state, Request $request = NULL) {
    $config = $this->config('acquia_lift.settings');

    $form['credential'] = $this->buildCredentialForm($config);
    $form['identity'] = $this->buildIdentityForm($config);

    return parent::buildForm($form, $form_state);
  }

  /**
   * Build credential form.
   *
   * @param \Drupal\Core\Config\Config $config
   *   Acquia Lift Config.
   *
   * @return array
   *   Credential form.
   */
  private function buildCredentialForm(Config $config) {
    $form = array(
      '#title' => t('Credential'),
      '#type' => 'fieldset',
      '#tree' => TRUE,
    );
    $form['account_name'] = array(
      '#type' => 'textfield',
      '#title' => t('Account Name'),
      '#default_value' => $config->get('account_name'),
      '#required' => TRUE,
    );
    $form['customer_site'] = array(
      '#type' => 'textfield',
      '#title' => t('Customer Site'),
      '#default_value' => $config->get('customer_site'),
    );
    $form['api_url'] = array(
      '#type' => 'textfield',
      '#title' => t('API URL'),
      '#field_prefix' => 'http(s)://',
      '#default_value' => $config->get('api_url'),
      '#required' => TRUE,
    );
    $form['access_key'] = array(
      '#type' => 'textfield',
      '#title' => t('API Access Key'),
      '#default_value' => $config->get('access_key'),
      '#required' => TRUE,
    );
    $form['secret_key'] = array(
      '#type' => 'password',
      '#title' => t('API Secret Key'),
      '#default_value' => $config->get('secret_key'),
      '#required' => empty($config->get('secret_key')),
      '#description' => !empty($config->get('secret_key')) ? t('Only necessary if updating') : '',
    );

    return $form;
  }

  /**
   * Build identity form.
   *
   * @param \Drupal\Core\Config\Config $config
   *   Acquia Lift Config.
   *
   * @return array
   *   Identity form.
   */
  private function buildIdentityForm(Config $config) {
    $form = array(
      '#title' => t('Identity'),
      '#type' => 'fieldset',
      '#tree' => TRUE,
    );
    $form['capture_identity'] = array(
      '#type' => 'checkbox',
      '#title' => t('Capture identity on login / register'),
      '#default_value' => $config->get('capture_identity'),
    );
    $form['identity_parameter'] = array(
      '#type' => 'textfield',
      '#title' => t('Identity Parameter'),
      '#default_value' => $config->get('identity_parameter'),
    );
    $form['identity_type_parameter'] = array(
      '#type' => 'textfield',
      '#title' => t('Identity Type Parameter'),
      '#default_value' => $config->get('identity_type_parameter'),
    );
    $form['default_identity_type'] = array(
      '#type' => 'textfield',
      '#title' => t('Default Identity Type'),
      '#default_value' => $config->get('default_identity_type'),
    );

    return $form;
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    $config = $this->config('acquia_lift.settings');
    $values = $form_state->getValues();

    $this->setCredentialValues($config, $values['credential']);
    $this->setIdentityValues($config, $values['identity']);

    $config->save();

    parent::submitForm($form, $form_state);
  }

  /**
   * Set credential values.
   *
   * @param \Drupal\Core\Config\Config $config
   *   Acquia Lift Config.
   * @param array $values
   *   Credential values.
   */
  private function setCredentialValues(Config $config, array $values) {
    if (!empty($values['secret_key'])) {
      $config->set('secret_key', $values['secret_key']);
    }
    $config->set('account_name', $values['account_name']);
    $config->set('customer_site', $values['customer_site']);
    $config->set('api_url', $values['api_url']);
    $config->set('access_key', $values['access_key']);
  }

  /**
   * Set identity values.
   *
   * @param \Drupal\Core\Config\Config $config
   *   Acquia Lift Config.
   * @param array $values
   *   Identity values.
   */
  private function setIdentityValues(Config $config, array $values) {
    $config->set('capture_identity', $values['capture_identity']);
    $config->set('identity_parameter', $values['identity_parameter']);
    $config->set('identity_type_parameter', $values['identity_type_parameter']);
    $config->set('default_identity_type', $values['default_identity_type']);
  }
}
