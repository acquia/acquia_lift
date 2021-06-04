<?php

namespace Drupal\acquia_perz\Plugin\rest\resource;

use Drupal\rest\Plugin\ResourceBase;
use Drupal\rest\ResourceResponse;
use Drupal\Component\Uuid\Uuid;

/**
 * Annotation for get method
 *
 * @RestResource(
 *   id = "acquia_perz_views_count_endpoint",
 *   label = @Translation("Ac Perz - Views Count"),
 *   uri_paths = {
 *     "canonical" = "/api/acquia-perz/views-count/{entity_type_id}/{arg}"
 *   }
 * )
 */
class ViewsCount extends ResourceBase {

  const ENTITY_VIEWS_COUNTER_TABLE = 'acquia_perz_entity_counter';

  /**
   *
   * @param string $entity_type_id
   *   The entity type id.
   * @param string $arg
   *  Allowed values:
   *  - uuid value: returns view count of specific entity.
   *  - 'list': returns list of entities and corresponding view count.
   * @return \Drupal\rest\ResourceResponse
   */
  public function get($entity_type_id, $arg) {
    $values = [];
    if (Uuid::isValid($arg)) {
      $entity = \Drupal::service('entity.repository')
        ->loadEntityByUuid($entity_type_id, $arg);
      $values[$arg] = [
        'entity_id' => $entity->id(),
        'views_count' => rand(1, 5000000),
      ];
    }
    elseif ($arg === 'list') {
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
    }
    foreach ($values as $uuid => $value) {
      $this->insertOrUpdate($value['entity_id'], $uuid, $value['views_count']);
    }
    $result = ['status' => TRUE];
    $response = new ResourceResponse($result);
    // Disable default caching.
    $current_user = \Drupal::currentUser();
    $response->addCacheableDependency(
      $current_user
    );
    return $response;
  }

  /**
   * Determines if an entity will be inserted or
   * updated with passed views count.
   *
   * @param integer $entity_id
   * @param string $uuid
   * @param integer $views_count
   *
   * @return \Drupal\Core\Database\StatementInterface|int|string|null
   * @throws \Exception
   */
  protected function insertOrUpdate($entity_id, $uuid, $views_count) {
    $database = \Drupal::database();
    $timestamp = \Drupal::time()->getCurrentTime();
    $values = [
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
