<?php

/**
 * @file
 * Contains \Drupal\acquia_lift\Service\Context\PathContext.
 */

namespace Drupal\acquia_lift\Service\Context;

use Symfony\Component\HttpFoundation\RequestStack;
use Drupal\Core\Config\ConfigFactoryInterface;
use Drupal\Core\Path\CurrentPathStack;
use Drupal\Component\Utility\UrlHelper;

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
  private $identity;

  /**
   * Constructor.
   *
   * @param \Drupal\Core\Config\ConfigFactoryInterface $config_factory
   *   The config factory service.
   * @param \Drupal\Core\Path\CurrentPathStack $current_path_stack
   *   The current path service.
   * @param \Symfony\Component\HttpFoundation\RequestStack $request_stack
   *   The request stack.
   */
  public function __construct(ConfigFactoryInterface $config_factory, CurrentPathStack $current_path_stack, RequestStack $request_stack) {
    $visibility = $config_factory->get('acquia_lift.settings')->get('visibility');
    $this->requestPathPatterns = $visibility['path_patterns'];
    $this->currentPath = $current_path_stack->getPath();

    // Set identity.
    $identityConfig = $config_factory->get('acquia_lift.settings')->get('identity');
    if (!$identityConfig['capture_identity'] || empty($identityConfig['identity_parameter'])) {
      return;
    }
    $query_string = $request_stack->getCurrentRequest()->getQueryString();
    $parsed_query_string = UrlHelper::parse('?'.$query_string);
    $queries = $parsed_query_string['query'];

    $identity_parameter = $identityConfig['identity_parameter'];
    $identity_type_parameter = $identityConfig['identity_type_parameter'];
    $default_identity_type = $identityConfig['default_identity_type'];
    if (!empty($queries[$identity_parameter])) {
      $this->identity['identity'] = $queries[$identity_parameter];
      $this->identity['identityType'] = empty($default_identity_type) ? SELF::DEFAULT_IDENTITY_TYPE_DEFAULT : $default_identity_type;
      if (!empty($identity_type_parameter) && isset($queries[$identity_type_parameter])) {
        $this->identity['identityType'] = $queries[$identity_type_parameter];
      }
    }
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
   * @return array|NULL
   *   Identity.
   */
  public function getIdentity() {
    return $this->identity;
  }
}
