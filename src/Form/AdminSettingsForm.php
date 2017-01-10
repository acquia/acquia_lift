<?php

namespace Drupal\acquia_lift\Form;

use Exception;
use Symfony\Component\DependencyInjection\ContainerInterface;
use Drupal\Core\Config\Config;
use Drupal\Core\Entity\EntityManagerInterface;
use Drupal\node\Entity\NodeType;
use Drupal\Core\Form\ConfigFormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\Core\Link;
use Drupal\Core\Url;
use Drupal\acquia_lift\Service\Helper\SettingsHelper;

/**
 * Defines a form that configures settings.
 */
class AdminSettingsForm extends ConfigFormBase {
  /**
   * The entity manager.
   *
   * @var \Drupal\Core\Entity\EntityManagerInterface
   */
  private $entityManager;

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
   * Constructs an AdminSettingsForm object.
   *
   * @param \Drupal\Core\Entity\EntityManagerInterface $entity_manager
   *   The entity manager.
   */
  public function __construct(EntityManagerInterface $entity_manager) {
    $this->entityManager = $entity_manager;
  }

  /**
   * {@inheritdoc}
   */
  public static function create(ContainerInterface $container) {
    return new static(
      $container->get('entity.manager')
    );
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state) {
    $this->preValidateData();

    $form['credential'] = $this->buildCredentialForm();

    // Data collection settings.
    $form['data_collection_settings'] = [
      '#type' => 'vertical_tabs',
      '#title' => t('Data collection settings'),
    ];
    $form['identity'] = $this->buildIdentityForm();
    $form['field_mappings'] = $this->buildFieldMappingsForm();
    $form['udf_person_mappings'] = $this->buildUdfMappingsForm('person');
    $form['udf_touch_mappings'] = $this->buildUdfMappingsForm('touch');
    $form['udf_event_mappings'] = $this->buildUdfMappingsForm('event');
    $form['visibility'] = $this->buildVisibilityForm();
    $form['advanced_configuration'] = $this->buildAdvancedConfigurationForm();

    return parent::buildForm($form, $form_state);
  }

  /**
   * Pre-validate data.
   */
  private function preValidateData() {
    $credential_settings = $this->config('acquia_lift.settings')->get('credential');

    // Validate the essential fields.
    if (SettingsHelper::isInvalidCredentialAccountId($credential_settings['account_id']) ||
      SettingsHelper::isInvalidCredentialSiteId($credential_settings['site_id']) ||
      SettingsHelper::isInvalidCredentialAssetsUrl($credential_settings['assets_url'])
    ) {
      drupal_set_message(t('The Acquia Lift module requires a valid Account ID, Site ID, and Assets URL to complete activation.'), 'warning');
    }

    // Validate URLs and check connections.
    if (isset($credential_settings['decision_api_url']) && SettingsHelper::isInvalidCredentialDecisionApiUrl($credential_settings['decision_api_url']) ||
      isset($credential_settings['oauth_url']) && SettingsHelper::isInvalidCredentialOauthUrl($credential_settings['oauth_url'])
    ) {
      drupal_set_message(t('Acquia Lift module requires valid Decision API URL and Authentication URL to be activate.'), 'warning');
    }
  }

  /**
   * Build credential form.
   *
   * @return array
   *   Credential form.
   */
  private function buildCredentialForm() {
    $credential_settings = $this->config('acquia_lift.settings')->get('credential');

    $form = [
      '#title' => t('Acquia Lift Credential'),
      '#type' => 'details',
      '#tree' => TRUE,
      '#open' => SettingsHelper::isInvalidCredential($credential_settings),
    ];
    $form['account_id'] = [
      '#type' => 'textfield',
      '#title' => t('Account ID'),
      '#description' => t('Your Lift subscription\'s account ID.'),
      '#default_value' => $credential_settings['account_id'],
      '#required' => TRUE,
    ];
    $form['site_id'] = [
      '#type' => 'textfield',
      '#title' => t('Site ID'),
      '#description' => t('Current site\'s site ID. WARNING: different sites must use different value here, even between a staging and a production of the same site.'),
      '#default_value' => $credential_settings['site_id'],
      '#required' => TRUE,
    ];
    $form['assets_url'] = [
      '#type' => 'textfield',
      '#title' => t('Assets URL'),
      '#description' => t('Your Lift application\'s assets URL. It determines which version of the Lift application is being used.'),
      '#field_prefix' => 'https://',
      '#default_value' => $this->cleanUrl($credential_settings['assets_url']),
      '#required' => TRUE,
    ];
    $form['decision_api_url'] = [
      '#type' => 'textfield',
      '#title' => t('Decision API URL'),
      '#description' => t('Your Lift Decision API\'s URL. Unless explicitly instructed, leave empty to use default URL.'),
      '#field_prefix' => 'https://',
      '#default_value' => isset($credential_settings['decision_api_url']) ? $this->cleanUrl($credential_settings['decision_api_url']) : '',
    ];
    $form['oauth_url'] = [
      '#type' => 'textfield',
      '#title' => t('Authentication URL'),
      '#description' => t('Your Lift Authentication API\'s URL. Unless explicitly instructed, leave empty to use default URL.'),
      '#field_prefix' => 'https://',
      '#field_suffix' => '/authorize',
      '#default_value' => isset($credential_settings['oauth_url']) ? $this->cleanUrl($this->removeAuthorizeSuffix($credential_settings['oauth_url'])) : '',
    ];

    return $form;
  }

  /**
   * Build identity form.
   *
   * @return array
   *   Identity form.
   */
  private function buildIdentityForm() {
    $identity_settings = $this->config('acquia_lift.settings')->get('identity');
    $identity_parameter_display_value = $identity_settings['identity_parameter'] ?: 'identity';
    $identity_type_parameter_display_value = $identity_settings['identity_type_parameter'] ?: 'identityType';
    $default_identity_type_display_value = $identity_settings['default_identity_type'] ?: 'account';
    $default_identity_type_default_value = $identity_settings['default_identity_type'] ?: 'email';

    $form = [
      '#title' => t('Identity'),
      '#type' => 'details',
      '#tree' => TRUE,
      '#group' => 'data_collection_settings',
    ];
//    $form['capture_identity'] = [
//      '#type' => 'checkbox',
//      '#title' => t('Capture identity on login / register'),
//      '#default_value' => $identity_settings['capture_identity'],
//    ];
    $form['identity_parameter'] = [
      '#type' => 'textfield',
      '#title' => t('Identity Parameter'),
      '#description' => t('The URL link parameter for specific visitor information, such as an email address or social media username, which is sent to the Lift Profile Manager. Example using <strong>@identity_parameter_display_value</strong>: ?<strong><ins>@identity_parameter_display_value</ins></strong>=jdoe01', [
        '@identity_parameter_display_value' => $identity_parameter_display_value,
      ]),
      '#default_value' => $identity_settings['identity_parameter'],
    ];
    $form['identity_type_parameter'] = [
      '#type' => 'textfield',
      '#title' => t('Identity Type Parameter'),
      '#description' => t('The URL link parameter that corresponds to a Lift Profile Manager identifier type (one of the pre-defined ones or a new one you\'ve created). Example using <strong>@identity_type_parameter_display_value</strong>: ?@identity_parameter_display_value=jdoe01&<strong><ins>@identity_type_parameter_display_value</ins></strong>=@default_identity_type_default_value', [
        '@identity_parameter_display_value' => $identity_parameter_display_value,
        '@identity_type_parameter_display_value' => $identity_type_parameter_display_value,
        '@default_identity_type_default_value' => $default_identity_type_default_value,
      ]),
      '#default_value' => $identity_settings['identity_type_parameter'],
      '#states' => [
        'visible' => [
          ':input[name="identity[identity_parameter]"]' => ['!value' => ''],
        ],
      ],
    ];
    $form['default_identity_type'] = [
      '#type' => 'textfield',
      '#title' => t('Default Identity Type'),
      '#description' => t('The Lift Profile Manager identifier type to be used by default. Example using <strong>@default_identity_type_display_value</strong>: a visitor may visit the site through ?@identity_parameter_display_value=jdoe01 and omit the "@identity_type_parameter_display_value" query, and Lift will automatically identify this visitor as "jdoe01" of <strong><ins>@default_identity_type_display_value</ins></strong></strong> type. Leave this field blank to default to <strong>@default</strong> identity type.', [
        '@default' => 'email',
        '@identity_parameter_display_value' => $identity_parameter_display_value,
        '@identity_type_parameter_display_value' => $identity_type_parameter_display_value,
        '@default_identity_type_display_value' => $default_identity_type_display_value,
      ]),
      '#default_value' => $identity_settings['default_identity_type'],
      '#placeholder' => SettingsHelper::DEFAULT_IDENTITY_TYPE_DEFAULT,
      '#states' => [
        'visible' => [
          ':input[name="identity[identity_parameter]"]' => ['!value' => ''],
        ],
      ],
    ];

    return $form;
  }

  /**
   * Build field mappings form.
   *
   * @return array
   *   Field mappings form.
   */
  private function buildFieldMappingsForm() {
    $field_mappings_settings = $this->config('acquia_lift.settings')->get('field_mappings');
    $field_names = $this->getTaxonomyTermFieldNames();

    $taxonomy_vocabulary_url_text = t('Taxonomy Vocabulary');
    $taxonomy_vocabulary_url = Url::fromRoute('entity.taxonomy_vocabulary.collection', [], ['attributes' => ['target' => '_blank']]);
    $taxonomy_vocabulary_link = Link::fromTextAndUrl($taxonomy_vocabulary_url_text, $taxonomy_vocabulary_url)->toString();

    $form = [
      '#title' => t('Field Mappings'),
      '#description' => t('Create ') . $taxonomy_vocabulary_link . t(' and map to "content section", "content keywords", and "persona" fields.'),
      '#type' => 'details',
      '#tree' => TRUE,
      '#group' => 'data_collection_settings',
    ];
    $form['content_section'] = [
      '#type' => 'select',
      '#title' => t('Content Section'),
      '#empty_value' => '',
      '#options' => $field_names,
      '#default_value' => $field_mappings_settings['content_section'],
    ];
    $form['content_keywords'] = [
      '#type' => 'select',
      '#title' => t('Content Keywords'),
      '#empty_value' => '',
      '#options' => $field_names,
      '#default_value' => $field_mappings_settings['content_keywords'],
    ];
    $form['persona'] = [
      '#type' => 'select',
      '#title' => t('Persona'),
      '#empty_value' => '',
      '#options' => $field_names,
      '#default_value' => $field_mappings_settings['persona'],
    ];

    return $form;
  }

  /**
   * Build UDF mappings form.
   *
   * @param string $type
   *   The type of UDF field. Can be person, touch or event.
   *
   * @return array
   *   UDF mappings form.
   * @throws Exception
   *   An exception if the type given is not supported.
   */
  private function buildUdfMappingsForm($type = 'person') {
    if ($type !== 'person' && $type !== 'touch' && $type !== 'event') {
      throw new Exception('This Udf Field type is not supported');
    }

    $field_mappings_settings = $this->config('acquia_lift.settings')->get('udf_' . $type . '_mappings');
    $field_names = $this->getTaxonomyTermFieldNames();
    $taxonomy_vocabulary_url_text = t('Taxonomy Vocabulary');
    $taxonomy_vocabulary_url = Url::fromRoute('entity.taxonomy_vocabulary.collection', [], ['attributes' => ['target' => '_blank']]);
    $taxonomy_vocabulary_link = Link::fromTextAndUrl($taxonomy_vocabulary_url_text, $taxonomy_vocabulary_url)->toString();
    $udf_limit = SettingsHelper::getUdfLimitsForType($type);

    $form = [
      '#title' => t('User @type Mappings', ['@type' => ucfirst($type)]),
      '#description' => t('Map taxonomy terms to Visitor Profile @type fields in Acquia Lift. Select a Taxonomy Reference Field that, if present, will map the value of the specified field to the Acquia Lift Profile for that specific visitor. No options available? Create ',  ['@type' => $type]) . $taxonomy_vocabulary_link . t(' and map the corresponding value.'),
      '#type' => 'details',
      '#tree' => TRUE,
      '#group' => 'data_collection_settings',
    ];

    // Go over the amount of fields that we can map.
    for ($i = 1; $i < $udf_limit + 1; $i++) {
      $form[$type . '_udf' . $i] = [
        '#type' => 'select',
        '#title' => t('User Profile @type Field @number', ['@number' => $i, '@type' => ucfirst($type)]),
        '#empty_value' => '',
        '#options' => $field_names,
        '#default_value' => isset($field_mappings_settings[$type . '_udf' . $i]['value']) ? $field_mappings_settings[$type . '_udf' . $i]['value'] : '',
      ];
    }

    return $form;
  }

  /**
   * Get a list of Field names that are targeting type Taxonomy Terms.
   *
   * @return array
   *   An array of field names.
   */
  private function getTaxonomyTermFieldNames() {
    $definitions = $this->entityManager->getFieldStorageDefinitions('node');
    $field_names = [];
    foreach ($definitions as $field_name => $field_storage) {
      if ($field_storage->getType() != 'entity_reference' || $field_storage->getSetting('target_type') !== 'taxonomy_term') {
        continue;
      }
      $field_names[$field_name] = $field_name;
    }

    return $field_names;
  }

  /**
   * Build visibility form.
   *
   * @return array
   *   Visibility form.
   */
  private function buildVisibilityForm() {
    $visibility_settings = $this->config('acquia_lift.settings')->get('visibility');

    $form = [
      '#title' => t('Visibility'),
      '#description' => t('Lift will skip data collection on those URLs and their aliases.'),
      '#type' => 'details',
      '#tree' => TRUE,
      '#group' => 'data_collection_settings',
    ];
    $form['path_patterns'] = [
      '#type' => 'textarea',
      '#title' => t('Path patterns'),
      '#default_value' => $visibility_settings['path_patterns'],
    ];

    return $form;
  }

  /**
   * Display advanced configurations forms.
   *
   * @return array
   *   The render array for the advanced configuration form.
   */
  private function buildAdvancedConfigurationForm() {
    $settings = $this->config('acquia_lift.settings')->get('advanced');

    $form = [
      '#title' => t('Advanced configuration'),
      '#type' => 'details',
      '#tree' => TRUE,
      '#open' => FALSE,
    ];
    $form['content_replacement_mode'] = [
      '#type' => 'radios',
      '#title' => t('Content replacement mode'),
      '#description' => t('The default, site-wide setting for <a href="https://docs.acquia.com/lift/drupal/3/config/trusted" target="_blank">content replacement mode</a>.'),
      '#default_value' => $settings['content_replacement_mode'],
      '#options' => ['trusted' => t('Trusted'), 'untrusted' => t('Untrusted')],
    ];

    return $form;
  }

  /**
   * {@inheritdoc}
   */
  public function validateForm(array &$form, FormStateInterface $form_state) {
    parent::validateForm($form, $form_state);
    $this->validateCredentialvalues($form, $form_state);
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    $settings = $this->config('acquia_lift.settings');
    $values = $form_state->getValues();

    $this->setCredentialValues($settings, $values['credential']);
    $this->setIdentityValues($settings, $values['identity']);
    $this->setFieldMappingsValues($settings, $values['field_mappings']);
    $this->setUdfMappingsValues($settings, $values['udf_person_mappings'], 'person');
    $this->setUdfMappingsValues($settings, $values['udf_event_mappings'], 'event');
    $this->setUdfMappingsValues($settings, $values['udf_touch_mappings'], 'touch');
    $this->setVisibilityValues($settings, $values['visibility']);
    $this->setAdvancedConfigurationValues($settings, $values['advanced_configuration']);

    $settings->save();

    // It is required to flush all caches on save. This is because many settings
    // here impact page caches and their invalidation strategies.
    drupal_flush_all_caches();

    parent::submitForm($form, $form_state);
  }

  private function validateCredentialValues(array &$form, FormStateInterface $form_state) {
    $values = $form_state->getValues();

    // Validate Account ID.
    if (SettingsHelper::isInvalidCredentialAccountId($values['credential']['account_id'])) {
      $form_state->setError($form['credential']['account_id'], $this->t('Account ID contains invalid characters. It has to start with a letter and contain only alphanumerical characters.'));
    }

    // Validate Site ID.
    if (SettingsHelper::isInvalidCredentialSiteId($values['credential']['site_id'])) {
      $form_state->setError($form['credential']['site_id'], $this->t('Site ID contains invalid characters. Can only contain alphanumerical characters.'));
    }

    // Validate Assets URL.
    if (SettingsHelper::isInvalidCredentialAssetsUrl($values['credential']['assets_url'])) {
      $form_state->setError($form['credential']['assets_url'], $this->t('Assets URL is an invalid URL.'));
    }

    // Validate Decision API URL.
    if (SettingsHelper::isInvalidCredentialDecisionApiUrl($values['credential']['decision_api_url'])) {
      $form_state->setError($form['credential']['decision_api_url'], $this->t('Decision API URL is an invalid URL.'));
    }

    // Validate Auth URL.
    if (SettingsHelper::isInvalidCredentialOauthUrl($values['credential']['oauth_url'])) {
      $form_state->setError($form['credential']['oauth_url'], $this->t('Authentication URL is an invalid URL.'));
    }
  }

  /**
   * Set credential values.
   *
   * @param \Drupal\Core\Config\Config $settings
   *   Acquia Lift config settings.
   * @param array $values
   *   Credential values.
   */
  private function setCredentialValues(Config $settings, array $values) {
    $settings->set('credential.account_id', trim($values['account_id']));
    $settings->set('credential.site_id', trim($values['site_id']));

    // Set Assets URL, also check its connection.
    $standardized_assets_url = 'https://' . $this->cleanUrl($values['assets_url']);
    $settings->set('credential.assets_url', $standardized_assets_url);
    $standardized_assets_url_parts = parse_url($standardized_assets_url);
    $standardized_assets_url_scheme = isset($standardized_assets_url_parts['scheme']) ? $standardized_assets_url_parts['scheme'] : '';
    $standardized_assets_url_host = isset($standardized_assets_url_parts['host']) ? $standardized_assets_url_parts['host'] : '';
    $standardized_assets_url_path = isset($standardized_assets_url_parts['path']) ? $standardized_assets_url_parts['path'] : '';
    $this->checkConnection('Assets', $standardized_assets_url_scheme . '://' . $standardized_assets_url_host, $standardized_assets_url_path . '/lift.js');

    // If present, set Decision API URL, also check its connection.
    $settings->clear('credential.decision_api_url');
    if (!empty($values['decision_api_url'])) {
      $standardized_decision_api_url = 'https://' . $this->cleanUrl($values['decision_api_url']);
      $settings->set('credential.decision_api_url', $standardized_decision_api_url);
      $this->checkConnection('Decision API', $standardized_decision_api_url, '/admin/ping', 403);
    }

    // If present, set OAuth URL, also check its connection.
    $settings->clear('credential.oauth_url');
    if (!empty($values['oauth_url'])) {
      $standardized_oauth_url = 'https://' . $this->cleanUrl($this->removeAuthorizeSuffix($values['oauth_url']));
      $settings->set('credential.oauth_url', $standardized_oauth_url . '/authorize');
      $this->checkConnection('Authentication', $standardized_oauth_url, '/ping');
    }
  }

  /**
   * Clean up URL. Remove the:
   *   1) Protocol "http://" and "http://".
   *   2) Leading and trailing slashes and space characters.
   *
   * @param string $url
   *   URL.
   * @return string
   *   URL, but cleaned up.
   */
  private function cleanUrl($url) {
    $searchFor = [
      '~^[ \t\r\n\/]+~',
      '~[ \t\r\n\/]+$~',
      '~^https?://~',
    ];
    return preg_replace($searchFor, '', $url);
  }

  /**
   * Remove the "/authorize" suffix, if any.
   *
   * @param string $url
   *   URL.
   * @return string
   *   URL, but with "/authorize" removed.
   */
  private function removeAuthorizeSuffix($url) {
    return preg_replace('~/authorize$~', '', $url);
  }

  /**
   * Check URL's connection.
   *
   * @param string $name
   *   Name of the service.
   * @param string $base_uri
   *   Base URI.
   * @param string $path
   *   Path to "ping" end point.
   * @param integer $expected_status_code
   *   Expected status code.
   */
  private function checkConnection($name, $base_uri, $path, $expected_status_code = 200) {
    $responseInfo = SettingsHelper::pingUri($base_uri, $path);
    if (empty($responseInfo)) {
      drupal_set_message(t('Acquia Lift module could not reach the specified :name URL.', [':name' => $name]), 'error');
      return;
    }
    if ($responseInfo['statusCode'] !== $expected_status_code) {
      drupal_set_message(t('Acquia Lift module has successfully connected to :name URL, but received status code ":statusCode" with the reason ":reasonPhrase".', [
        ':name' => $name,
        ':statusCode' => $responseInfo['statusCode'],
        ':reasonPhrase' => $responseInfo['reasonPhrase'],
      ]), 'error');
    }
  }

  /**
   * Set identity values.
   *
   * @param \Drupal\Core\Config\Config $settings
   *   Acquia Lift config settings.
   * @param array $values
   *   Identity values.
   */
  private function setIdentityValues(Config $settings, array $values) {
    //$settings->set('identity.capture_identity', trim($values['capture_identity']));
    $settings->set('identity.identity_parameter', trim($values['identity_parameter']));
    $settings->set('identity.identity_type_parameter', trim($values['identity_type_parameter']));
    $settings->set('identity.default_identity_type', trim($values['default_identity_type']));
  }

  /**
   * Set field mapping values.
   *
   * @param \Drupal\Core\Config\Config $settings
   *   Acquia Lift config settings.
   * @param array $values
   *   Field mappings values.
   */
  private function setFieldMappingsValues(Config $settings, array $values) {
    $settings->set('field_mappings.content_section', $values['content_section']);
    $settings->set('field_mappings.content_keywords', $values['content_keywords']);
    $settings->set('field_mappings.persona', $values['persona']);
  }

  /**
   * Set Udf Mapping mapping values to our config object.
   *
   * @param \Drupal\Core\Config\Config $settings
   *   Acquia Lift config settings.
   * @param array $values
   *   Field mappings values.
   * @param string $type
   *   The type of UDF field. Can be person, touch or event.
   *
   * @throws Exception
   *   An exception if the type given is not supported.
   */
  private function setUdfMappingsValues(Config $settings, array $values, $type = 'person') {
    if ($type !== 'person' && $type !== 'touch' && $type !== 'event') {
      throw new Exception('This Udf Field type is not supported');
    }
    $mappings = [];
    foreach ($values as $value_id => $value) {
      if (empty($value)) {
        continue;
      }
      $mappings[$value_id] = [
        'id' => $value_id,
        'value' => $value,
        'type' => 'taxonomy'
      ];
    }
    $settings->set('udf_' . $type . '_mappings', $mappings);
  }

  /**
   * Set visibility values.
   *
   * @param \Drupal\Core\Config\Config $settings
   *   Acquia Lift config settings.
   * @param array $values
   *   Visibility values.
   */
  private function setVisibilityValues(Config $settings, array $values) {
    $settings->set('visibility.path_patterns', $values['path_patterns']);
  }

  /**
   * Sets the advanced configuration values.
   *
   * @param \Drupal\Core\Config\Config $settings
   *   Acquia Lift config settings
   * @param array $values
   *   Advanced configuration values
   */
  private function setAdvancedConfigurationValues(Config $settings, array $values) {
    $settings->set('advanced.content_replacement_mode', $values['content_replacement_mode']);
  }
}
