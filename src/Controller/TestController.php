<?php

/**
 * @file
 * Contains \Drupal\acquia_lift\Controller\TestController.
 */

namespace Drupal\acquia_lift\Controller;

use Drupal\Core\Controller\ControllerBase;

class TestController extends ControllerBase {
  public function test() {
    $data_api = \Drupal::service('acquia_lift.service.api.data_api');
    $segments = $data_api->getSegments();
    $build['segments'] = [
      '#markup' => print_r($segments, TRUE),
    ];
    return $build;
  }
} 
