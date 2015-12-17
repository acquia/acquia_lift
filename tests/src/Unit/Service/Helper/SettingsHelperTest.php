<?php

/**
 * @file
 * Contains \Drupal\acquia_lift\Tests\Service\Helper\SettingsHelperTest.
 */

namespace Drupal\acquia_lift\Tests\Service\Helper;

use Drupal\Tests\UnitTestCase;
use Drupal\acquia_lift\Service\Helper\SettingsHelper;

/**
 * SettingsHelper Test.
 *
 * @coversDefaultClass Drupal\acquia_lift\Service\Helper\SettingsHelper
 * @group acquia_lift
 */
class SettingsHelperTest extends UnitTestCase {
  /**
   * Tests the getFrontEndCredentialSettings() method.
   *
   * @param array $full_settings
   * @param array $expected_front_end_settings
   *
   * @covers ::getFrontEndCredentialSettings
   * @dataProvider providerTestGetFrontEndCredentialSettings
   */
  public function testGetFrontEndCredentialSettings($full_settings, $expected_front_end_settings) {
    $result_front_end_settings = SettingsHelper::getFrontEndCredentialSettings($full_settings);
    $this->assertSame($expected_front_end_settings, $result_front_end_settings);
  }

  /**
   * Data provider for testGetFrontEndCredentialSettings().
   */
  public function providerTestGetFrontEndCredentialSettings() {
    $data = [];
    $valid_settings = $this->getValidCredentialSettings();
    $valid_front_end_settings = $this->getValidFrontEndCredentialSettings();

    $data['valid data 1'] = [$valid_settings, $valid_front_end_settings];

    $data['valid data 2'] = [$valid_settings, $valid_front_end_settings];
    $data['valid data 2'][0]['account_name'] = 'account_name_2';
    $data['valid data 2'][1]['account_name'] = 'account_name_2';
    $data['valid data 2'][0]['customer_site'] = '';
    $data['valid data 2'][1]['customer_site'] = '';
    $data['valid data 2'][0]['js_path'] = 'js_path_2';
    $data['valid data 2'][1]['js_path'] = 'js_path_2';

    return $data;
  }

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

  private function getValidFrontEndCredentialSettings() {
    return [
      'account_name' => 'account_name_1',
      'customer_site' => 'customer_site_1',
      'js_path' => 'js_path_1',
    ];
  }
}
