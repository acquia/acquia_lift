<?php

namespace Drupal\Tests\acquia_lift\Service\Helper;

use Drupal\Tests\UnitTestCase;
use Drupal\acquia_lift\Service\Helper\HelpMessageHelper;
use Drupal\Tests\acquia_lift\Unit\Traits\SettingsDataTrait;

require_once(__DIR__ . '/../../Traits/SettingsDataTrait.php');
require_once(__DIR__ . '/../../Polyfill/Drupal.php');

/**
 * HelpMessageHelper Test.
 *
 * @coversDefaultClass Drupal\acquia_lift\Service\Helper\HelpMessageHelper
 * @group acquia_lift
 */
class HelpMessageHelperTest extends UnitTestCase {

  use SettingsDataTrait;

  /**
   * @var \Drupal\Core\Config\ConfigFactoryInterface|\PHPUnit_Framework_MockObject_MockObject
   */
  private $configFactory;

  /**
   * @var \Drupal\Core\Utility\LinkGeneratorInterface|\PHPUnit_Framework_MockObject_MockObject
   */
  private $linkGenerator;

  /**
   * @var \Drupal\Core\Config\ImmutableConfig|\PHPUnit_Framework_MockObject_MockObject
   */
  private $settings;

  /**
   * {@inheritdoc}
   */
  public function setUp() {
    parent::setUp();

    $this->configFactory = $this->getMock('Drupal\Core\Config\ConfigFactoryInterface');
    $this->linkGenerator = $this->getMock('Drupal\Core\Utility\LinkGeneratorInterface');
    $this->settings = $this->getMockBuilder('Drupal\Core\Config\ImmutableConfig')
      ->disableOriginalConstructor()
      ->getMock();

    $this->configFactory->expects($this->once())
      ->method('get')
      ->with('acquia_lift.settings')
      ->willReturn($this->settings);

    $this->linkGenerator->expects($this->at(0))
      ->method('generate')
      ->with('Documentation')
      ->willReturn('a_documentation_link');
  }

  /**
   * Tests the getMessage() method - AdminSettingsForm, full settings.
   *
   * @covers ::getMessage
   *
   * @param string $route_name
   *
   * @dataProvider providerRouteNames
   */
  public function testGetMessageAdminSettingsFormFullSettings($route_name) {
    $full_settings = $this->getValidCredentialSettings();

    $this->settings->expects($this->once())
      ->method('get')
      ->with('credential')
      ->willReturn($full_settings);

    $help_message_helper = new HelpMessageHelper($this->configFactory, $this->linkGenerator);
    $message = $help_message_helper->getMessage($route_name);
    $this->assertEquals('You can find more info in a_documentation_link.', $message);
  }

  /**
   * Tests the getMessage() method - AdminSettingsForm, no API URL setting.
   *
   * @covers ::getMessage
   *
   * @param string $route_name
   *
   * @dataProvider providerRouteNames
   */
  public function testGetMessageAdminSettingsFormNoApiUrl($route_name) {
    $missing_api_url_settings = $this->getValidCredentialSettings();
    unset($missing_api_url_settings['api_url']);

    $this->settings->expects($this->once())
      ->method('get')
      ->with('credential')
      ->willReturn($missing_api_url_settings);

    $help_message_helper = new HelpMessageHelper($this->configFactory, $this->linkGenerator);
    $message = $help_message_helper->getMessage($route_name);
    $this->assertEquals('You can find more info in a_documentation_link.', $message);
  }

  /**
   * Data provider to produce route names.
   */
  public function providerRouteNames() {
    $data = [];
    $data['help page'] = ['help.page.acquia_lift'];
    $data['admin settings form'] = ['acquia_lift.admin_settings_form'];

    return $data;
  }
}
