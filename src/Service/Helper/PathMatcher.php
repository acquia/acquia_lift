<?php

namespace Drupal\acquia_lift\Service\Helper;

use Drupal\Core\Path\PathMatcherInterface;
use Drupal\path_alias\AliasManagerInterface;

/**
 * Path Matcher Helper.
 */
class PathMatcher {

  /**
   * Alias manager.
   *
   * @var \Drupal\path_alias\AliasManagerInterface
   */
  private $aliasManager;

  /**
   * Path matcher.
   *
   * @var \Drupal\Core\Path\PathMatcherInterface
   */
  private $pathMatcher;

  /**
   * Constructor.
   *
   * @param \Drupal\path_alias\AliasManagerInterface $alias_manager
   *   The alias manager service.
   * @param \Drupal\Core\Path\PathMatcherInterface $path_matcher
   *   The path matcher service.
   */
  public function __construct(AliasManagerInterface $alias_manager, PathMatcherInterface $path_matcher) {
    $this->aliasManager = $alias_manager;
    $this->pathMatcher = $path_matcher;
  }

  /**
   * Determine if the path falls into one of the allowed paths.
   *
   * @param string $path
   *   The actual path that's being matched by.
   * @param string $path_patterns
   *   The path patterns that the path is being matched to.
   *
   * @return bool
   *   True if should attach.
   */
  public function match($path, $path_patterns) {
    // Convert path to lowercase and match.
    $converted_path = mb_strtolower($path);
    $converted_path_patterns = mb_strtolower($path_patterns);
    if ($this->pathMatcher->matchPath($converted_path, $converted_path_patterns)) {
      return TRUE;
    }

    // Compare the lowercase path alias (if any) and internal path.
    $converted_path_alias = mb_strtolower($this->aliasManager->getAliasByPath($converted_path));
    if (($converted_path != $converted_path_alias) && $this->pathMatcher->matchPath($converted_path_alias, $converted_path_patterns)) {
      return TRUE;
    }

    return FALSE;
  }

}
