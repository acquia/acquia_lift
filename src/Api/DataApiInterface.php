<?php

/**
 * @file
 * Contains \Drupal\acquia_lift\Api\DataApiInterface.
 */

namespace Drupal\acquia_lift\Api;

interface DataApiInterface {
  /**
   * Gets a list of available segments for the current configuration.
   *
   * @return array
   *   An array of segment names.
   */
  public function getSegments();
} 
