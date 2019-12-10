<?php

namespace Drupal\acquia_lift_publisher\Form;

use Drupal\acquia_contenthub\Client\ClientFactory;
use Drupal\Core\Entity\EntityFieldManagerInterface;
use Drupal\Core\Entity\EntityInterface;
use Drupal\Core\Entity\EntityTypeManagerInterface;
use Drupal\Core\Form\ConfigFormBase;
use Drupal\Core\Form\FormStateInterface;
use Drupal\Core\Entity\FieldableEntityInterface;
use Drupal\Core\Url;
use Symfony\Component\DependencyInjection\ContainerInterface;

/**
 * Class EntityViewModeConfigurationForm.
 *
 * @package Drupal\acquia_lift_publisher\Form
 */
class EntityViewModeConfigurationForm extends ConfigFormBase {

  /**
   * The entity type manager.
   *
   * @var \Drupal\Core\Entity\EntityTypeManagerInterface
   */
  protected $entityTypeManager;

  /**
   * The entity field manager.
   *
   * @var \Drupal\Core\Entity\EntityFieldManagerInterface
   */
  protected $entityFieldManager;

  /**
   * The client factory.
   *
   * @var |Drupal\acquia_contenthub\Client\ClientFactory
   */
  protected $clientFactory;

  /**
   * Contenthub Client.
   *
   * @var \Acquia\ContentHubClient\ContentHubClient|bool
   */
  protected $client;

  /**
   * {@inheritdoc}
   */
  public static function create(ContainerInterface $container) {
    return new static(
      $container->get('entity_type.manager'),
      $container->get('entity_field.manager'),
      $container->get('acquia_contenthub.client.factory')
    );
  }

  /**
   * EntityViewModeConfigurationForm constructor.
   *
   * @param \Drupal\Core\Entity\EntityTypeManagerInterface $entity_type_manager
   *   The entity type manager.
   * @param \Drupal\Core\Entity\EntityFieldManagerInterface $entity_field_manager
   *   The entity field manager.
   * @param \Drupal\acquia_contenthub\Client\ClientFactory $client_factory
   *   The client factory.
   */
  public function __construct(EntityTypeManagerInterface $entity_type_manager, EntityFieldManagerInterface $entity_field_manager, ClientFactory $client_factory) {
    $this->entityTypeManager = $entity_type_manager;
    $this->entityFieldManager = $entity_field_manager;
    $this->clientFactory = $client_factory;
  }

  /**
   * {@inheritdoc}
   */
  protected function getEditableConfigNames() {
    return ['acquia_lift_publisher.entity_config'];
  }

  /**
   * {@inheritdoc}
   */
  public function getFormId() {
    return 'acquia_lift_publisher_entity_config';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state) {
    // Check to see that we have a valid content hub client.
    $this->client = $this->clientFactory->getClient();
    if (empty($this->client)) {
      \Drupal::messenger()->addWarning(
        $this->t(
          'Please configure the <a href="@ch">Acquia Content Hub Module</a> to start publishing personalized content.',
          ['@ch' => Url::fromRoute('acquia_contenthub.admin_settings')->toString()]
        )
      );
    }

    $config = $this->config('acquia_lift_publisher.entity_config');
    /** @var \Drupal\Core\Entity\Entity\EntityViewMode[] $view_modes */
    $view_modes = $this->entityTypeManager->getStorage('entity_view_mode')->loadMultiple();
    $form['options'] = [
      '#tree' => TRUE,
    ];
    foreach ($view_modes as $view_mode) {
      $entity_type_id = $view_mode->getTargetType();
      /** @var \Drupal\Core\Entity\EntityTypeInterface $entity_type */
      $entity_type = $this->entityTypeManager->getDefinition($entity_type_id);
      // Do not present options for non-fieldable entity types.
      if (!$entity_type->entityClassImplements(FieldableEntityInterface::class)) {
        continue;
      }
      $bundles = $entity_type->getBundleEntityType() ? $this->entityTypeManager->getStorage($entity_type->getBundleEntityType())->loadMultiple() : [$entity_type];
      foreach ($bundles as $bundle) {
        if (empty($form['options'][$entity_type_id][$bundle->id()])) {
          if ($bundle instanceof EntityInterface) {
            $title = "{$entity_type->getLabel()}: {$bundle->label()}";
          }
          else {
            $title = "{$entity_type->getLabel()}: {$bundle->id()}";
          }
          $form['options'][$entity_type_id][$bundle->id()] = [
            '#type' => 'details',
            '#title' => $title,
          ];
        }
        $short_name = explode('.', $view_mode->id())[1];
        $form['options'][$entity_type_id][$bundle->id()][$short_name] = [
          '#type' => 'checkbox',
          '#title' => $view_mode->label(),
          '#default_value' => $config->get("view_modes.$entity_type_id.{$bundle->id()}.$short_name"),
        ];
        if (empty($form['options'][$entity_type_id][$bundle->id()]['acquia_lift_preview_image'])) {
          $this->setImageSelectorElement($form['options'][$entity_type_id][$bundle->id()], $entity_type_id, $bundle->id());
          $form['options'][$entity_type_id][$bundle->id()]['acquia_lift_preview_image']['#default_value'] = $config->get("view_modes.$entity_type_id.{$bundle->id()}.acquia_lift_preview_image");
        }
      }
    }
    $roles = $this->entityTypeManager->getStorage('user_role')->loadMultiple();
    $options = [];
    foreach ($roles as $role) {
      $options[$role->id()] = $role->label();
    }
    $form['render_role'] = [
      '#title' => $this->t('Render role'),
      '#description' => $this->t('The role to use when rendering entities for personalization.'),
      '#type' => 'select',
      '#options' => $options,
      '#default_value' => $config->get("render_role"),
    ];
    return parent::buildForm($form, $form_state);
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    $options = $form_state->getValue('options');
    foreach ($options as $entity_type_id => $bundles) {
      foreach ($bundles as $bundle => $values) {
        unset($values['label']);
        $options[$entity_type_id][$bundle] = array_filter($values);
        if (empty($options[$entity_type_id][$bundle])) {
          unset($options[$entity_type_id][$bundle]);
        }
      }
      if (empty($options[$entity_type_id])) {
        unset($options[$entity_type_id]);
      }
    }
    $config = $this->config('acquia_lift_publisher.entity_config');
    $config->set('view_modes', $options);
    $config->set('render_role', $form_state->getValue('render_role'));
    $config->save();
    parent::submitForm($form, $form_state);
  }

  /**
   * Set image selector element.
   *
   * @param array $form
   *   The form array.
   * @param string $entity_type_id
   *   The entity type id.
   * @param string $bundle
   *   The entity bundle type.
   */
  protected function setImageSelectorElement(array &$form, $entity_type_id, $bundle) {
    // Find image fields.
    $image_fields = $this->collectImageFields($entity_type_id, $bundle);
    if (!empty($image_fields)) {
      $form['acquia_lift_preview_image'] = [
        '#type' => 'select',
        '#title' => t("Select bundle's preview image field."),
        '#options' => $image_fields,
        '#empty_option' => t('None'),
        '#empty_value' => '',
        '#weight' => 100,
      ];
    }
  }

  /**
   * Collect imagefields for a particular entity bundle.
   *
   * @param string $entity_type_id
   *   The entity type identifier.
   * @param string $bundle
   *   The bundle name.
   *
   * @return array
   *   The array of image fields for a given entity bundle.
   */
  protected function collectImageFields($entity_type_id, $bundle) {
    $image_fields = [];
    $field_definitions = $this->entityFieldManager->getFieldDefinitions($entity_type_id, $bundle);
    foreach ($field_definitions as $field_key => $field_definition) {
      if ($field_definition->getType() === 'image') {
        $image_fields[$field_key] = $field_definition->getLabel();
      }
    }
    return $image_fields;
  }

}
