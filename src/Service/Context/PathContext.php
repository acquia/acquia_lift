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
use Drupal\Component\Utility\Html;

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
    $identity_config = $config_factory->get('acquia_lift.settings')->get('identity');
    $this->setIdentity($identity_config, $request_stack);
  }

  /**
   * Set Identity.
   *
   * @return array $identity_config
   *   Identity config.
   * @param \Symfony\Component\HttpFoundation\RequestStack $request_stack
   *   The request stack.
   */
  private function setIdentity($identity_config, $request_stack) {
    // Stop, if "capture identity" flag is not on or there is no "identity parameter".
    if (!$identity_config['capture_identity'] || empty($identity_config['identity_parameter'])) {
      return;
    }

    // Find the current URL queries.
    $query_string = $request_stack->getCurrentRequest()->getQueryString();
    $parsed_query_string = UrlHelper::parse('?'.$query_string);
    $queries = $parsed_query_string['query'];
    $identity_parameter = $identity_config['identity_parameter'];

    // Stop, if there is no or empty identity parameter in the query string.
    if (empty($queries[$identity_parameter])) {
      return;
    }

    // Gather the identity and identity type by configuration.
    $identity_type_parameter = $identity_config['identity_type_parameter'];
    $default_identity_type = $identity_config['default_identity_type'];
    $identity = $queries[$identity_parameter];
    $identityType = empty($default_identity_type) ? SELF::DEFAULT_IDENTITY_TYPE_DEFAULT : $default_identity_type;
    if (!empty($identity_type_parameter) && isset($queries[$identity_type_parameter])) {
      $identityType = $queries[$identity_type_parameter];
    }

    // Sanitize string and output.
    $this->identity['identity'] = Html::escape($identity);
    $this->identity['identityType'] = Html::escape($identityType);
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
