<?php

/**
 * @file
 * Contains \Drupal\acquia_lift\Service\Manager\AttachmentsManager.
 */

namespace Drupal\acquia_lift\Service\Manager;

use Drupal\Core\Config\ConfigFactoryInterface;
use Drupal\acquia_lift\Service\Context\PageContext;
use Drupal\acquia_lift\Service\Context\PathContext;
use Drupal\acquia_lift\Service\Helper\SettingsHelper;

class AttachmentsManager {
  /**
   * Acquia Lift credential settings.
   *
   * @var array
   */
  private $credentialSettings;

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
    $this->credentialSettings = $config_factory->get('acquia_lift.settings')->get('credential');
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
    $settings['credential'] = SettingsHelper::getFrontEndCredentialSettings($this->credentialSettings);
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
