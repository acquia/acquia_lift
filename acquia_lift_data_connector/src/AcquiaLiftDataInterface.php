<?php
/**
 * Created by PhpStorm.
 * User: lisa.backer
 * Date: 8/25/15
 * Time: 2:24 PM
 */

namespace Drupal\acquia_lift_data_connector;


interface AcquiaLiftDataInterface {
  /**
   * Gets a list of available segments for the current configuration.
   *
   * @return array
   *   An array of segment names.
   */
  public function getSegments();
} 
