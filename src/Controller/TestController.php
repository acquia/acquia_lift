<?php

/**
 * @file
 * Contains \Drupal\acquia_lift\Controller\TestController.
 */

namespace Drupal\acquia_lift\Controller;

use Drupal\Core\Controller\ControllerBase;

class TestController extends ControllerBase {
  public function test() {
    $webapi = \Drupal::service('acquia_lift.data_connector');
    $segments = $webapi->getSegments();
    $build['segments'] = array(
      '#markup' => print_r($segments, TRUE),
    );
    return $build;
  }
} 
