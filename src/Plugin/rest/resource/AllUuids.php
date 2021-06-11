<?php

namespace Drupal\acquia_perz\Plugin\rest\resource;

use Drupal\rest\Plugin\ResourceBase;
use Drupal\rest\ResourceResponse;

/**
 * Annotation for get method
 *
 * @RestResource(
 *   id = "acquia_perz_all_uuids_endpoint",
 *   label = @Translation("Acquia Perz: All uuids by entity type"),
 *   uri_paths = {
 *     "canonical" = "/api/acquia-perz/all-uuids/{entity_type_id}"
 *   }
 * )
 */
class AllUuids extends ResourceBase {

  /**
   * Responds to GET requests.
   *
   * Returns a list of all content uuids.
   *
   */
  public function get($entity_type_id) {
    $storage = \Drupal::service('entity_type.manager')
      ->getStorage($entity_type_id);
    $query = $storage->getQuery()
      ->currentRevision()
      ->accessCheck();
    $ids = $query->execute();
    $items = $storage->loadMultiple($ids);
    $uuids = [];
    foreach ($items as $item) {
      $uuids[] = $item->uuid();
    }
    $response = new ResourceResponse($uuids);
    // Disable default caching.
    $current_user = \Drupal::currentUser();
    $response->addCacheableDependency(
      $current_user
    );
    return $response;
  }

}
