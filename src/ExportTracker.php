<?php

namespace Drupal\acquia_perz;

use Drupal\Core\Database\Connection;
use Drupal\Core\Entity\EntityChangedInterface;
use Drupal\Core\Entity\EntityInterface;

/**
 * The publisher tracker table class.
 */
class ExportTracker {

  const DELETED = 'deleted';

  const EXPORTED = 'exported';

  const FAILED = 'failed';

  /**
   * The name of the tracking table.
   */
  const EXPORT_TRACKING_TABLE = 'acquia_perz_export_tracking';

  /**
   * PublisherTracker constructor.
   *
   * @param \Drupal\Core\Database\Connection $database
   *   The database connection.
   */
  public function __construct(Connection $database) {
    $this->database = $database;
  }

  /**
   * Gets the tracking record for a given uuid.
   *
   * @param string $uuid
   *   The entity uuid.
   * @param string $langcode
   *   The langcode of the tracking entity.
   *
   * @return \Drupal\Core\Database\StatementInterface|int|null
   *   Database statement
   */
  public function get($uuid, $langcode = '') {
    $query = $this->database->select(self::EXPORT_TRACKING_TABLE, 't')
      ->fields('t', ['entity_uuid']);
    $query->condition('entity_uuid', $uuid);
    if (!empty($langcode)) {
      $query->condition('langcode', $langcode);
    }
    return $query->execute()->fetchObject();
  }


  /**
   * Add tracking for an entity in a self::EXPORTED state.
   *
   * @param \Drupal\Core\Entity\EntityInterface $entity
   *   The entity for which to add tracking.
   * @param string $langcode
   *   The langcode of the tracking entity.
   *
   * @throws \Exception
   */
  public function track(EntityInterface $entity, $langcode) {
    $this->insertOrUpdate($entity, self::EXPORTED, $langcode);
  }

  /**
   * Remove tracking for an entity.
   *
   * @param string $entity
   *   *   The entity that is deleted.
   * @param string $langcode
   *   The langcode of the tracking entity.
   *
   * @return \Drupal\Core\Database\StatementInterface|int|null
   *   Database statement
   *
   * @throws \Exception
   */
  public function delete($entity, $langcode = '') {
    $this->insertOrUpdate($entity, self::DELETED, $langcode);
  }

  /**
   * Determines if an entity will be inserted or updated with a status.
   *
   * @param \Drupal\Core\Entity\EntityInterface $entity
   *   The entity to add tracking to.
   * @param string $status
   *   The status of the tracking.
   * @param string $langcode
   *   The langcode of the tracking entity.
   *
   * @return \Drupal\Core\Database\StatementInterface|int|null
   *   Database statement.
   *
   * @throws \Exception
   */
  protected function insertOrUpdate(EntityInterface $entity, $status, $langcode = '') {
    if ($entity instanceof EntityChangedInterface) {
      $modified = date('c', $entity->getChangedTime());
    }
    else {
      $modified = date('c');
    }
    $results = $this->get($entity->uuid(), $langcode);
    if ($results) {
      $values = ['modified' => $modified, 'status' => $status];
      $query = $this->database->update(self::EXPORT_TRACKING_TABLE)
        ->fields($values);
      $query->condition('entity_uuid', $entity->uuid());
      if (!empty($langcode)) {
        $query->condition('langcode', $langcode);
      }
      return $query->execute();
    }
    $values = [
      'entity_type' => $entity->getEntityTypeId(),
      'entity_id' => $entity->id(),
      'entity_uuid' => $entity->uuid(),
      'status' => $status,
      'langcode' => $langcode,
      'modified' => $modified,
    ];
    return $this->database->insert(self::EXPORT_TRACKING_TABLE)
      ->fields($values)
      ->execute();
  }

  /**
   * Checks if a particular entity uuid is tracked.
   *
   * @param string $uuid
   *   The uuid of an entity.
   * @param string $langcode
   *   The langcode of an entity.
   *
   * @return bool
   *   Whether or not the entity is tracked in the subscriber tables.
   */
  public function isTracked($uuid, $langcode) {
    $query = $this->database->select(self::EXPORT_TRACKING_TABLE, 't');
    $query->fields('t', ['entity_type', 'entity_id']);
    $query->condition('entity_uuid', $uuid);
    $query->condition('langcode', $langcode);

    return (bool) $query->execute()->fetchObject();
  }

}
