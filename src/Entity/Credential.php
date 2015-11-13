<?php

/**
 * @file
 * Contains \Drupal\acquia_lift\Entity\Credential.
 */

namespace Drupal\acquia_lift\Entity;

use Drupal\Core\Config\Config;

/**
 * Defines the Credential entity class.
 */
class Credential {
  /**
   * Config.
   *
   * @var \Drupal\Core\Config\Config $config
   */
  private $config;

  /**
   * Constructor.
   *
   * @param \Drupal\Core\Config\Config $config
   *  Config
   */
  public function __construct(Config $config) {
    $this->config = $config;
  }

  /**
   * Get account name.
   *
   * @return string
   *   Account name.
   */
  public function getAccountName() {
    return $this->config->get('account_name');
  }

  /**
   * Get customer site.
   *
   * @return string
   *   Customer site.
   */
  public function getCustomerSite() {
    return $this->config->get('customer_site');
  }

  /**
   * Get API URL.
   *
   * @return string
   *   API URL.
   */
  public function getApiUrl() {
    return $this->config->get('api_url');
  }

  /**
   * Get access key.
   *
   * @return string
   *   Access key.
   */
  public function getAccessKey() {
    return $this->config->get('access_key');
  }

  /**
   * Get secret key.
   *
   * @return string
   *   Secret key.
   */
  public function getSecretKey() {
    return $this->config->get('secret_key');
  }

  /**
   * Get JavaScript path.
   *
   * @return string
   *   JavaScript path.
   */
  public function getJsPath() {
    return $this->config->get('js_path');
  }

  /**
   * Is a valid credential.
   *
   * @return boolean
   *   True if is a valid credential.
   *
   * @todo This is quick and primitive. Class is to be updated later.
   */
  public function isValid() {
    return !empty($this->getAccountName()) &&
      !empty($this->getApiUrl()) &&
      !empty($this->getAccessKey()) &&
      !empty($this->getSecretKey()) &&
      !empty($this->getJsPath());
  }
}
