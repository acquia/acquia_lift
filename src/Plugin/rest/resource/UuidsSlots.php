<?php

namespace Drupal\acquia_perz2\Plugin\rest\resource;

use Drupal\rest\Plugin\ResourceBase;
use Drupal\rest\ResourceResponse;

/**
 * Annotation for get method
 *
 * @RestResource(
 *   id = "acquia_perz2_uuids_slots_endpoint",
 *   label = @Translation("acquia perz2: Uuids - Slots"),
 *   uri_paths = {
 *     "canonical" = "/api/acquia-perz2/uuids-slots/{entity_type_id}"
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
    $slot1_id = \Drupal::config('acquia_perz2.settings')->get('slot1_id');
    $slot2_id = \Drupal::config('acquia_perz2.settings')->get('slot2_id');
    $slot3_id = \Drupal::config('acquia_perz2.settings')->get('slot3_id');
    $uuids_slots = [];
    if (!empty($slot1_id)) {
      $uuids_slots[$uuids[0]] = $slot1_id;
    }
    if (!empty($slot2_id)) {
      $uuids_slots[$uuids[1]] = $slot2_id;
    }
    if (!empty($slot3_id)) {
      $uuids_slots[$uuids[2]] = $slot3_id;
    }
    $response = new ResourceResponse($uuids_slots);
    // Disable default caching.
    $current_user = \Drupal::currentUser();
    $response->addCacheableDependency(
      $current_user
    );
    return $response;
  }

}
