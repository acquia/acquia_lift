<?php

namespace Drupal\acquia_lift\Form;

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
    $form['credential'] = $this->buildCredentialForm();

    // Data collection settings.
    $form['data_collection_settings'] = [
      '#type' => 'vertical_tabs',
      '#title' => t('Data collection settings'),
    ];
    $form['identity'] = $this->buildIdentityForm();
    $form['field_mappings'] = $this->buildFieldMappingsForm();
    $form['visibility'] = $this->buildVisibilityForm();
    $form['thumbnail_url'] = $this->buildThumbnailUrlForm();

    return parent::buildForm($form, $form_state);
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
      '#title' => t('Credential'),
      '#type' => 'details',
      '#tree' => TRUE,
      '#open' => SettingsHelper::isInvalidCredential($credential_settings),
    ];
    $form['account_name'] = [
      '#type' => 'textfield',
      '#title' => t('Account Name'),
      '#default_value' => $credential_settings['account_name'],
      '#required' => TRUE,
    ];
    $form['customer_site'] = [
      '#type' => 'textfield',
      '#title' => t('Customer Site'),
      '#default_value' => $credential_settings['customer_site'],
      '#required' => TRUE,
    ];
    $form['js_path'] = [
      '#type' => 'textfield',
      '#title' => t('JavaScript Path'),
      '#field_prefix' => 'https://',
      '#default_value' => $this->removeProtocal($credential_settings['js_path']),
      '#required' => TRUE,
    ];
    $form['api_url'] = [
      '#type' => 'textfield',
      '#title' => t('API URL'),
      '#field_prefix' => 'https://',
      '#default_value' => $this->removeProtocal($credential_settings['api_url']),
      '#placeholder' => t('Leave empty to use default URL'),
    ];
    $form['oauth_url'] = [
      '#type' => 'textfield',
      '#title' => t('Authentication URL'),
      '#field_prefix' => 'https://',
      '#default_value' => $this->removeProtocal($credential_settings['oauth_url']),
      '#placeholder' => t('Leave empty to use default URL'),
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
      '#default_value' => $identity_settings['identity_parameter'],
    ];
    $form['identity_type_parameter'] = [
      '#type' => 'textfield',
      '#title' => t('Identity Type Parameter'),
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
      '#default_value' => $identity_settings['default_identity_type'],
      '#placeholder' => SettingsHelper::DEFAULT_IDENTITY_TYPE_DEFAULT,
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

    $form = [
      '#title' => t('Field Mappings'),
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
      '#type' => 'details',
      '#tree' => TRUE,
      '#group' => 'data_collection_settings',
    ];
    $form['path_patterns'] = [
      '#type' => 'textarea',
      '#title' => t('Path patterns'),
      '#description' => t('Lift will skip data collection on those URLs and their aliases.'),
      '#default_value' => $visibility_settings['path_patterns'],
    ];

    return $form;
  }

  /**
   * Display thumbnail URL form.
   *
   * @return array
   *   Thumbnail URL form.
   */
  private function buildThumbnailUrlForm() {
    $form = [
      '#title' => t('Thumbnail URL'),
      '#type' => 'details',
      '#tree' => TRUE,
      '#group' => 'data_collection_settings',
    ];
    $form['link_list'] = [
      '#type' => 'markup',
      '#markup' => '<div>' . t('There are no content types. Please create a content type first.') . '</div>',
    ];

    $node_types = NodeType::loadMultiple();
    if (empty($node_types)) {
      return $form;
    }

    $links = [];
    $link_attributes = ['attributes' => ['target' => '_blank'], 'fragment' => 'edit-acquia-lift'];
    foreach ($node_types as $node_type) {
      $url = Url::fromRoute('entity.node_type.edit_form', ['node_type' => $node_type->id()], $link_attributes);
      $links[] = '<p>' . Link::fromTextAndUrl($node_type->label(), $url)->toString() . '</p>';
    }
    $form['link_list']['#markup'] = t('Configure thumbnail URLs on each content type\'s edit page (in a new window):');
    $form['link_list']['#markup'] .= implode('', $links);

    return $form;
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
    $this->setVisibilityValues($settings, $values['visibility']);

    $settings->save();

    drupal_flush_all_caches();

    parent::submitForm($form, $form_state);
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
    $settings->set('credential.account_name', $values['account_name']);
    $settings->set('credential.customer_site', $values['customer_site']);
    $settings->set('credential.js_path', 'https://' . $this->removeProtocal($values['js_path']));
    $settings->clear('credential.oauth_url');
    if (!empty($values['api_url'])) {
      $settings->set('credential.api_url', 'https://' . $this->removeProtocal($values['api_url']));
    }
    $settings->clear('credential.oauth_url');
    if (!empty($values['oauth_url'])) {
      $settings->set('credential.oauth_url', 'https://' . $this->removeProtocal($values['oauth_url']));
    }
  }

  /**
   * Remove the protocal from an URL string.
   *
   * @param string $url
   *   URL.
   *
   * @return string
   *   Same URL as input except with the 'http://' and 'https://' trimmed.
   */
  private function removeProtocal($url) {
    return preg_replace('~^https?://~', '', $url);
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
//    $settings->set('identity.capture_identity', $values['capture_identity']);
    $settings->set('identity.identity_parameter', $values['identity_parameter']);
    $settings->set('identity.identity_type_parameter', $values['identity_type_parameter']);
    $settings->set('identity.default_identity_type', $values['default_identity_type']);
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
}
