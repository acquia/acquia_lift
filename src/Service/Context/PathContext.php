<?php

namespace Drupal\acquia_lift\Service\Context;

use Symfony\Component\HttpFoundation\RequestStack;
use Drupal\Core\Config\ConfigFactoryInterface;
use Drupal\Core\Path\CurrentPathStack;
use Drupal\Component\Utility\UrlHelper;
use Drupal\Component\Utility\Html;
use Drupal\user\UserInterface;
use Drupal\acquia_lift\Service\Helper\PathMatcher;
use Drupal\acquia_lift\Service\Helper\SettingsHelper;

class PathContext {
  /**
   * Acquia Lift credential settings.
   *
   * @var array
   */
  private $credentialSettings;

  /**
   * Identity settings.
   *
   * @var array
   */
  private $identitySettings;

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
   * Path matcher.
   *
   * @var \Drupal\acquia_lift\Service\Helper\PathMatcher
   */
  private $pathMatcher;

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
   * @param \Drupal\acquia_lift\Service\Helper\PathMatcher $pathMatcher
   *   The path matcher.
   */
  public function __construct(ConfigFactoryInterface $config_factory, CurrentPathStack $current_path_stack, RequestStack $request_stack, PathMatcher $pathMatcher) {
    $settings = $config_factory->get('acquia_lift.settings');
    $credential_settings = $settings->get('credential');
    $identity_settings = $settings->get('identity');
    $visibilitySettings = $settings->get('visibility');

    $this->credentialSettings = $credential_settings;
    $this->identitySettings = $identity_settings;
    $this->requestPathPatterns = $visibilitySettings['path_patterns'];
    $this->currentPath = $current_path_stack->getPath();
    $this->pathMatcher = $pathMatcher;

    $this->setIdentityByRequest($request_stack);
  }

  /**
   * Should attach.
   *
   * @return boolean
   *   True if should attach.
   */
  public function shouldAttach() {
    // Should not attach if credential is invalid.
    if (SettingsHelper::isInvalidCredential($this->credentialSettings)) {
      return FALSE;
    }

    // Should not attach if current path match the path patterns.
    if ($this->pathMatcher->match($this->currentPath, $this->requestPathPatterns)) {
      return FALSE;
    }

    // Should attach.
    return TRUE;
  }

  /**
   * Set Identity by request stack's query parameters.
   *
   * @param \Symfony\Component\HttpFoundation\RequestStack $request_stack
   *   The request stack.
   */
  private function setIdentityByRequest($request_stack) {
    // Stop, if there is no "identity parameter".
    if (empty($this->identitySettings['identity_parameter'])) {
      return;
    }

    // Find the current URL queries.
    $query_string = $request_stack->getCurrentRequest()->getQueryString();
    $parsed_query_string = UrlHelper::parse('?' . $query_string);
    $queries = $parsed_query_string['query'];
    $identity_parameter = $this->identitySettings['identity_parameter'];

    // Stop, if there is no or empty identity parameter in the query string.
    if (empty($queries[$identity_parameter])) {
      return;
    }

    // Gather the identity and identity type by configuration.
    $identity_type_parameter = $this->identitySettings['identity_type_parameter'];
    $default_identity_type = $this->identitySettings['default_identity_type'];
    $identity = $queries[$identity_parameter];
    $identityType = empty($default_identity_type) ? SettingsHelper::DEFAULT_IDENTITY_TYPE_DEFAULT : $default_identity_type;
    if (!empty($identity_type_parameter) && isset($queries[$identity_type_parameter])) {
      $identityType = $queries[$identity_type_parameter];
    }

    $this->setIdentity($identity, $identityType);
  }

  /**
   * Set Identity by User.
   *
   * @param \Drupal\user\UserInterface $user
   *   User.
   */
  public function setIdentityByUser(UserInterface $user) {
    if (empty($this->identitySettings['capture_identity'])) {
      return;
    }

    $this->setIdentity($user->getEmail(), 'email');
  }

  /**
   * Set Identity.
   *
   * @param string $identity
   *   Identity.
   * @param string $identityType
   *   Identity type.
   */
  private function setIdentity($identity, $identityType) {
    // Sanitize string and output.
    $this->identity['identity'] = Html::escape($identity);
    $this->identity['identityType'] = Html::escape($identityType);
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
