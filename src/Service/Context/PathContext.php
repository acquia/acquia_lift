<?php

namespace Drupal\acquia_lift\Service\Context;

use Drupal\acquia_lift\Service\Helper\PathMatcher;
use Drupal\acquia_lift\Service\Helper\SettingsHelper;
use Drupal\Component\Utility\Html;
use Drupal\Component\Utility\UrlHelper;
use Drupal\Core\Cache\CacheableDependencyInterface;
use Drupal\Core\Config\ConfigFactoryInterface;
use Drupal\Core\Path\CurrentPathStack;
use Drupal\user\UserInterface;
use Symfony\Component\HttpFoundation\RequestStack;

/**
 * Path Context extension of Lift Contexts
 */
class PathContext extends BaseContext implements CacheableDependencyInterface {

  /**
   * Acquia Lift settings.
   *
   * @var \Drupal\Core\Config\ImmutableConfig
   */
  private $settings;

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
    $this->settings = $config_factory->get('acquia_lift.settings');
    $this->credentialSettings = $this->settings->get('credential');
    $this->identitySettings = $this->settings->get('identity');
    $visibilitySettings = $this->settings->get('visibility');
    $this->requestPathPatterns = $visibilitySettings['path_patterns'];
    $this->currentPath = $current_path_stack->getPath();
    $this->pathMatcher = $pathMatcher;

    $this->setContextIdentityByRequest($request_stack);
  }

  /**
   * Should attach.
   *
   * @return bool
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
   * Set Path Context Identity by request stack's query parameters.
   *
   * @param \Symfony\Component\HttpFoundation\RequestStack $request_stack
   *   The request stack.
   */
  private function setContextIdentityByRequest(RequestStack $request_stack) {
    // Stop, if there is no "identity parameter".
    $identity_parameter = $this->identitySettings['identity_parameter'];
    if (empty($identity_parameter)) {
      return;
    }

    // Set cache contexts. This is done as if the identity parameter is set.
    $identity_type_parameter = $this->identitySettings['identity_type_parameter'];
    $query_names = [$identity_parameter, $identity_type_parameter];
    $this->setContextCacheByQueryNames($query_names);

    // Find the current URL queries.
    $query_string = $request_stack->getCurrentRequest()->getQueryString();
    $parsed_query_string = UrlHelper::parse('?' . $query_string);
    $queries = $parsed_query_string['query'];

    // Stop, if there is no or empty identity parameter in the query string.
    if (empty($queries[$identity_parameter])) {
      return;
    }

    // Gather the identity and identity type by configuration.
    $default_identity_type = $this->identitySettings['default_identity_type'];
    $identity = $queries[$identity_parameter];
    $identityType = empty($default_identity_type) ? SettingsHelper::DEFAULT_IDENTITY_TYPE_DEFAULT : $default_identity_type;
    if (!empty($identity_type_parameter) && !empty($queries[$identity_type_parameter])) {
      $identityType = $queries[$identity_type_parameter];
    }
    $this->setContextIdentity($identity, $identityType);
  }

  /**
   * Set Cache Context by query names.
   *
   * @todo Add implements CacheContextInterface instead of brewing our own.
   *
   * @param array $query_names
   *   The query names.
   */
  private function setContextCacheByQueryNames(array $query_names) {
    foreach ($query_names as $query_name) {
      if (empty($query_name)) {
        continue;
      }
      $this->cacheContexts[] = 'url.query_args:' . $query_name;
    }
  }

  /**
   * Set Context Identity by User.
   *
   * @param \Drupal\user\UserInterface $user
   *   User.
   */
  public function setContextIdentityByUser(UserInterface $user) {
    if (empty($this->identitySettings['capture_identity'])) {
      return;
    }

    $this->setContextIdentity($user->getEmail(), 'email');
  }

  /**
   * Set Context Identity.
   *
   * @param string $identity
   *   Identity.
   * @param string $identityType
   *   Identity type.
   */
  private function setContextIdentity($identity, $identityType) {
    // Sanitize string and output.
    $sanitized_identity = Html::escape($identity);
    $sanitized_identity_type = Html::escape($identityType);
    $this->htmlHeadContexts['identity:' . $sanitized_identity_type] = $sanitized_identity;
  }

  /**
   * {@inheritdoc}
   */
  public function getCacheMaxAge() {
    return $this->settings->getCacheMaxAge();
  }

  /**
   * {@inheritdoc}
   */
  public function getCacheContexts() {
    return $this->settings->getCacheContexts();
  }

  /**
   * {@inheritdoc}
   */
  public function getCacheTags() {
    return $this->settings->getCacheTags();
  }

}
