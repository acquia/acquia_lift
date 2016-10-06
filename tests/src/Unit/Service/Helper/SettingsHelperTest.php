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
   * Tests the isInvalidCredentialAccountId() method.
   *
   * @covers ::isInvalidCredentialAccountId
   *
   * @param string $setting
   * @param boolean $expected
   *
   * @dataProvider providerTestIsInvalidCredentialAccountId
   */
  public function testIsInvalidCredentialAccountId($setting, $expected) {
    $result = SettingsHelper::isInvalidCredentialAccountId($setting);
    $this->assertEquals($expected, $result);
  }

  /**
   * Data provider for testIsInvalidCredentialAccountId().
   */
  public function providerTestIsInvalidCredentialAccountId() {
    $data = [];

    $data['invalid null'] = [NULL, TRUE];
    $data['invalid empty'] = ['', TRUE];
    $data['invalid start with number'] = ['1', TRUE];
    $data['invalid has "~" sign'] = ['a~', TRUE];
    $data['valid has "_" sign'] = ['a_', FALSE];
    $data['valid start with alphabetic then alphanumeric'] = ['a123', FALSE];

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
    $data['missing account_id'] = [$valid_settings, TRUE];
    $data['invalid assets_url URL'] = [$valid_settings, TRUE];
    $data['invalid decision_api_url URL'] = [$valid_settings, TRUE];

    $data['valid data 2'][0]['account_id'] = 'accountId2';
    $data['valid data 2'][0]['site_id'] = '2222';
    $data['valid data 2'][0]['assets_url'] = 'assets_url_2';
    $data['missing account_id'][0]['account_id'] = '';
    $data['invalid assets_url URL'][0]['assets_url'] = 'invalid assets URL';
    $data['invalid decision_api_url URL'][0]['decision_api_url'] = '\\\\////\\\\////';

    return $data;
  }

  /**
   * Tests the isValidContentReplacementMode() method.
   *
   * @covers ::isValidContentReplacementMode
   *
   * @param string $test_value
   * @param boolean $expected
   *
   * @dataProvider providerTestIsValidContentReplacementMode
   */
  public function testIsValidContentReplacementMode($test_value, $expected) {
    $result = SettingsHelper::isValidContentReplacementMode($test_value);
    $this->assertEquals($expected, $result);
  }

  /**
   * Data provider for testIsValidContentReplacementMode().
   */
  public function providerTestIsValidContentReplacementMode() {
    $data = [];

    $data['valid trusted'] = ['trusted', TRUE];
    $data['valid untrusted'] = ['untrusted', TRUE];
    $data['invalid null'] = [NULL, FALSE];
    $data['invalid value'] = ['another', FALSE];

    return $data;
  }
}
