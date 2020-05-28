<?php

namespace Drupal\acquia_lift_publisher\Form;

use Drupal\Core\Entity\EntityInterface;

/**
 * Provides common methods for classes that involve publisher settings.
 *
 * @package Drupal\acquia_lift_publisher\Form
 */
trait ContentPublishingSettingsTrait {

  /**
   * Acquia lift publisher configuration object.
   *
   * @var \Drupal\Core\Config\ImmutableConfig
   * @see \Drupal\acquia_lift_publisher\Form\ContentPublishingForm
   */
  private $publisherSettings;

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

}
