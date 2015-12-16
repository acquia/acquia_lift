<?php

/**
 * @file
 * Contains \Drupal\acquia_lift\Service\Helper\SettingsHelper.
 */

namespace Drupal\acquia_lift\Service\Helper;

use Drupal\Component\Utility\UrlHelper;

/**
 * Defines the Settings Helper class.
 */
class SettingsHelper {
  /**
   * Get front-end credential config.
   *
   * @param array $credential_settings
   *   Credential settings array.
   * @return array
   *   Get front end config array.
   */
  static public function getFrontEndConfig($credential_settings) {
    if (SELF::isInvalidCredential($credential_settings)) {
      return [];
    }
    return [
      'account_name' => $credential_settings['account_name'],
      'customer_site' => $credential_settings['customer_site'],
      'js_path' => $credential_settings['js_path'],
    ];
  }

  /**
   * Is an invalid credential.
   *
   * @param array
   *   Credential settings array.
   * @return boolean
   *   True if is an invalid credential.
   */
  static public function isInvalidCredential($credential_settings) {
    // Required credential need to be filled.
    if (empty($credential_settings['account_name']) ||
      empty($credential_settings['api_url']) ||
      empty($credential_settings['access_key']) ||
      empty($credential_settings['secret_key']) ||
      empty($credential_settings['js_path'])
    ) {
      return TRUE;
    }

    // URLs need to be valid.
    if (!UrlHelper::isValid($credential_settings['api_url']) ||
      !UrlHelper::isValid($credential_settings['js_path'])) {
      return TRUE;
    }

    return FALSE;
  }
}
