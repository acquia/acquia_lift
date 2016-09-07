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
   * @param array $args An array in the form ['from' => 'to', ...].
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

  /**
   * PageContextTest uses file_create_url(),
   * which *is* available when using the Simpletest test runner, but not when
   * using the PHPUnit test runner; hence this hack.
   */
  if (!function_exists('file_create_url')) {

    /**
     * Temporary mock for file_create_url(), until that is moved into
     * Component/Utility.
     */
    function file_create_url($uri) {
      return 'file_create_url:' . $uri;
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
