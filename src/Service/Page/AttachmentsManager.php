<?php

/**
 * @file
 * Contains \Drupal\acquia_lift\Service\Page\AttachmentsManager.
 */

namespace Drupal\acquia_lift\Service\Page;

use Drupal\Core\Config\ConfigFactoryInterface;
use Drupal\acquia_lift\Entity\Credential;
use Drupal\acquia_lift\Service\Context\PageContext;
use Drupal\acquia_lift\Service\Context\PathContext;

class AttachmentsManager {
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
   * Constructor.
   *
   * @param \Drupal\Core\Config\ConfigFactoryInterface $config_factory
   *   The config factory service.
   * @param \Drupal\acquia_lift\Service\Context\PageContext $pageContext
   *   The page context.
   * @param \Drupal\acquia_lift\Service\Context\PathContext $pathContext
   *   The path context.
   */
  public function __construct(ConfigFactoryInterface $config_factory, PageContext $pageContext, PathContext $pathContext) {
    $settings = $config_factory->get('acquia_lift.settings');
    $credential_settings = $settings->get('credential');

    $this->credential = new Credential($credential_settings);
    $this->pageContext = $pageContext;
    $this->pathContext = $pathContext;
  }

  /**
   * Get Drupal settings.
   *
   * @return array
   *   Settings.
   */
  public function getDrupalSettings() {
    $settings['credential'] = $this->credential->getFrontEndConfig();
    $settings['pageContext'] = $this->pageContext->getAll();
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
