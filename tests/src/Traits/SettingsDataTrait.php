<?php

namespace Drupal\Tests\acquia_lift\Traits;

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
      'account_id' => 'account_id_1',
      'site_id' => 'site_id_1',
      'content_origin' => 'content_origin_1',
      'public_key' => 'public_key_1',
      'secret_key' => 'secret_key_1',
      'assets_url' => 'assets_url_1',
      'decision_api_url' => 'https://example.com',
      'oauth_url' => 'https://example.com',
    ];
  }

  /**
   * Get a valid identity settings array.
   *
   * @return array
   *   A valid identity settings array.
   */
  private function getValidIdentitySettings() {
    return [
      'capture_identity' => FALSE,
      'identity_parameter' => 'my_identity_parameter',
      'identity_type_parameter' => 'my_identity_type_parameter',
      'default_identity_type' => 'my_default_identity_type',
    ];
  }

  /**
   * Get a valid field mappings settings array.
   *
   * @return array
   *   A valid field mappings settings array.
   */
  private function getValidFieldMappingsSettings() {
    return [
      'content_section' => 'field_country',
      'content_keywords' => 'field_tags',
      'persona' => 'field_people',
    ];
  }

  /**
   * Get a valid thumbnail settings array.
   *
   * @return array
   *   A valid thumbnail settings array.
   */
  private function getValidThumbnailSettings() {
    return [
      'article' => [
        'field' => 'field_media->field_image',
        'style' => 'medium',
      ],
    ];
  }

  /**
   * Get a valid visibility settings array.
   *
   * @return array
   *   A valid visibility settings array.
   */
  private function getValidVisibilitySettings() {
    return [
      'path_patterns' => "/admin\n/admin/*\n/batch\n/node/add*\n/node/*/*\n/user/*/*\n/block/*",
    ];
  }
}
