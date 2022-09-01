<?php

namespace Drupal\Tests\acquia_lift\Unit\Service\Helper;

use Drupal\acquia_lift\Service\Helper\SettingsHelper;
use Drupal\Core\DependencyInjection\ContainerBuilder;
use Drupal\Core\Http\ClientFactory;
use Drupal\Tests\acquia_lift\Unit\Traits\SettingsDataTrait;
use Drupal\Tests\UnitTestCase;
use GuzzleHttp\Client;
use Prophecy\Argument;
use Prophecy\PhpUnit\ProphecyTrait;
use Psr\Http\Message\ResponseInterface;

/**
 * SettingsHelper Test.
 *
 * @coversDefaultClass \Drupal\acquia_lift\Service\Helper\SettingsHelper
 * @group acquia_lift
 */
class SettingsHelperTest extends UnitTestCase {

  use ProphecyTrait;
  use SettingsDataTrait;

  /**
   * Tests the isInvalidCredentialAccountId() method.
   *
   * @param string $setting
   *   Test Value.
   * @param bool $expected
   *   Expected Value.
   *
   * @covers ::isInvalidCredentialAccountId
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
    $data['invalid start with number'] = ['1a', TRUE];
    $data['invalid has "~" sign'] = ['a~', TRUE];
    $data['valid has "_" sign'] = ['a_', FALSE];
    $data['valid start with alphabetic then alphanumeric'] = ['a123', FALSE];

    return $data;
  }

  /**
   * Tests the isInvalidCredentialSiteId() method.
   *
   * @param string $setting
   *   Test Value.
   * @param bool $expected
   *   Expected Value.
   *
   * @covers ::isInvalidCredentialSiteId
   *
   * @dataProvider providerTestIsInvalidCredentialSiteId
   */
  public function testIsInvalidCredentialSiteId($setting, $expected) {
    $result = SettingsHelper::isInvalidCredentialSiteId($setting);
    $this->assertEquals($expected, $result);
  }

  /**
   * Data provider for testIsInvalidCredentialSiteId().
   */
  public function providerTestIsInvalidCredentialSiteId() {
    $data = [];

    $data['invalid null'] = [NULL, TRUE];
    $data['invalid empty'] = ['', TRUE];
    $data['valid has "~" sign'] = ['a~', FALSE];
    $data['valid alphanumeric 1'] = ['a123', FALSE];
    $data['valid alphanumeric 2'] = ['3Ab', FALSE];

    return $data;
  }

  /**
   * Tests the isInvalidCredentialAssetsUrl() method.
   *
   * @param string $setting
   *   Test Value.
   * @param bool $expected
   *   Expected Value.
   *
   * @covers ::isInvalidCredentialAssetsUrl
   *
   * @dataProvider providerTestIsInvalidCredentialAssetsUrl
   */
  public function testIsInvalidCredentialAssetsUrl($setting, $expected) {
    $result = SettingsHelper::isInvalidCredentialAssetsUrl($setting);
    $this->assertEquals($expected, $result);
  }

  /**
   * Data provider for testIsInvalidCredentialAssetsUrl().
   */
  public function providerTestIsInvalidCredentialAssetsUrl() {
    $data = [];

    $data['invalid null'] = [NULL, TRUE];
    $data['invalid empty'] = ['', TRUE];
    $data['invalid has non-ascii characters'] = ['不合法', TRUE];
    $data['valid url 1'] = ['acquia', FALSE];
    $data['valid url 2'] = ['acquia.com', FALSE];

    return $data;
  }

  /**
   * Tests the isInvalidCredentialDecisionApiUrl() method.
   *
   * @param string $setting
   *   Test Value.
   * @param bool $expected
   *   Expected Value.
   *
   * @covers ::isInvalidCredentialDecisionApiUrl
   *
   * @dataProvider providerTestIsInvalidCredentialDecisionApiUrl
   */
  public function testIsInvalidCredentialDecisionApiUrl($setting, $expected) {
    $result = SettingsHelper::isInvalidCredentialDecisionApiUrl($setting);
    $this->assertEquals($expected, $result);
  }

  /**
   * Data provider for testIsInvalidCredentialDecisionApiUrl().
   */
  public function providerTestIsInvalidCredentialDecisionApiUrl() {
    $data = [];

    $data['invalid has non-ascii characters'] = ['不合法', TRUE];
    $data['valid null'] = [NULL, FALSE];
    $data['valid empty'] = ['', FALSE];
    $data['valid url 1'] = ['acquia', FALSE];
    $data['valid url 2'] = ['acquia.com', FALSE];

    return $data;
  }

  /**
   * Tests the isInvalidCredential() method.
   *
   * @param string $full_settings
   *   Test Value.
   * @param bool $expected
   *   Expected Value.
   *
   * @covers ::isInvalidCredential
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
   * @param string $test_value
   *   Test Value.
   * @param bool $expected
   *   Expected Value.
   *
   * @covers ::isValidContentReplacementMode
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
    $data['valid customized'] = ['customized', TRUE];
    $data['invalid null'] = [NULL, FALSE];
    $data['invalid value'] = ['another', FALSE];

    return $data;
  }

  /**
   * Tests the getUdfLimitsForType() method.
   *
   * @param string $test_value
   *   Test Value.
   * @param bool $expected
   *   Expected Value.
   *
   * @covers ::getUdfLimitsForType
   *
   * @dataProvider providerTestGetUdfLimitsForType
   */
  public function testGetUdfLimitsForType($test_value, $expected) {
    $result = SettingsHelper::getUdfLimitsForType($test_value);
    $this->assertEquals($expected, $result);
  }

  /**
   * Data provider for testGetUdfLimitsForType().
   */
  public function providerTestGetUdfLimitsForType() {
    $data = [];

    $data['person'] = ['person', 50];
    $data['touch'] = ['touch', 20];
    $data['event'] = ['event', 50];

    return $data;
  }

  /**
   * Tests the getUdfLimitsForType() method, expected exception.
   *
   * @covers ::getUdfLimitsForType
   */
  public function testGetUdfLimitsForTypeExpectedException() {
    $this->expectException(\Exception::class);
    $this->expectExceptionCode(0);
    $this->expectExceptionMessage('This UDF Field type is not supported.');
    SettingsHelper::getUdfLimitsForType('non_exist');
  }

  /**
   * Tests the pingUri() method.
   *
   * @param string $test_value
   *   Test Value.
   * @param bool $expected
   *   Expected Value.
   *
   * @covers ::pingUri
   *
   * @dataProvider providerTestPingUri
   */
  public function testPingUri($test_value, $expected) {
    $response = $this->prophesize(ResponseInterface::class);
    $response->getStatusCode()->willReturn($expected['statusCode']);
    $response->getReasonPhrase()->willReturn($expected['reasonPhrase']);
    $client = $this->prophesize(Client::class);
    $client->get($test_value[1], ['http_errors' => FALSE])->willReturn($response->reveal());
    $clientFactory = $this->prophesize(ClientFactory::class);
    $clientFactory->fromOptions(Argument::any())->willReturn($client->reveal());
    $container = $this->prophesize(ContainerBuilder::class);
    $container->get('http_client_factory')->willReturn($clientFactory->reveal());
    \Drupal::setContainer($container->reveal());

    $result = SettingsHelper::pingUri($test_value[0], $test_value[1]);
    $this->assertEquals($expected, $result);
  }

  /**
   * Data provider for testPingUri().
   */
  public function providerTestPingUri() {
    $data = [];

    $data['invalid uri'] = [
      ['uri_1', ''],
      ['statusCode' => '', 'reasonPhrase' => ''],
    ];
    $data['valid uri 1'] = [
      ['uri_1', 'path_1'],
      ['statusCode' => 123, 'reasonPhrase' => 'a:1:{s:8:"base_uri";s:5:"uri_1";} path_1 a:1:{s:11:"http_errors";b:0;}'],
    ];
    $data['valid uri 2'] = [
      ['uri_2', 'path_2'],
      ['statusCode' => 123, 'reasonPhrase' => 'a:1:{s:8:"base_uri";s:5:"uri_2";} path_2 a:1:{s:11:"http_errors";b:0;}'],
    ];

    return $data;
  }

}
