<?php

namespace Drupal\acquia_perz\Commands;

use Drupal\Component\Uuid\Uuid;
use Drush\Commands\DrushCommands;

/**
 * Class AcquiaPerzMetricCommands.
 *
 * @package Drupal\acquia_perz\Commands
 */
class AcquiaPerzMetricCommands extends DrushCommands {

  const ENTITY_VIEWS_COUNTER_TABLE = 'acquia_perz_entity_counter';

  /**
   * AcquiaPerzMetricCommands constructor.
   */
  public function __construct() {
  }

  /**
   * Import metric of entities by entity type id.
   *
   * @param string $entity_type_id
   *   The entity type id.
   *
   * @command acquia:perz-metric-import-all
   * @aliases apm-ia
   *
   * @return false|string
   *   The json output if successful or false.
   *
   * @throws \Exception
   */
  public function importAll($entity_type_id = 'node') {
    $entity_types = \Drupal::entityTypeManager()->getDefinitions();
    if (!isset($entity_types[$entity_type_id])) {
      throw new \Exception("The provided entity type does not exist.");
    }
    $storage = \Drupal::service('entity_type.manager')
      ->getStorage($entity_type_id);
    $query = $storage->getQuery()
      ->currentRevision()
      ->accessCheck();
    $ids = $query->execute();
    $items = $storage->loadMultiple($ids);
    foreach ($items as $item) {
      $values[$item->uuid()] = [
        'entity_id' => $item->id(),
        'views_count' => rand(1, 5000000),
      ];
    }
    foreach ($values as $uuid => $value) {
      $this->insertOrUpdate($entity_type_id, $value['entity_id'], $uuid, $value['views_count']);
    }

    $this->output->writeln(dt("The metric for all '@entity_type_id' entities have been imported.", [
      '@entity_type_id' => $entity_type_id,
    ]));
  }

  /**
   * Import metric of single entity by uuid and entity type id.
   *
   * @param string $uuid
   *   The entity uuid.
   * @param string $entity_type_id
   *   The entity type id.
   *
   * @command acquia:perz-metric-import-single
   * @aliases apm-is
   *
   * @return false|string
   *   The json output if successful or false.
   *
   * @throws \Exception
   */
  public function importSingle($uuid, $entity_type_id = 'node') {
    if (!Uuid::isValid($uuid)) {
      throw new \Exception("The provided entity uuid is not valid.");
    }
    $entity_types = \Drupal::entityTypeManager()->getDefinitions();
    if (!isset($entity_types[$entity_type_id])) {
      throw new \Exception("The provided entity type does not exist.");
    }

    $entity = \Drupal::service('entity.repository')
      ->loadEntityByUuid($entity_type_id, $uuid);
    $views_count = rand(1, 5000000);
    $this->insertOrUpdate($entity_type_id, $entity->id(), $uuid, $views_count);
    $this->output->writeln(dt("The metric for '@uuid' entity has been imported.", [
      '@uuid' => $uuid,
    ]));
  }

  /**
   * Determines if an entity will be inserted or
   * updated with passed views count.
   *
   * @param string $entity_type_id
   * @param integer $entity_id
   * @param string $uuid
   * @param integer $views_count
   *
   * @return \Drupal\Core\Database\StatementInterface|int|string|null
   * @throws \Exception
   */
  protected function insertOrUpdate($entity_type_id, $entity_id, $uuid, $views_count) {
    $database = \Drupal::database();
    $timestamp = \Drupal::time()->getCurrentTime();
    $values = [
      'entity_type_id' => $entity_type_id,
      'entity_id' => $entity_id,
      'count' => $views_count,
      'timestamp' => $timestamp
    ];
    $query = $database->select(self::ENTITY_VIEWS_COUNTER_TABLE, 't')
      ->fields('t', ['entity_uuid']);
    $query->condition('entity_uuid', $uuid);
    $results = $query->execute()->fetchObject();
    // If we've previously tracked this thing, set its created date.
    if ($results) {
      $query = $database->update(self::ENTITY_VIEWS_COUNTER_TABLE)
        ->fields($values);
      $query->condition('entity_uuid', $uuid);
      return $query->execute();
    }
    $values['entity_uuid'] = $uuid;
    return $database->insert(self::ENTITY_VIEWS_COUNTER_TABLE)
      ->fields($values)
      ->execute();
  }

}
