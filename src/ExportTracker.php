<?php

namespace Drupal\acquia_perz;

use Drupal\Core\Database\Connection;

/**
 * The publisher tracker table class.
 */
class ExportTracker {

  const DELETED = 'deleted';

  const DELETE_TIMEOUT = 'delete_timeout';

  const EXPORTED = 'exported';

  const EXPORT_TIMEOUT = 'export_timeout';

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
   * Clear tracking for an entity.
   *
   * @param string $entity_type_id
   *   The entity type id.
   * @param integer $entity_id
   *   The entity id.
   *
   * @throws \Exception
   */
  public function clear($entity_type_id, $entity_id) {
    $query = $this->database->delete(self::EXPORT_TRACKING_TABLE);
    $query->condition('entity_type', $entity_type_id);
    $query->condition('entity_id', $entity_id);
    $query->execute();
  }

  /**
   * Add tracking for an entity in a self::EXPORTED state.
   *
   * @param string $entity_type_id
   *   The entity type id.
   * @param integer $entity_id
   *   The entity id.
   * @param string $entity_uuid
   *   The entity uuid.
   * @param string $langcode
   *   The langcode of the tracking entity.
   *
   * @throws \Exception
   */
  public function export($entity_type_id, $entity_id, $entity_uuid, $langcode) {
    $this->insertOrUpdate($entity_type_id, $entity_id, $entity_uuid, self::EXPORTED, $langcode);
  }

  /**
   * Add tracking for an entity in a self::EXPORT_TIMEOUT state.
   *
   * @param string $entity_type_id
   *   The entity type id.
   * @param integer $entity_id
   *   The entity id of the entity.
   * @param string $entity_uuid
   *   The entity uuid of the entity.
   * @param string $langcode
   *   The langcode of the tracking entity.
   *
   * @throws \Exception
   */
  public function exportTimeout($entity_type_id, $entity_id, $entity_uuid, $langcode) {
    $this->insertOrUpdate($entity_type_id, $entity_id, $entity_uuid, self::EXPORT_TIMEOUT, $langcode);
  }

  /**
   * Delete tracking for an entity.
   *
   * @param string $entity_type_id
   *   The entity type id.
   * @param integer $entity_id
   *   The entity id of the entity.
   * @param string $entity_uuid
   *   The entity uuid of the entity.
   * @param string $langcode
   *   The langcode of the tracking entity.
   *
   * @return \Drupal\Core\Database\StatementInterface|int|null
   *   Database statement
   *
   * @throws \Exception
   */
  public function delete($entity_type_id, $entity_id, $entity_uuid, $langcode = '') {
    return $this->insertOrUpdate($entity_type_id, $entity_id, $entity_uuid, self::DELETED, $langcode);
  }

  /**
   * Delete timeout tracking for an entity.
   *
   * @param string $entity_type_id
   *   The entity type id.
   * @param integer $entity_id
   *   The entity id of the entity.
   * @param string $entity_uuid
   *   The entity uuid of the entity.
   * @param string $langcode
   *   The langcode of the tracking entity.
   *
   * @return \Drupal\Core\Database\StatementInterface|int|null
   *   Database statement
   *
   * @throws \Exception
   */
  public function deleteTimeout($entity_type_id, $entity_id, $entity_uuid, $langcode = '') {
    return $this->insertOrUpdate($entity_type_id, $entity_id, $entity_uuid, self::DELETE_TIMEOUT, $langcode);
  }

  /**
   * Determines if an entity will be inserted or updated with a status.
   *
   * @param string $entity_type_id
   *   The entity type id of the entity.
   * @param integer $entity_id
   *   The entity id of the entity.
   * @param string $entity_uuid
   *   The entity uuid of the entity.
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
  protected function insertOrUpdate($entity_type_id, $entity_id, $entity_uuid, $status, $langcode = '') {
    $modified = date('c');
    $results = $this->get($entity_uuid, $langcode);
    if ($results) {
      $values = ['modified' => $modified, 'status' => $status];
      $query = $this->database->update(self::EXPORT_TRACKING_TABLE)
        ->fields($values);
      $query->condition('entity_uuid', $entity_uuid);
      if (!empty($langcode)) {
        $query->condition('langcode', $langcode);
      }
      return $query->execute();
    }
    $values = [
      'entity_type' => $entity_type_id,
      'entity_id' => $entity_id,
      'entity_uuid' => $entity_uuid,
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
