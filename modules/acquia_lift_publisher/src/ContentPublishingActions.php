<?php

namespace Drupal\acquia_lift_publisher;

use Drupal\acquia_contenthub_publisher\ContentHubExportQueue;
use Drupal\acquia_lift_publisher\Form\ContentPublishingForm;
use Drupal\Core\Config\ImmutableConfig;
use Drupal\Core\Entity\ContentEntityInterface;
use Drupal\Core\Entity\EntityInterface;

/**
 * Contains helper methods for managing Content Hub exports.
 *
 * @package Drupal\acquia_lift_publisher
 */
class ContentPublishingActions {

  /**
   * The acquia lift publishing settings.
   *
   * @var \Drupal\Core\Config\ImmutableConfig
   * @see \Drupal\acquia_lift_publisher\Form\ContentPublishingForm
   */
  private $publisherSettings;

  /**
   * The Content Hub export queue.
   *
   * @var \Drupal\acquia_contenthub_publisher\ContentHubExportQueue
   */
  private $exportQueue;

  /**
   * ContentPublishingActions constructor.
   *
   * @param \Drupal\acquia_contenthub_publisher\ContentHubExportQueue $content_hub_export_queue
   *   The Content Hub export queue service.
   * @param \Drupal\Core\Config\ImmutableConfig $config
   *   The acquia lift publishing settings.
   */
  public function __construct(ContentHubExportQueue $content_hub_export_queue, ImmutableConfig $config) {
    $this->exportQueue = $content_hub_export_queue;
    $this->publisherSettings = $config;
  }

  /**
   * Triggers the Content Hub export process.
   *
   * @param \Drupal\Core\Entity\EntityInterface $entity
   *   The current entity.
   */
  public function triggerQueue(EntityInterface $entity): void {
    if (!$entity instanceof ContentEntityInterface ||
      !$this->publisherSettings->get(ContentPublishingForm::$pushSettingField)) {
      return;
    }

    if ($this->exportQueue->getQueueCount() < 1) {
      return;
    }

    $this->exportQueue->processQueueItems();
  }

  /**
   * Returns publisher setting value by its name.
   *
   * @param string $field_name
   *   Publish Setting Field.
   * @param string|null $default_value
   *   Default Value.
   *
   * @return array|mixed|null
   *   The setting's value.
   */
  public function getPublicationsFieldValue($field_name, $default_value = NULL) {
    return $this->publisherSettings->get($field_name) ?? $default_value;
  }

}
