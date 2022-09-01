<?php

namespace Drupal\Tests\acquia_lift\Unit\Traits;

/**
 * Settings Data Trait.
 */
trait SettingsDataTrait {

  /**
   * Set valid settings.
   *
   * @return array
   *   A valid settings array.
   */
  private function setValidSettings() {
    $settings = $this->config('acquia_lift.settings');
    $settings->set('credential', $this->getValidCredentialSettings());
    $settings->set('identity', $this->getValidIdentitySettings());
    $settings->set('field_mappings', $this->getValidFieldMappingsSettings());
    $settings->set('udf_person_mappings', $this->getValidUdfPersonMappingsSettings());
    $settings->set('udf_event_mappings', $this->getValidUdfEventMappingsSettings());
    $settings->set('udf_touch_mappings', $this->getValidUdfTouchMappingsSettings());
    $settings->set('visibility', $this->getValidVisibilitySettings());
    $settings->set('advanced', $this->getValidAdvancedSettings());
    $settings->save();

    return $settings->getRawData();
  }

  /**
   * Get a valid credential settings array.
   *
   * @return array
   *   A valid credential settings array.
   */
  private function getValidCredentialSettings() {
    return [
      'account_id' => 'AccountId1',
      'site_id' => 'SiteId1',
      'assets_url' => 'AssetsUrl1',
      'decision_api_url' => 'decision_api_url_1',
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
      // 'capture_identity' => FALSE,
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
   * Get a valid udf person mappings settings array.
   *
   * @return array
   *   A valid UDF Person mappings settings array.
   */
  private function getValidUdfPersonMappingsSettings() {
    return [
      'person_udf1' => [
        'id' => 'person_udf1',
        'value' => 'field_tags',
        'type' => 'taxonomy',
      ],
      'person_udf2' => [
        'id' => 'person_udf2',
        'value' => 'field_people',
        'type' => 'taxonomy',
      ],
    ];
  }

  /**
   * Get a valid udf person mappings settings array.
   *
   * @return array
   *   A valid UDF Person mappings settings array.
   */
  private function getValidUdfPersonMappingsFormData() {
    return [
      'person_udf1' => 'field_tags',
      'person_udf2' => 'field_people',
    ];
  }

  /**
   * Get a valid udf touch mappings settings array.
   *
   * @return array
   *   A valid UDF Touch mappings settings array.
   */
  private function getValidUdfTouchMappingsSettings() {
    return [
      'touch_udf1' => [
        'id' => 'touch_udf1',
        'value' => 'field_country',
        'type' => 'taxonomy',
      ],
      'touch_udf2' => [
        'id' => 'touch_udf2',
        'value' => 'field_people',
        'type' => 'taxonomy',
      ],
    ];
  }

  /**
   * Get a valid udf touch mappings settings array.
   *
   * @return array
   *   A valid UDF Touch mappings settings array.
   */
  private function getValidUdfTouchMappingsFormData() {
    return [
      'touch_udf1' => 'field_country',
      'touch_udf2' => 'field_people',
    ];
  }

  /**
   * Get a valid udf event mappings settings array.
   *
   * @return array
   *   A valid UDF Event mappings settings array.
   */
  private function getValidUdfEventMappingsSettings() {
    return [
      'event_udf1' => [
        'id' => 'event_udf1',
        'value' => 'field_country',
        'type' => 'taxonomy',
      ],
      'event_udf2' => [
        'id' => 'event_udf2',
        'value' => 'field_tags',
        'type' => 'taxonomy',
      ],
    ];
  }

  /**
   * Get a valid udf event mappings settings array.
   *
   * @return array
   *   A valid UDF Event mappings settings array.
   */
  private function getValidUdfEventMappingsFormData() {
    return [
      'event_udf1' => 'field_country',
      'event_udf2' => 'field_tags',
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
      'path_patterns' => "/admin\n/admin/*\n/batch\n/node/add*\n/node/*/*\n/user/*\n/block/*",
    ];
  }

  /**
   * Get a valid advanced settings array.
   *
   * @param string $content_origins
   *   Content Origin.
   *
   * @return array
   *   A valid advanced configuration settings array.
   */
  private function getValidAdvancedSettings($content_origins = '') {
    return [
      'bootstrap_mode' => 'manual',
      'content_replacement_mode' => 'customized',
      'cdf_version' => 2,
      'content_origins' => $content_origins,
    ];
  }

  /**
   * Convert to post form settings.
   *
   * @param array $settings
   *   Form Settings.
   * @param string $prefix
   *   Form Settings Prefix.
   *
   * @return array
   *   A valid front end credential settings array.
   */
  private function convertToPostFormSettings(array $settings, $prefix) {
    $post_form_settings = [];
    foreach ($settings as $setting_name => $setting_value) {
      $post_form_settings[$prefix . '[' . $setting_name . ']'] = $setting_value;
    }
    return $post_form_settings;
  }

}
