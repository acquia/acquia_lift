<?php

/**
 * @file
 * Contains \Drupal\acquia_lift\Entity\Credential.
 */

namespace Drupal\acquia_lift\Entity;

use Drupal\Component\Utility\UrlHelper;

/**
 * Defines the Credential entity class.
 */
class Credential {
  /**
   * Credential.
   *
   * @var array $credential
   */
  private $credential;

  /**
   * Constructor.
   *
   * @param array|NULL $credential
   *  Credential
   */
  public function __construct($credential) {
    $this->credential = $credential ?: [];
  }

  /**
   * Get account name.
   *
   * @return string
   *   Account name.
   */
  public function getAccountName() {
    return $this->credential['account_name'];
  }

  /**
   * Get customer site.
   *
   * @return string
   *   Customer site.
   */
  public function getCustomerSite() {
    return $this->credential['customer_site'];
  }

  /**
   * Get API URL.
   *
   * @return string
   *   API URL.
   */
  public function getApiUrl() {
    return $this->credential['api_url'];
  }

  /**
   * Get access key.
   *
   * @return string
   *   Access key.
   */
  public function getAccessKey() {
    return $this->credential['access_key'];
  }

  /**
   * Get secret key.
   *
   * @return string
   *   Secret key.
   */
  public function getSecretKey() {
    return $this->credential['secret_key'];
  }

  /**
   * Get JavaScript path.
   *
   * @return string
   *   JavaScript path.
   */
  public function getJsPath() {
    return $this->credential['js_path'];
  }

  /**
   * Get front-end config.
   *
   * @return array
   *   Get front end config array.
   */
  public function getFrontEndConfig() {
    return [
      'account_name' => $this->getAccountName(),
      'customer_site' => $this->getCustomerSite(),
      'js_path' => $this->getJsPath(),
    ];
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
    // Required credential need to be filled.
    if (empty($this->getAccountName()) ||
      empty($this->getApiUrl()) ||
      empty($this->getAccessKey()) ||
      empty($this->getSecretKey()) ||
      empty($this->getJsPath())
    ) {
      return FALSE;
    }

    // URLs need to be valid.
    if (!UrlHelper::isValid($this->getApiUrl()) || !UrlHelper::isValid($this->getJsPath())) {
      return FALSE;
    }

    return TRUE;
  }
}
