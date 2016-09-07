<?php
namespace Drupal\Tests\acquia_lift\Unit\Lift;

use Drupal\acquia_lift\Lift\APILoader;
use Drupal\Tests\acquia_lift\Traits\SettingsDataTrait;
use Drupal\Tests\UnitTestCase;

require_once(__DIR__ . '/../../Traits/SettingsDataTrait.php');

/**
* PageContextTest Test.
*
* @coversDefaultClass Drupal\acquia_lift\Service\Context\PageContext
* @group acquia_lift
*/
class APILoaderTest extends UnitTestCase {
  use SettingsDataTrait;

  /**
   * @var \Drupal\Core\Config\ConfigFactoryInterface|\PHPUnit_Framework_MockObject_MockObject
   */
  private $configFactory;

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
    $this->settings = $this->getMockBuilder('Drupal\Core\Config\ImmutableConfig')
      ->disableOriginalConstructor()
      ->getMock();

    $this->configFactory->expects($this->once())
      ->method('get')
      ->with('acquia_lift.settings')
      ->willReturn($this->settings);
  }

  /**
   * Tests the populateHtmlHead() method, credential configuration.
   *
   * @covers ::populateHtmlHead
   */
  public function testCreateAPILoaderAndGetLiftClient() {
    $this->settings->expects($this->at(0))
      ->method('get')
      ->with('credential')
      ->willReturn($this->getValidCredentialSettings());
    $apiLoader = new APILoader($this->configFactory);
    $apiLoader->initialize();
    $apiLoader->getLiftClient();
  }

  /**
   * Test that without an account id, the APILoader returns the correct error.
   *
   * @expectedException        \Drupal\acquia_lift\Exception\APILoaderException
   * @expectedExceptionMessage Account ID not found. Please verify your settings.
   */
  public function testFailOnMissingAccountId() {
    $credentials = $this->getValidCredentialSettings();
    unset($credentials['account_id']);
    $this->settings->expects($this->at(0))
      ->method('get')
      ->with('credential')
      ->willReturn($credentials);

    $apiLoader = new APILoader($this->configFactory);
    $apiLoader->initialize();
  }

  /**
   * Test that without a site id, the APILoader returns the correct error.
   *
   * @expectedException        \Drupal\acquia_lift\Exception\APILoaderException
   * @expectedExceptionMessage Site ID not found. Please verify your settings.
   */
  public function testFailOnMissingSiteId() {
    $credentials = $this->getValidCredentialSettings();
    unset($credentials['site_id']);
    $this->settings->expects($this->at(0))
      ->method('get')
      ->with('credential')
      ->willReturn($credentials);

    $apiLoader = new APILoader($this->configFactory);
    $apiLoader->initialize();
  }

  /**
   * Test that without a public key, the APILoader returns the correct error.
   *
   * @expectedException        \Drupal\acquia_lift\Exception\APILoaderException
   * @expectedExceptionMessage Public Key not found. Please verify your settings.
   */
  public function testFailOnMissingPublicKey() {
    $credentials = $this->getValidCredentialSettings();
    unset($credentials['public_key']);
    $this->settings->expects($this->at(0))
      ->method('get')
      ->with('credential')
      ->willReturn($credentials);

    $apiLoader = new APILoader($this->configFactory);
    $apiLoader->initialize();
  }

  /**
   * Test that without a secret key, the APILoader returns the correct error.
   *
   * @expectedException        \Drupal\acquia_lift\Exception\APILoaderException
   * @expectedExceptionMessage Secret Key not found. Please verify your settings.
   */
  public function testFailOnMissingSecretKey() {
    $credentials = $this->getValidCredentialSettings();
    unset($credentials['secret_key']);
    $this->settings->expects($this->at(0))
      ->method('get')
      ->with('credential')
      ->willReturn($credentials);

    $apiLoader = new APILoader($this->configFactory);
    $apiLoader->initialize();
  }

  /**
   * Test that without a decision API URL, the APILoader returns the correct error.
   *
   * @expectedException        \Drupal\acquia_lift\Exception\APILoaderException
   * @expectedExceptionMessage Decision API URL not found. Please verify your settings.
   */
  public function testFailOnMissingDecisionAPIUrl() {
    $credentials = $this->getValidCredentialSettings();
    unset($credentials['decision_api_url']);
    $this->settings->expects($this->at(0))
      ->method('get')
      ->with('credential')
      ->willReturn($credentials);

    $apiLoader = new APILoader($this->configFactory);
    $apiLoader->initialize();
  }
}