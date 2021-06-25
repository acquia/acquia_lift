<?php

namespace Drupal\acquia_perz1\Plugin\rest\resource;

use Drupal\rest\Plugin\ResourceBase;
use Drupal\rest\ResourceResponse;

/**
 * @RestResource(
 *   id = "acquia_perz1_slow_endpoint",
 *   label = @Translation("Acquia Perz - Slow Endpoint for testing timeouts"),
 *   uri_paths = {
 *     "canonical" = "/api/slow_endpoint"
 *   }
 * )
 */
class SlowEndpoint extends ResourceBase {

  /**
   * Responds to GET requests.
   *
   * Returns a list of bundles for specified entity.
   *
   */
  public function get() {
    sleep(3);
    $response_result['data'] = 'any';
    $response = new ResourceResponse($response_result);
    // Disable default caching.
    $current_user = \Drupal::currentUser();
    $response->addCacheableDependency(
      $current_user
    );
    return $response;
  }

}
