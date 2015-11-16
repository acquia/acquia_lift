<?php

/**
 * @file
 * Contains \Drupal\acquia_lift\Service\Context\PathContext.
 */

namespace Drupal\acquia_lift\Service\Context;

use Drupal\Core\Config\ConfigFactory;
use Drupal\Core\Path\CurrentPathStack;

class PathContext {
  /**
   * Request path patterns (exclusion).
   *
   * @var array
   */
  private $requestPathPatterns;

  /**
   * Current path.
   *
   * @var string
   */
  private $currentPath;

  /**
   * Current parameters.
   *
   * @var array
   */
  private $currentParameters;

  /**
   * Constructor.
   *
   * @param \Drupal\Core\Config\ConfigFactory $config_factory
   *   The config factory service.
   * @param \Drupal\Core\Path\CurrentPathStack $current_path_stack
   *   The current path service.
   */
  public function __construct(ConfigFactory $config_factory, CurrentPathStack $current_path_stack) {
    $visibility = $config_factory->get('acquia_lift.settings')->get('visibility');
    $this->requestPathPatterns = $visibility['path_patterns'];
    $this->currentPath = $current_path_stack->getPath();
  }

  /**
   * Get request path patterns.
   *
   * @return string
   *   Request path patterns (to be excluded).
   */
  public function getRequestPathPatterns() {
    return $this->requestPathPatterns;
  }

  /**
   * Get current path.
   *
   * @return string
   *   Current path.
   */
  public function getCurrentPath() {
    return $this->currentPath;
  }
}
