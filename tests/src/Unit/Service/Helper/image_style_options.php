<?php
/**
 * @file
 * Contains \Drupal\acquia_lift\Service\Helper\ImageStyleOptions.
 */

namespace Drupal\acquia_lift\Service\Helper {
  /**
   * Mock Drupal's image_style_options function.
   *
   * @param bool $include_empty
   * @return array
   */
  function image_style_options($include_empty = TRUE) {
    return ImageStyleOptions::$return;
  }

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
