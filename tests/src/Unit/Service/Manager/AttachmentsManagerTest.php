<?php

/**
 * @file
 * Contains \Drupal\acquia_lift\Tests\Service\Manager\AttachmentsManagerTest.
 */

namespace Drupal\acquia_lift\Tests\Service\Manager;

use Drupal\Tests\UnitTestCase;
use Drupal\acquia_lift\Service\Manager\AttachmentsManager;
use Drupal\acquia_lift\Tests\Traits\SettingsDataTrait;

require_once(__DIR__ . '/../../../Traits/SettingsDataTrait.php');

/**
 * AttachmentsManager Test.
 *
 * @coversDefaultClass Drupal\acquia_lift\Service\Manager\AttachmentsManager
 * @group acquia_lift
 */
class AttachmentsManagerTest extends UnitTestCase {

  use SettingsDataTrait;

  /**
   * @var \Drupal\Core\Config\ConfigFactoryInterface|\PHPUnit_Framework_MockObject_MockObject
   */
  private $configFactory;

  /**
   * Page context.
   *
   * @var \Drupal\acquia_lift\Service\Context\PageContext|\PHPUnit_Framework_MockObject_MockObject
   */
  private $pageContext;

  /**
   * Path context.
   *
   * @var \Drupal\acquia_lift\Service\Context\PathContext|\PHPUnit_Framework_MockObject_MockObject
   */
  private $pathContext;

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
    $this->pageContext = $this->getMockBuilder('Drupal\acquia_lift\Service\Context\PageContext')
      ->disableOriginalConstructor()
      ->getMock();;
    $this->pathContext = $this->getMockBuilder('Drupal\acquia_lift\Service\Context\PathContext')
      ->disableOriginalConstructor()
      ->getMock();;
    $this->settings = $this->getMockBuilder('Drupal\Core\Config\ImmutableConfig')
      ->disableOriginalConstructor()
      ->getMock();

    $this->configFactory->expects($this->once())
      ->method('get')
      ->with('acquia_lift.settings')
      ->willReturn($this->settings);
    $full_settings = $this->getValidCredentialSettings();
    $this->settings->expects($this->once())
      ->method('get')
      ->with('credential')
      ->willReturn($full_settings);
  }

  /**
   * Tests the getLibrary() method.
   *
   * @covers ::getLibrary
   */
  public function testGetLibrary() {
    $attachments_manager = new AttachmentsManager($this->configFactory, $this->pageContext, $this->pathContext);
    $message = $attachments_manager->getLibrary();
    $this->assertEquals('acquia_lift/acquia_lift', $message);
  }
}
