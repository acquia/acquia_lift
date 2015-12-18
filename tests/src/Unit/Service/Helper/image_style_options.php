<?php

namespace Drupal\acquia_lift\Service\Helper {
  function image_style_options($include_empty = TRUE) {
    return ImageStyleOptions::$returnValue;
  }

  class ImageStyleOptions {
    public static $returnValue = [];
  }
}
