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
   * @covers ::getFrontEndCredentialSettings
   * @param array $full_settings
   * @param array $expected_front_end_settings
   *
   * @dataProvider providerTestGetFrontEndCredentialSettings
   */
  public function testGetFrontEndCredentialSettings($full_settings, $expected_front_end_settings) {
    $result_front_end_settings = SettingsHelper::getFrontEndCredentialSettings($full_settings);
    $this->assertEquals($expected_front_end_settings, $result_front_end_settings);
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

  /**
   * Tests the getFrontEndCredentialSettings() method's Exception.
   *
   * @covers ::getFrontEndCredentialSettings
   * @param array $incomplete_settings
   *
   * @dataProvider providerTestGetFrontEndCredentialSettingsException
   * @expectedException \Drupal\acquia_lift\Exception\MissingSettingsException
   * @expectedExceptionMessage Cannot generate front-end credential settings because some settings are missing.
   */
  public function testGetFrontEndCredentialSettingsException($incomplete_settings) {
    SettingsHelper::getFrontEndCredentialSettings($incomplete_settings);
  }

  /**
   * Data provider for testGetFrontEndCredentialSettingsException().
   */
  public function providerTestGetFrontEndCredentialSettingsException() {
    $data = [];
    $valid_settings = $this->getValidCredentialSettings();

    $data['missing account_name'] = [$valid_settings];
    $data['missing account_name'][0]['account_name'] = '';
    $data['missing api_url'] = [$valid_settings];
    $data['missing api_url'][0]['js_path'] = '';
    $data['not set customer_site'] = [$valid_settings];
    unset($data['not set customer_site'][0]['customer_site']);

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
