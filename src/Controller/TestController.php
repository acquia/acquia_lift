<?php
/**
 * Created by PhpStorm.
 * User: lisa.backer
 * Date: 8/25/15
 * Time: 8:04 AM
 */

namespace Drupal\acquia_lift\Controller;


use Drupal\Core\Controller\ControllerBase;

class TestController extends ControllerBase {
  public function test() {
    $webapi = \Drupal::service('acquia_lift_data_connector');
    $segments = $webapi->getSegments();
    $build['segments'] = array(
      '#markup' => print_r($segments, TRUE),
    );
    return $build;
  }
} 
