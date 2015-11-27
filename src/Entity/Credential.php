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
   * Settings.
   *
   * @var array $settings
   */
  private $settings;

  /**
   * Constructor.
   *
   * @param array|NULL $settings
   *  Settings
   */
  public function __construct($settings) {
    $this->settings = $settings ?: [];
  }

  /**
   * Get by key.
   *
   * @param string $key
   *   Key.
   *
   * @return string
   *   Value.
   */
  public function get($key) {
    if (empty($this->settings[$key])) {
      return '';
    };
    return $this->settings[$key];
  }

  /**
   * Get front-end config.
   *
   * @return array
   *   Get front end config array.
   */
  public function getFrontEndConfig() {
    return [
      'account_name' => $this->get('account_name'),
      'customer_site' => $this->get('customer_site'),
      'js_path' => $this->get('js_path'),
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
    if (empty($this->get('account_name')) ||
      empty($this->get('api_url')) ||
      empty($this->get('access_key')) ||
      empty($this->get('secret_key')) ||
      empty($this->get('js_path'))
    ) {
      return FALSE;
    }

    // URLs need to be valid.
    if (!UrlHelper::isValid($this->get('api_url')) || !UrlHelper::isValid($this->get('js_path'))) {
      return FALSE;
    }

    return TRUE;
  }
}
