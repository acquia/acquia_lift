<?php

/**
 * @file
 * Contains \Drupal\acquia_lift\AcquiaLiftDataInterface.
 */

namespace Drupal\acquia_lift;

interface AcquiaLiftDataInterface {
  /**
   * Gets a list of available segments for the current configuration.
   *
   * @return array
   *   An array of segment names.
   */
  public function getSegments();
} 
