<?php

/**
 * @file
 * Contains \Drupal\acquia_lift\Tests\SettingsTest.
 */

namespace Drupal\acquia_lift\Tests;

use Drupal\simpletest\WebTestBase;
use Drupal\acquia_lift\Tests\Traits\SettingsDataTrait;

require_once(__DIR__ . '/SettingsDataTrait.php');

/**
 * Test Settings.
 *
 * @group Acquia Lift
 */
class SettingsTest extends WebTestBase {

  use SettingsDataTrait;

  /**
   * Modules to enable.
   *
   * @var array
   */
  public static $modules = ['node', 'acquia_lift'];

  /**
   * {@inheritdoc}
   */
  protected function setUp() {
    parent::setUp();

    $permissions = [
      'access administration pages',
      'administer acquia lift',
      'administer modules',
      'administer site configuration',
    ];

    // User to set up google_analytics.
    $this->admin_user = $this->drupalCreateUser($permissions);
    $this->drupalLogin($this->admin_user);
  }

  public function testConfigurationLinks() {
    // Check if Configure link is available on 'Extend' page.
    // Requires 'administer modules' permission.
    $this->drupalGet('admin/modules');
    $this->assertRaw('admin/config/content/acquia_lift', '[testConfigurationLinks]: Configure link from Extend page to Acquia Lift Settings page exists.');

    // Check if Configure link is available on 'Status Reports' page. NOTE: Link is only shown without a configured Acquia Lift credential.
    // Requires 'administer site configuration' permission.
    $this->drupalGet('admin/reports/status');
    $this->assertRaw('admin/config/content/acquia_lift', '[testConfigurationLinks]: Configure link from Status Reports page to Acquia Lift Settings page exists.');
  }

  public function testAdminSettingsForm() {
    // Check for setting page's presence.
    $this->drupalGet('admin/config/content/acquia_lift');
    $this->assertRaw(t('Acquia Lift settings'), '[testAdminSettingsForm]: Settings page displayed.');

    $credential_settings = $this->getValidCredentialSettings();
    $identity_settings = $this->getValidIdentitySettings();
    $field_mappings_settings = $this->getValidFieldMappingsSettings();
    $visibility_settings = $this->getValidVisibilitySettings();

    $edit =[];
    $edit += $this->convertToPostFormSettings($credential_settings, 'credential');
    $edit += $this->convertToPostFormSettings($identity_settings, 'identity');
    //$edit += $this->convertToPostFormSettings($field_mappings_settings, 'field_mappings');
    $edit += $this->convertToPostFormSettings($visibility_settings, 'visibility');
    $edit_settings_count = count($edit);

    $this->drupalPostForm('admin/config/content/acquia_lift', $edit, t('Save configuration'));

    $this->assertText(t('The configuration options have been saved.'));

    // All fields should be correctly shown, except the secret key, because it is not displayed back.
    $actual_secret_key = $this->config('acquia_lift.settings')->get('credential.secret_key');
    $this->assertEqual($edit['credential[secret_key]'], $actual_secret_key, 'Credential\'s secret key was saved into DB.');
    $edit['credential[secret_key]'] = '';
    $this->assertText('Only necessary if updating');

    foreach ($edit as $name => $value) {
      $this->assertFieldByName($name, $value, format_string('"@name" has correct value.', array('@name' => $name)));
    }
    $this->assertEqual(10, $edit_settings_count,  'Counting the exact numbers of form fields were all asserted.');
  }
}
