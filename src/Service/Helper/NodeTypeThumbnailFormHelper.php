<?php

namespace Drupal\acquia_lift\Service\Helper;

use Drupal\Core\Entity\EntityManagerInterface;
use Drupal\Core\Config\ConfigFactoryInterface;

/**
 * Defines a form that alters node type form to add a thumbnail form.
 */
class NodeTypeThumbnailFormHelper {
  /**
   * Settings.
   *
   * @var \Drupal\Core\Config\Config
   */
  private $settings;

  /**
   * Entity manager.
   *
   * @var \Drupal\Core\Entity\EntityManagerInterface
   */
  private $entityManager;

  /**
   * A list of spl_object_hash codes of objects that this service has already
   * iterated through. This design is for handling circular referencing entities.
   *
   * Example: ['000000005e937119000000007b808ade' => TRUE]
   *
   * @var array $processedFieldHashes
   */
  private $processedFieldHashes = [];

  /**
   * Available image fields, keyed by field key "roadmap" and valued at field labels.
   *
   * Example: [
   *  'field_image' => 'Image',
   *  'field_media->thumbnail' => Media->Thumbnail,
   * ]
   *
   * @var array $imageFields
   */
  private $imageFields = [];

  /**
   * Constructor.
   *
   * @param \Drupal\Core\Config\ConfigFactoryInterface $config_factory
   *   The config factory service.
   * @param \Drupal\Core\Entity\EntityManagerInterface $entity_manager
   *   Entity type manager.
   */
  public function __construct(ConfigFactoryInterface $config_factory, EntityManagerInterface $entity_manager) {
    $this->settings = $config_factory->getEditable('acquia_lift.settings');
    $this->entityManager = $entity_manager;
  }

  /**
   * Get Form.
   *
   * @param string $node_type
   *   Node Type.
   *
   * @return array
   *   Acquia Lift Thumbnail Form.
   */
  public function getForm($node_type) {
    $form = [
      '#title' => t('Acquia Lift'),
      '#type' => 'details',
      '#tree' => TRUE,
      '#group' => 'additional_settings',
    ];

    // Find image fields.
    $this->collectImageFields('node', $node_type);
    if (empty($this->imageFields)) {
      $form['no_image_field'] = [
        '#type' => 'markup',
        '#markup' => '<div>' . t('This content type has no image field yet.') . '</div>',
      ];
      return $form;
    }

    // Find image styles.
    $image_styles = image_style_options(FALSE);
    if (empty($image_styles)) {
      $form['no_image_styles'] = [
        '#type' => 'markup',
        '#markup' => '<div>' . t('This site has no image style yet. Please define an image style first.') . '</div>',
      ];
      return $form;
    }

    $thumbnail_settings = $this->settings->get('thumbnail');
    $node_type_thumbnail_settings = isset($thumbnail_settings[$node_type]) ?
      $thumbnail_settings[$node_type] :
      ['field' => '', 'style' => ''];
    $form['field'] = [
      '#type' => 'select',
      '#title' => t('Select preview image to use for content recommendations.'),
      '#options' => $this->imageFields,
      '#default_value' => $node_type_thumbnail_settings['field'],
      '#empty_option' => t('None'),
      '#empty_value' => '',
    ];
    $form['style'] = [
      '#type' => 'select',
      '#title' => t('Select the style to use for the thumbnail.'),
      '#options' => $image_styles,
      '#default_value' => $node_type_thumbnail_settings['style'],
      '#empty_option' => t('None'),
      '#empty_value' => '',
      '#states' => [
        'visible' => [
          ':input[name="acquia_lift[field]"]' => ['!value' => ''],
        ],
      ],
    ];

    return $form;
  }

  /**
   * Traverse the FieldableEntity and its fields, collect a field "roadmap" that
   * can lead to a image file.
   *
   * @param string $target_type
   *   Fieldable entity's identifier.
   * @param string $type
   *   Type of the fieldable entity.
   * @param string $key_prefix
   *   The concatenated entity field keys that has been traversed through.
   * @param string $label_prefix
   *   The concatenated entity labels that has been traversed through.
   */
  private function collectImageFields($target_type, $type, $key_prefix = '', $label_prefix = '') {
    $field_definitions = $this->entityManager->getFieldDefinitions($target_type, $type);
    foreach ($field_definitions as $field_key => $field_definition) {
      $field_type = $field_definition->getType();
      $field_target_type = $field_definition->getSetting('target_type');
      $field_label = $field_definition->getLabel();
      $full_label = $label_prefix . $field_label;
      $full_key = $key_prefix . $field_key;

      // 1) Image type.
      if ($field_type === 'image') {
        $this->imageFields[$full_key] = $full_label . ' (' . $full_key . ')';
        continue;
      }

      // Check if the field has already been processed. If so, skip.
      $field_hash = spl_object_hash($field_definition);
      if (isset($this->processedFieldHashes[$field_hash])) {
        continue;
      }

      // 2) Entity Reference type whose entity is Fieldable.
      if ($field_type === 'entity_reference' &&
        $this->entityManager->getDefinition($field_target_type)->isSubclassOf('\Drupal\Core\Entity\FieldableEntityInterface')
      ) {
        // Track this field, since it is about to be processed.
        $this->processedFieldHashes[$field_hash] = TRUE;

        // Process this field.
        $this->collectImageFields($field_target_type, $field_type, $full_key . '->', $full_label . '->');
        continue;
      }
    }
  }

  /**
   * Save settings.
   *
   * @param string $node_type
   *   Node Type.
   * @param array $settings
   *   Settings.
   */
  public function saveSettings($node_type, $settings) {
    $thumbnail_settings = $this->settings->get('thumbnail');
    $thumbnail_settings[$node_type] = $settings;
    $this->settings->set('thumbnail', $thumbnail_settings)->save();
  }
}
