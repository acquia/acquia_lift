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
use Drupal\taxonomy\Entity\Vocabulary;
use Drupal\acquia_lift\Entity\Credential;

/**
 * Defines a form that configures settings.
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
    $credential = new Credential($config);

    $form['credential'] = $this->buildCredentialForm($credential);
    $form['identity'] = $this->buildIdentityForm($config);
    $form['field_mappings'] = $this->buildFieldMappingsForm($config);

    return parent::buildForm($form, $form_state);
  }

  /**
   * Build credential form.
   *
   * @param \Drupal\acquia_lift\Entity\Credential $credential
   *   Acquia Lift Config.
   *
   * @return array
   *   Credential form.
   */
  private function buildCredentialForm(Credential $credential) {
    $form = array(
      '#title' => t('Credential'),
      '#type' => 'details',
      '#tree' => TRUE,
      '#open' => !$credential->isValid(),
    );
    $form['account_name'] = array(
      '#type' => 'textfield',
      '#title' => t('Account Name'),
      '#default_value' => $credential->getAccountName(),
      '#required' => TRUE,
    );
    $form['customer_site'] = array(
      '#type' => 'textfield',
      '#title' => t('Customer Site'),
      '#default_value' => $credential->getCustomerSite(),
    );
    $form['api_url'] = array(
      '#type' => 'textfield',
      '#title' => t('API URL'),
      '#field_prefix' => 'http(s)://',
      '#default_value' => $credential->getApiUrl(),
      '#required' => TRUE,
    );
    $form['access_key'] = array(
      '#type' => 'textfield',
      '#title' => t('API Access Key'),
      '#default_value' => $credential->getAccessKey(),
      '#required' => TRUE,
    );
    $form['secret_key'] = array(
      '#type' => 'password',
      '#title' => t('API Secret Key'),
      '#default_value' => $credential->getSecretKey(),
      '#required' => empty($credential->getSecretKey()),
      '#description' => !empty($credential->getSecretKey()) ? t('Only necessary if updating') : '',
    );
    $form['js_path'] = array(
      '#type' => 'textfield',
      '#title' => t('JavaScript Path'),
      '#field_prefix' => 'http(s)://',
      '#default_value' => $credential->getJsPath(),
      '#required' => TRUE,
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
      '#type' => 'details',
      '#tree' => TRUE,
      '#open' => TRUE,
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
   * Build field mappings form.
   *
   * @param \Drupal\Core\Config\Config $config
   *   Acquia Lift Config.
   *
   * @return array
   *   Field mappings form.
   */
  private function buildFieldMappingsForm(Config $config) {
    $vocabularies = Vocabulary::loadMultiple();
    $vocabulary_options = array();
    foreach ($vocabularies as $vocabulary_vid => $vocabulary) {
      $vocabulary_options[$vocabulary_vid] = $vocabulary->label();
    }

    $form = array(
      '#title' => t('Field Mappings'),
      '#type' => 'details',
      '#tree' => TRUE,
      '#open' => TRUE,
    );
    $form['content_section'] = array(
      '#type' => 'select',
      '#title' => t('Content Section'),
      '#empty_value' => '',
      '#options' => $vocabulary_options,
      '#default_value' => $config->get('content_section'),
    );
    $form['content_keywords'] = array(
      '#type' => 'select',
      '#title' => t('Content Keywords'),
      '#empty_value' => '',
      '#options' => $vocabulary_options,
      '#default_value' => $config->get('content_keywords'),
    );
    $form['persona'] = array(
      '#type' => 'select',
      '#title' => t('Persona'),
      '#empty_value' => '',
      '#options' => $vocabulary_options,
      '#default_value' => $config->get('persona'),
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
    $this->setFieldMappingsValues($config, $values['field_mappings']);

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
    $config->set('account_name', $values['account_name']);
    $config->set('customer_site', $values['customer_site']);
    $config->set('api_url', $values['api_url']);
    $config->set('access_key', $values['access_key']);
    if (!empty($values['secret_key'])) {
      $config->set('secret_key', $values['secret_key']);
    }
    $config->set('js_path', $values['js_path']);
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

  /**
   * Set field mapping values.
   *
   * @param \Drupal\Core\Config\Config $config
   *   Acquia Lift Config.
   * @param array $values
   *   Field mappings values.
   */
  private function setFieldMappingsValues(Config $config, array $values) {
    $config->set('content_section', $values['content_section']);
    $config->set('content_keywords', $values['content_keywords']);
    $config->set('persona', $values['persona']);
  }
}
