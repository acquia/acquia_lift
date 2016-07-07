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
   * Is an invalid credential.
   *
   * @param array
   *   Credential settings array.
   * @return boolean
   *   True if is an invalid credential.
   */
  public static function isInvalidCredential($credential_settings) {
    // Required credential need to be filled.
    if (empty($credential_settings['account_id']) ||
      empty($credential_settings['site_id']) ||
      empty($credential_settings['assets_url'])
    ) {
      return TRUE;
    }

    // URLs need to be valid.
    if (!UrlHelper::isValid($credential_settings['assets_url']) ||
      !empty($credential_settings['decision_api_url']) && !UrlHelper::isValid($credential_settings['decision_api_url']) ||
      !empty($credential_settings['oauth_url']) && !UrlHelper::isValid($credential_settings['oauth_url'])
    ) {
      return TRUE;
    }

    return FALSE;
  }
}
