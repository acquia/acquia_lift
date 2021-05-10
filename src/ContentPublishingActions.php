<?php

namespace Drupal\acquia_perz;

use Drupal\Core\Config\ImmutableConfig;
use Drupal\Core\Entity\ContentEntityInterface;
use Drupal\Core\Entity\EntityTypeManagerInterface;
use Drupal\Core\Datetime\DateFormatterInterface;
use Drupal\Component\Uuid\UuidInterface;
use Drupal\Core\Render\RendererInterface;
use Drupal\Core\Entity\EntityInterface;
use Drupal\Component\Datetime\TimeInterface;

/**
 * Contains helper methods for managing Content Index Service exports.
 *
 * @package Drupal\acquia_perz
 */
class ContentPublishingActions {

  /**
   * The acquia perz publishing settings.
   *
   * @var \Drupal\Core\Config\ImmutableConfig
   * @see \Drupal\acquia_perz\Form\ContentPublishingForm
   */
  private $publisherSettings;

  /**
   * The acquia perz publishing settings.
   *
   * @var \Drupal\Core\Config\ImmutableConfig
   * @see \Drupal\acquia_perz\Form\CISSettingsForm
   */
  private $cisSettings;

  /**
   * The entity type manager.
   *
   * @var \Drupal\Core\Entity\EntityTypeManagerInterface
   */
  protected $entityTypeManager;

  /**
   * The renderer.
   *
   * @var \Drupal\Core\Render\RendererInterface
   */
  protected $renderer;

  /**
   * The uuid generator.
   *
   * @var \Drupal\Component\Uuid\UuidInterface
   */
  protected $uuidGenerator;

  /**
   * The date formatter service.
   *
   * @var \Drupal\Core\Datetime\DateFormatterInterface
   */
  protected $dateFormatter;

  /**
   * The time service.
   *
   * @var \Drupal\Component\Datetime\TimeInterface
   */
  protected $time;

  /**
   * ContentPublishingActions constructor.
   *
   * @param \Drupal\Core\Config\ImmutableConfig $publisher_settings
   *   The acquia perz publishing settings.
   * @param \Drupal\Core\Config\ImmutableConfig $cis_settings
   *   The acquia perz cis settings.
   * @param \Drupal\Core\Entity\EntityTypeManagerInterface $entity_type_manager
   *   The entity type manager.
   * @param \Drupal\Core\Render\RendererInterface $renderer
   *   The renderer.
   * @param \Drupal\Component\Uuid\UuidInterface $uuid_generator
   *   The UUID generator.
   * @param \Drupal\Core\Datetime\DateFormatterInterface $date_formatter
   *   The date formatter service.
   * @param \Drupal\Component\Datetime\TimeInterface $time
   *   The time service.
   */
  public function __construct(ImmutableConfig $publisher_settings, ImmutableConfig $cis_settings, EntityTypeManagerInterface $entity_type_manager, RendererInterface $renderer, UuidInterface $uuid_generator, DateFormatterInterface $date_formatter, TimeInterface $time) {
    $this->publisherSettings = $publisher_settings;
    $this->cisSettings = $cis_settings;
    $this->entityTypeManager = $entity_type_manager;
    $this->renderer = $renderer;
    $this->uuidGenerator = $uuid_generator;
    $this->dateFormatter = $date_formatter;
    $this->time = $time;
  }

  /**
   * Returns the value of the entity view modes setting.
   *
   * @param \Drupal\Core\Entity\EntityInterface $entity
   *   The current entity.
   *
   * @return array
   *   The setting value.
   */
  protected function getEntityViewModesSettingValue(EntityInterface $entity): array {
    return $this->publisherSettings->get("view_modes.{$entity->getEntityTypeId()}.{$entity->bundle()}") ?? [];
  }

  /**
   * Get and publish entity by its entity type and id.
   *
   * @param string $entity_type
   *  Entity type of the entity that should be exported.
   * @param integer $entity_id
   *  Id of the entity that should be exported.
   * @param string $language
   *  Language code of the entity translation that should be exported.
   * 'all' value means that all entity translations should be exported.
   */
  public function publishEntityById($entity_type, $entity_id, $langcode = 'all') {
    $entity = $this
      ->entityTypeManager
      ->getStorage($entity_type)
      ->load($entity_id);
    if (!$entity instanceof ContentEntityInterface) {
      return;
    }
    if (!$view_modes = $this->getEntityViewModesSettingValue($entity)) {
      return;
    }
    foreach (array_keys($view_modes) as $view_mode) {
      // The preview image field setting is saved along side the view modes.
      // Don't process it as one.
      if ($view_mode == 'acquia_perz_preview_image') {
        continue;
      }
      if ($langcode === 'all') {
        foreach ($entity->getTranslationLanguages() as $language) {
          $langcode = $language->getId();
          //$translation = $entity->getTranslation($language->getId());
          $this->publishEntityByViewMode($entity, $view_mode, $langcode);
        }
      }
      else {
        $this->publishEntityByViewMode($entity, $view_mode, $langcode);
      }
    }
  }

  /**
   * Publish all entity view modes.
   *
   * @param \Drupal\Core\Entity\EntityInterface $entity
   *   The current entity.
   */
  public function publishEntity(EntityInterface $entity) {
    if (!$entity instanceof ContentEntityInterface) {
      return;
    }
    if (!$view_modes = $this->getEntityViewModesSettingValue($entity)) {
      return;
    }
    $langcode = $entity->language()->getId();
    foreach (array_keys($view_modes) as $view_mode) {
      // The preview image field setting is saved along side the view modes.
      // Don't process it as one.
      if ($view_mode == 'acquia_perz_preview_image') {
        continue;
      }
      $this->publishEntityByViewMode($entity, $view_mode, $langcode);
    }
  }

  /**
   * Publish entity by view mode.
   *
   * @param \Drupal\Core\Entity\EntityInterface $entity
   *   The current entity.
   * @param string $view_mode
   *   The view mode.
   * @param string $langcode
   *   The language code.
   */
  protected function publishEntityByViewMode(EntityInterface $entity, $view_mode, $langcode) {
    $elements = $this->entityTypeManager
      ->getViewBuilder($entity->getEntityTypeId())
      ->view($entity, $view_mode, $langcode);
    $rendered_data = $this->renderer->renderPlain($elements);
    $data = [
      'account_id' => $this->cisSettings->get('cis.account_id'),
      'environment' => $this->cisSettings->get('cis.environment'),
      'content_uuid' => $entity->uuid(),
      'updated' => $this->dateFormatter->format($this->time->getCurrentTime(), 'custom', 'Y-m-d\TH:i:s'),
      'content_type' => $entity->getEntityTypeId(),
      'view_mode' => $view_mode,
      'language' => $langcode,
      'rendered_data' => $rendered_data,
    ];
    \Drupal::logger('data')->notice('<pre>'.print_r($data, TRUE).'</pre>');
  }

}
