<?php

/**
 * @file
 * Contains \Drupal\acquia_lift\Tests\SettingsTest.
 */

namespace Drupal\acquia_lift\Tests;

use Drupal\simpletest\WebTestBase;

/**
 * Test Settings.
 *
 * @group Acquia Lift
 */
class SettingsTest extends WebTestBase {
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

  function testConfigurationLinks() {
    // Check if Configure link is available on 'Extend' page.
    // Requires 'administer modules' permission.
    $this->drupalGet('admin/modules');
    $this->assertRaw('admin/config/content/acquia_lift', '[testConfigurationLinks]: Configure link from Extend page to Acquia Lift Settings page exists.');

    // Check if Configure link is available on 'Status Reports' page. NOTE: Link is only shown without a configured Acquia Lift credential.
    // Requires 'administer site configuration' permission.
    $this->drupalGet('admin/reports/status');
    $this->assertRaw('admin/config/content/acquia_lift', '[testConfigurationLinks]: Configure link from Status Reports page to Acquia Lift Settings page exists.');
  }

  function testAdminSettingsForm() {
    // Check for setting page's presence.
    $this->drupalGet('admin/config/content/acquia_lift');
    $this->assertRaw(t('Acquia Lift settings'), '[testAdminSettingsForm]: Settings page displayed.');
  }
}
