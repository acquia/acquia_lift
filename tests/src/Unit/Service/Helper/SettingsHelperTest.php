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
    $data['invalid js_path URL'] = [$valid_settings, TRUE];
    $data['invalid api_url URL'] = [$valid_settings, TRUE];

    $data['valid data 2'][0]['account_name'] = 'account_name_2';
    $data['valid data 2'][0]['customer_site'] = 'customer_site_2';
    $data['valid data 2'][0]['js_path'] = 'js_path_2';
    $data['missing account_name'][0]['account_name'] = '';
    $data['invalid js_path URL'][0]['js_path'] = 'invalid js path';
    $data['invalid api_url URL'][0]['api_url'] = '\\\\////\\\\////';

    return $data;
  }
}
