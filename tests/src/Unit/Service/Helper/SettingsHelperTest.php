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
    $data['missing account_id'] = [$valid_settings, TRUE];
    $data['invalid assets_url URL'] = [$valid_settings, TRUE];
    $data['invalid decision_api_url URL'] = [$valid_settings, TRUE];

    $data['valid data 2'][0]['account_id'] = 'account_id_2';
    $data['valid data 2'][0]['site_id'] = 'site_id_2';
    $data['valid data 2'][0]['assets_url'] = 'assets_url_2';
    $data['missing account_id'][0]['account_id'] = '';
    $data['invalid assets_url URL'][0]['assets_url'] = 'invalid assets URL';
    $data['invalid decision_api_url URL'][0]['decision_api_url'] = '\\\\////\\\\////';

    return $data;
  }
}
