<?php

/**
 * @file
 * Contains \Drupal\acquia_lift\Service\Page\PageAttachmentsManager.
 */

namespace Drupal\acquia_lift\Service\Page;

use Drupal\Core\Config\ConfigFactoryInterface;
use Drupal\acquia_lift\Entity\Credential;
use Drupal\acquia_lift\Service\Context\PageContext;
use Drupal\acquia_lift\Service\Context\PathContext;

class PageAttachmentsManager {
  /**
   * Acquia Lift credential.
   *
   * @var \Drupal\acquia_lift\Entity\Credential
   */
  private $credential;

  /**
   * Page context.
   *
   * @var \Drupal\acquia_lift\Service\Context\PageContext
   */
  private $pageContext;

  /**
   * Path context.
   *
   * @var \Drupal\acquia_lift\Service\Context\PathContext
   */
  private $pathContext;

  /**
   * Path matcher.
   *
   * @var \Drupal\acquia_lift\Service\Page\PathMatcher
   */
  private $pathMatcher;

  /**
   * Constructor.
   *
   * @param \Drupal\Core\Config\ConfigFactoryInterface $config_factory
   *   The config factory service.
   * @param \Drupal\acquia_lift\Service\Context\PageContext $pageContext
   *   The page context.
   * @param \Drupal\acquia_lift\Service\Context\PathContext $pathContext
   *   The path context.
   * @param \Drupal\acquia_lift\Service\Page\PathMatcher $pathMatcher
   *   The path matcher.
   */
  public function __construct(ConfigFactoryInterface $config_factory, PageContext $pageContext, PathContext $pathContext, PathMatcher $pathMatcher) {
    $settings = $config_factory->get('acquia_lift.settings');
    $credential_settings = $settings->get('credential');
    $this->credential = new Credential($credential_settings);
    $this->pageContext = $pageContext;
    $this->pathContext = $pathContext;
    $this->pathMatcher = $pathMatcher;
  }

  /**
   * Should attach.
   *
   * @return boolean
   *   True if should attach.
   */
  public function shouldAttach() {
    // Should not attach if credential is invalid.
    if (!$this->credential->isValid()) {
      return FALSE;
    }

    // Should not attach if current path match the path patterns.
    $path = $this->pathContext->getCurrentPath();
    $pathPatterns = $this->pathContext->getRequestPathPatterns();
    if ($this->pathMatcher->match($path, $pathPatterns)) {
      return FALSE;
    }

    // Should attach.
    return TRUE;
  }

  /**
   * Get Drupal JavaScript settings.
   *
   * @return array
   *   Settings.
   */
  public function getDrupalSettings() {
    $settings['credential'] = $this->credential->getFrontEndConfig();
    $settings['pageContext'] = $this->pageContext->getAll();
    $settings['identity'] = $this->pathContext->getIdentity();

    return $settings;
  }

  /**
   * Get library.
   *
   * @return string
   *   Library identifier.
   */
  public function getLibrary() {
    return 'acquia_lift/acquia_lift';
  }
}
