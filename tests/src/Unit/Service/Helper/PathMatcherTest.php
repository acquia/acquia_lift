<?php

/**
 * @file
 * Contains \Drupal\acquia_lift\Tests\Service\Helper\PathMatcherTest.
 */

namespace Drupal\acquia_lift\Tests\Service\Helper;

use Drupal\Tests\UnitTestCase;
use Drupal\acquia_lift\Service\Helper\PathMatcher;

/**
 * PathMatcher Test.
 *
 * @coversDefaultClass Drupal\acquia_lift\Service\Helper\PathMatcher
 * @group acquia_lift
 */
class PathMatcherTest extends UnitTestCase {
  /**
   * Alias manager.
   *
   * @var \Drupal\Core\Path\AliasManagerInterface|\PHPUnit_Framework_MockObject_MockObject
   */
  private $aliasManager;

  /**
   * Path matcher.
   *
   * @var \Drupal\Core\Path\PathMatcherInterface|\PHPUnit_Framework_MockObject_MockObject
   */
  private $basePathMatcher;

  /**
   * {@inheritdoc}
   */
  public function setUp() {
    parent::setUp();

    $this->aliasManager = $this->getMock('Drupal\Core\Path\AliasManagerInterface');
    $this->basePathMatcher = $this->getMock('Drupal\Core\Path\PathMatcherInterface');
  }

  /**
   * Tests the match() method - no match.
   *
   * @covers ::match
   */
  public function testMatchNoMatch() {
    $this->basePathMatcher->expects($this->exactly(2))
      ->method('matchPath')
      ->willReturn(FALSE);

    $pathMatcher = new PathMatcher($this->aliasManager, $this->basePathMatcher);
    $is_matched = $pathMatcher->match('a_path', 'a_pattern');
    $this->assertFalse($is_matched);
  }
}
