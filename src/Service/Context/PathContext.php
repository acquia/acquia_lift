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
   * Default identity type's default value.
   */
  const DEFAULT_IDENTITY_TYPE_DEFAULT = 'email';

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
   * Identity.
   *
   * @var array
   */
  private $identity = array(
    'identity' => '',
    'identity_type' => self::DEFAULT_IDENTITY_TYPE_DEFAULT,
  );

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

  /**
   * Get identity.
   *
   * @return string
   *   Identity.
   */
  public function getIdentity() {
    return $this->identity;
  }
}
