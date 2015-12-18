<?php

/**
 * @file
 * Contains \Drupal\acquia_lift\Tests\Service\Context\PathContextTest.
 */

namespace Drupal\acquia_lift\Tests\Service\Context;

use Drupal\Tests\UnitTestCase;
use Drupal\acquia_lift\Service\Context\PathContext;
use Drupal\acquia_lift\Tests\Traits\SettingsDataTrait;

require_once(__DIR__ . '/../../../Traits/SettingsDataTrait.php');

/**
 * PathContextTest Test.
 *
 * @coversDefaultClass Drupal\acquia_lift\Service\Context\PathContext
 * @group acquia_lift
 */
class PathContextTest extends UnitTestCase {

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
   * Current path stack.
   *
   * @var \Drupal\Core\Path\CurrentPathStack|\PHPUnit_Framework_MockObject_MockObject
   */
  private $currentPathStack;

  /**
   * Request stack.
   *
   * @var \Symfony\Component\HttpFoundation\RequestStack|\PHPUnit_Framework_MockObject_MockObject
   */
  private $requestStack;

  /**
   * Path matcher.
   *
   * @var \Drupal\acquia_lift\Service\Helper\PathMatcher|\PHPUnit_Framework_MockObject_MockObject
   */
  private $pathMatcher;

  /**
   * {@inheritdoc}
   */
  public function setUp() {
    parent::setUp();

    $this->configFactory = $this->getMock('Drupal\Core\Config\ConfigFactoryInterface');
    $this->settings = $this->getMockBuilder('Drupal\Core\Config\ImmutableConfig')
      ->disableOriginalConstructor()
      ->getMock();
    $this->currentPathStack = $this->getMockBuilder('Drupal\Core\Path\CurrentPathStack')
      ->disableOriginalConstructor()
      ->getMock();
    $this->requestStack = $this->getMock('Symfony\Component\HttpFoundation\RequestStack');
    $this->pathMatcher = $this->getMockBuilder('Drupal\acquia_lift\Service\Helper\PathMatcher')
      ->disableOriginalConstructor()
      ->getMock();
    $request = $this->getMock('Symfony\Component\HttpFoundation\Request');

    $this->configFactory->expects($this->once())
      ->method('get')
      ->with('acquia_lift.settings')
      ->willReturn($this->settings);

    $credential_settings = $this->getValidCredentialSettings();
    $visibility_settings = $this->getValidVisibilitySettings();

    $this->settings->expects($this->at(0))
      ->method('get')
      ->with('credential')
      ->willReturn($credential_settings);
    $this->settings->expects($this->at(2))
      ->method('get')
      ->with('visibility')
      ->willReturn($visibility_settings);
    $this->currentPathStack->expects($this->once())
      ->method('getPath')
      ->willReturn('my_current_path');
    $this->requestStack->expects($this->once())
      ->method('getCurrentRequest')
      ->willReturn($request);
    $request->expects($this->once())
      ->method('getQueryString')
      ->willReturn('a_query_string');
  }

  /**
   * Tests the getIdentity() method.
   *
   * @covers ::setIdentityByUser
   * @covers ::getIdentity
   * @param boolean $capture_identity
   * @param integer $call_get_email_times
   * @param array $expected_identity
   *
   * @dataProvider providerTestGetIdentity
   */
  public function testGetIdentity($capture_identity, $call_get_email_times, $expected_identity) {
    $identity_settings = $this->getValidIdentitySettings();
    $identity_settings['capture_identity'] = $capture_identity;
    $this->settings->expects($this->at(1))
      ->method('get')
      ->with('identity')
      ->willReturn($identity_settings);

    $user = $this->getMock('Drupal\user\UserInterface');
    $user->expects($this->exactly($call_get_email_times))
      ->method('getEmail')
      ->willReturn('a_user_email');

    $path_context = new PathContext($this->configFactory, $this->currentPathStack, $this->requestStack, $this->pathMatcher);
    $path_context->setIdentityByUser($user);
    $identity = $path_context->getIdentity();

    $this->assertEquals($expected_identity, $identity);
  }

  /**
   * Data provider for testGetIdentity().
   */
  public function providerTestGetIdentity() {
    $no_capture_identity = FALSE;
    $do_capture_identity = TRUE;
    $call_get_email_zero_times = 0;
    $call_get_email_one_time = 1;
    $expect_empty_identity = NULL;
    $expected_valid_identity = [
      'identity' => 'a_user_email',
      'identityType' => 'email',
    ];

    return [
      'no collect' => [$no_capture_identity, $call_get_email_zero_times, $expect_empty_identity],
      'collect' => [$do_capture_identity, $call_get_email_one_time, $expected_valid_identity],
    ];
  }
}
