<?php

/**
 * This file polyfills all the functions that are defined in global namespace
 * in Drupal environment but not available in PHPUnit context.
 */

namespace {
  use Drupal\Tests\acquia_lift\Unit\Polyfill\Drupal\ImageStyleOptions;

  /**
   * Mock Drupal's t function.
   *
   * @param $string String to be translated.
   * @param array $args An array in the form array('from' => 'to', ...).
   * @return string
   */
  if (!function_exists('t')) {
    function t($string, array $args = []) {
      return strtr($string, $args);
    }
  }

  /**
   * Mock Drupal's image_style_options function.
   *
   * @param bool $include_empty
   * @return array
   */
  if (!function_exists('image_style_options')) {
    function image_style_options($include_empty = TRUE) {
      return ImageStyleOptions::$return;
    }
  }
}

namespace Drupal\Tests\acquia_lift\Unit\Polyfill\Drupal {
  /**
   * Class ImageStyleOptions to manage image_style_options.
   */
  class ImageStyleOptions {
    /**
     * @var array image_style_options function's return value.
     */
    public static $return = [];
  }
}
