<?php

/**
 * @file
 * Contains \Drupal\acquia_lift\Service\Api\DataApi.
 */

namespace Drupal\acquia_lift\Service;

use Drupal\Core\Config\ConfigFactory;
use Drupal\Core\Path\CurrentPathStack;
use Drupal\Core\Path\AliasManager;
use Drupal\Core\Path\PathMatcher;
use Drupal\Component\Utility\Unicode;
use Drupal\acquia_lift\Entity\Credential;

class PageAttachmentsManager {
  /**
   * Alias manager.
   *
   * @var \Drupal\Core\Path\AliasManager
   */
  private $aliasManager;

  /**
   * Path matcher.
   *
   * @var \Drupal\Core\Path\PathMatcher
   */
  private $pathMatcher;

  /**
   * Acquia Lift credential.
   *
   * @var \Drupal\acquia_lift\Entity\Credential
   */
  private $credential;

  /**
   * Visibility.
   *
   * @var array
   */
  private $visibility;

  /**
   * Current path.
   *
   * @var string
   */
  private $currentPath;

  /**
   * Constructor.
   *
   * @param \Drupal\Core\Config\ConfigFactory $config_factory
   *   The config factory service.
   * @param \Drupal\Core\Path\CurrentPathStack $current_path_stack
   *   The current path service.
   * @param \Drupal\Core\Path\AliasManager $alias_manager
   *   The alias manager service.
   * @param \Drupal\Core\Path\PathMatcher $path_matcher
   *   The path matcher service.
   */
  public function __construct(ConfigFactory $config_factory, CurrentPathStack $current_path_stack, AliasManager $alias_manager, PathMatcher $path_matcher) {
    $settings = $config_factory->get('acquia_lift.settings');
    $credential_settings = $settings->get('credential');
    $this->credential = new Credential($credential_settings);
    $this->visibility = $settings->get('visibility');
    $this->currentPath = $current_path_stack->getPath();
    $this->aliasManager = $alias_manager;
    $this->pathMatcher = $path_matcher;
  }

  /**
   * Should attach.
   *
   * @return boolean
   *   True if should attach.
   */
  public function shouldAttach() {
    // Credential need to be filled.
    if (!$this->credential->isValid()) {
      return FALSE;
    }

    // Current path cannot match the path patterns.
    if ($this->matchRequestPath()) {
      return FALSE;
    }

    return TRUE;
  }

  /**
   * Determine if the request path falls into one of the allowed paths.
   *
   * @return boolean
   *   True if should attach.
   */
  private function matchRequestPath() {
    // Convert path to lowercase and match.
    $path_patterns = Unicode::strtolower($this->visibility['request_path_pages']);
    if ($this->pathMatcher->matchPath($this->currentPath, $path_patterns)) {
      return TRUE;
    }

    // Compare the lowercase path alias (if any) and internal path.
    $path_alias = Unicode::strtolower($this->aliasManager->getAliasByPath($this->currentPath));
    if (($this->currentPath != $path_alias) && $this->pathMatcher->matchPath($path_alias, $path_patterns)) {
      return TRUE;
    }

    return FALSE;
  }

  /**
   * Get Drupal JavaScript settings.
   *
   * @return array
   *   Settings.
   */
  public function getDrupalSettings() {
    $settings = array(
      'test' => 123,
    );

    return $settings;
  }

  /**
   * Get library.
   *
   * @return string
   *   Liberary identifier.
   */
  public function getLibrary() {
    return 'acquia_lift/acquia_lift';
  }
}
