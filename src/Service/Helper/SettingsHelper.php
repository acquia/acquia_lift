<?php

namespace Drupal\acquia_lift\Service\Helper;

use Drupal\Component\Utility\UrlHelper;
use Drupal\acquia_lift\Exception\MissingSettingsException;

/**
 * Defines the Settings Helper class.
 */
class SettingsHelper {
  /**
   * Default identity type's default value.
   */
  const DEFAULT_IDENTITY_TYPE_DEFAULT = 'email';

  /**
   * Get front-end credential settings.
   *
   * @param array $credential_settings
   *   Credential settings array.
   * @return array
   *   Get front end settings array.
   * @throws \Drupal\acquia_lift\Exception\MissingSettingsException
   */
  static public function getFrontEndCredentialSettings($credential_settings) {
    if (empty($credential_settings['account_name']) ||
      empty($credential_settings['customer_site']) ||
      empty($credential_settings['js_path'])
    ) {
      throw new MissingSettingsException('Cannot generate front-end credential settings because some settings are missing.');
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
      empty($credential_settings['customer_site']) ||
      empty($credential_settings['js_path'])
    ) {
      return TRUE;
    }

    // URLs need to be valid.
    if (!UrlHelper::isValid($credential_settings['js_path']) ||
      !UrlHelper::isValid($credential_settings['api_url']) ||
      !UrlHelper::isValid($credential_settings['oauth_url'])
    ) {
      return TRUE;
    }

    return FALSE;
  }
}
