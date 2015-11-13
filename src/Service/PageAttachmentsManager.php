<?php

/**
 * @file
 * Contains \Drupal\acquia_lift\Service\Api\DataApi.
 */

namespace Drupal\acquia_lift\Service;

use Drupal\Core\Config\ConfigFactory;
use Drupal\acquia_lift\Entity\Credential;

class PageAttachmentsManager {
  /**
   * Acquia Lift credential.
   *
   * @var \Drupal\acquia_lift\Entity\Credential
   */
  private $credential;

  /**
   * Constructor.
   *
   * @param ConfigFactory $config_factory
   *   The config factory service
   */
  public function __construct(ConfigFactory $config_factory) {
    $credential_settings = $config_factory->get('acquia_lift.settings')->get('credential');
    $this->credential = new Credential($credential_settings);
    if (!$this->credential->isValid()) {
      throw new DataApiCredentialException('Acquia Lift credential is invalid.');
    }
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

    return TRUE;
  }
}
