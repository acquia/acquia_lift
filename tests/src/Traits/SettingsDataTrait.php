<?php

/**
 * @file
 * Contains \Drupal\acquia_lift\Tests\Traits\SettingsDataTrait.
 */

namespace Drupal\acquia_lift\Tests\Traits;

/**
 * Settings Data Trait.
 */
trait SettingsDataTrait {
  /**
   * Get a valid credential settings array.
   *
   * @return array
   *   A valid credential settings array.
   */
  private function getValidCredentialSettings() {
    return [
      'account_name' => 'account_name_1',
      'customer_site' => 'customer_site_1',
      'api_url' => 'api_url_1',
      'access_key' => 'access_key_1',
      'secret_key' => 'secret_key_1',
      'js_path' => 'js_path_1',
    ];
  }

  /**
   * Get a valid front end credential settings.
   *
   * @return array
   *   A valid front end credential settings array.
   */
  private function getValidFrontEndCredentialSettings() {
    return [
      'account_name' => 'account_name_1',
      'customer_site' => 'customer_site_1',
      'js_path' => 'js_path_1',
    ];
  }
}
