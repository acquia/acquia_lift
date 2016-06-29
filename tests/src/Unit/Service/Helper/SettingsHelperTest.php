<?php

namespace Drupal\Tests\acquia_lift\Service\Helper;

use Drupal\Tests\UnitTestCase;
use Drupal\acquia_lift\Service\Helper\SettingsHelper;
use Drupal\Tests\acquia_lift\Unit\Traits\SettingsDataTrait;

require_once(__DIR__ . '/../../Traits/SettingsDataTrait.php');

/**
 * SettingsHelper Test.
 *
 * @coversDefaultClass Drupal\acquia_lift\Service\Helper\SettingsHelper
 * @group acquia_lift
 */
class SettingsHelperTest extends UnitTestCase {

  use SettingsDataTrait;

  /**
   * Tests the getFrontEndCredentialSettings() method.
   *
   * @covers ::getFrontEndCredentialSettings
   *
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
    $data['valid data 2'][0]['customer_site'] = 'customer_site_2';
    $data['valid data 2'][1]['customer_site'] = 'customer_site_2';
    $data['valid data 2'][0]['js_path'] = 'js_path_2';
    $data['valid data 2'][1]['js_path'] = 'js_path_2';

    return $data;
  }

  /**
   * Tests the getFrontEndCredentialSettings() method's Exception.
   *
   * @covers ::getFrontEndCredentialSettings
   *
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
    $data['missing api_url'] = [$valid_settings];
    $data['not set customer_site'] = [$valid_settings];

    $data['missing account_name'][0]['account_name'] = '';
    $data['missing api_url'][0]['js_path'] = '';
    unset($data['not set customer_site'][0]['customer_site']);

    return $data;
  }

  /**
   * Tests the isInvalidCredential() method.
   *
   * @covers ::isInvalidCredential
   *
   * @param array $full_settings
   * @param boolean $expected
   *
   * @dataProvider providerTestIsInvalidCredential
   */
  public function testIsInvalidCredential($full_settings, $expected) {
    $result = SettingsHelper::isInvalidCredential($full_settings);
    $this->assertEquals($expected, $result);
  }

  /**
   * Data provider for testIsInvalidCredential().
   */
  public function providerTestIsInvalidCredential() {
    $data = [];
    $valid_settings = $this->getValidCredentialSettings();

    $data['valid data 1'] = [$valid_settings, FALSE];
    $data['valid data 2'] = [$valid_settings, FALSE];
    $data['missing account_name'] = [$valid_settings, TRUE];
    $data['invalid api_url URL'] = [$valid_settings, TRUE];
    $data['invalid js_path URL'] = [$valid_settings, TRUE];

    $data['valid data 2'][0]['account_name'] = 'account_name_2';
    $data['valid data 2'][0]['customer_site'] = 'customer_site_2';
    $data['valid data 2'][0]['js_path'] = 'js_path_2';
    $data['missing account_name'][0]['account_name'] = '';
    $data['invalid api_url URL'][0]['api_url'] = '\\\\////\\\\////';
    $data['invalid js_path URL'][0]['js_path'] = 'invalid js path';

    return $data;
  }
}
