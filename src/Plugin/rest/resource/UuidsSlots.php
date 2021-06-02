<?php

namespace Drupal\acquia_perz\Plugin\rest\resource;

use Drupal\rest\Plugin\ResourceBase;
use Drupal\rest\ResourceResponse;

/**
 * Annotation for get method
 *
 * @RestResource(
 *   id = "acquia_perz_uuids_slots_endpoint",
 *   label = @Translation("Acquia Perz: Uuids - Slots"),
 *   uri_paths = {
 *     "canonical" = "/api/acquia-perz/uuids-slots/{entity_type_id}"
 *   }
 * )
 */
class UuidsSlots extends ResourceBase {

  /**
   * Responds to GET requests.
   *
   * Returns a list of [uuid => slot].
   *
   */
  public function get($entity_type_id) {

    $storage = \Drupal::service('entity_type.manager')
      ->getStorage($entity_type_id);
    $query = $storage->getQuery()
      ->addTag('random')
      ->currentRevision()
      ->accessCheck()
      ->range(0, 3);
    $ids = $query->execute();
    $items = $storage->loadMultiple($ids);
    $uuids = [];
    foreach ($items as $item) {
      $uuids[] = $item->uuid();
    }
    $uuids_slots = [
      $uuids[0] => 'block-customblock1',
      $uuids[1] => 'block-customblock2',
      $uuids[2] => 'block-customblock3',
    ];
    $response = new ResourceResponse($uuids_slots);
    // Disable default caching.
    $current_user = \Drupal::currentUser();
    $response->addCacheableDependency(
      $current_user
    );
    return $response;
  }

}
